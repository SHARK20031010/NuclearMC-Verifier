#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4Cons.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

static G4double gDepthDose[30] = {0};
class Det2B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* phantomSolid = new G4Box("WaterPhantom", 15*cm, 15*cm, 15*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterPhantom");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15*cm), phantomLog, "WaterPhantom", worldLog, false, 0);
    return worldPV;
  }
};
class Phys2B : public G4VModularPhysicsList {
public:
  Phys2B() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim2B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    // 通用自检：调制 100, 85, 70 MeV 三组能量形成 SOBP
    double r = G4UniformRand();
    double E = 100.0*MeV;
    double weight = 1.0;
    if (r < 0.5) { E = 100.0*MeV; weight = 0.5; }
    else if (r < 0.8) { E = 85.0*MeV; weight = 0.3; }
    else { E = 70.0*MeV; weight = 0.2; }
    (void)weight;
    gun.SetParticleEnergy(E);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step2B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    int zBin = pos.z() / (1.0*cm);
    if (zBin >= 0 && zBin < 30) gDepthDose[zBin] += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det2B()); rm->SetUserInitialization(new Phys2B());
  rm->SetUserAction(new Prim2B()); rm->SetUserAction(new Step2B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M2 Arm B] SOBP depth dose = " << gDepthDose[10] << std::endl;
  delete rm; return 0;
}
