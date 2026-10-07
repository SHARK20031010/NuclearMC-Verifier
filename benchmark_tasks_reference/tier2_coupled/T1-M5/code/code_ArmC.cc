#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
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

static G4double dose_profile[10] = {0};
class Det5C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 3*m, 3*m, 4*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* innerSolid = new G4Tubs("InnerSS", 29*cm, 30*cm, 1*m, 0, 360*deg);
    auto* innerLog = new G4LogicalVolume(innerSolid, nist->FindOrBuildMaterial("G4_Fe"), "InnerSS");
    new G4PVPlacement(nullptr, {}, innerLog, "InnerSS", worldLog, false, 0);

    auto* waterSolid = new G4Tubs("WaterShield", 30*cm, 50*cm, 1*m, 0, 360*deg);
    auto* waterLog = new G4LogicalVolume(waterSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterShield");
    new G4PVPlacement(nullptr, {}, waterLog, "WaterShield", worldLog, false, 1);

    auto* pbSolid = new G4Tubs("OuterPb", 50*cm, 52*cm, 1*m, 0, 360*deg);
    auto* pbLog = new G4LogicalVolume(pbSolid, nist->FindOrBuildMaterial("G4_Pb"), "OuterPb");
    new G4PVPlacement(nullptr, {}, pbLog, "OuterPb", worldLog, false, 2);
    return worldPV;
  }
};
class Phys5C : public G4VModularPhysicsList {
public:
  Phys5C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim5C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(0.662*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,(G4UniformRand()-0.5)*1.8*m));
    gun.SetParticleMomentumDirection(G4ThreeVector(1,0,0));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step5C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    int z_bin = (pos.z() + 1.0*m) / (0.2*m);
    if (z_bin >= 0 && z_bin < 10) {
      dose_profile[z_bin] += s->GetTotalEnergyDeposit();
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det5C()); rm->SetUserInitialization(new Phys5C());
  rm->SetUserAction(new Prim5C()); rm->SetUserAction(new Step5C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M5 Arm C] Height Dose Profile sum = " << dose_profile[0]+dose_profile[5] << std::endl;
  delete rm; return 0;
}
