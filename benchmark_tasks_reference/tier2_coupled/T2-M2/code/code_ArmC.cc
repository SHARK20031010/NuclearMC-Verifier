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

static G4double g_sobp_depth_dose[30] = {0};
class Det2C : public G4VUserDetectorConstruction {
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
class Phys2C : public G4VModularPhysicsList {
public:
  Phys2C() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim2C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    // 守恒护栏：E1 (100 MeV), E2 (85 MeV), E3 (70 MeV) 调制扩束平坦 SOBP
    double r = G4UniformRand();
    double E = 100.0*MeV;
    double weight = 1.0;
    if (r < 0.45) { E = 100.0*MeV; weight = 0.45; }
    else if (r < 0.75) { E = 85.0*MeV; weight = 0.30; }
    else { E = 70.0*MeV; weight = 0.25; }
    (void)weight;
    gun.SetParticleEnergy(E);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step2C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    int zBin = pos.z() / (1.0*cm);
    if (zBin >= 0 && zBin < 30) g_sobp_depth_dose[zBin] += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det2C()); rm->SetUserInitialization(new Phys2C());
  rm->SetUserAction(new Prim2C()); rm->SetUserAction(new Step2C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M2 Arm C] SOBP depth plateau = " << g_sobp_depth_dose[8] << std::endl;
  delete rm; return 0;
}
