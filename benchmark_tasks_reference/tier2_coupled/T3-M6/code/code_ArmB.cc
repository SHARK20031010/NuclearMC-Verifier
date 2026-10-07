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

class Det6B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    return new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);
  }
};
class Phys6B : public G4VModularPhysicsList {
public:
  Phys6B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim6B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e+"));
    gun.SetParticleEnergy(0.633*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { (void)s; }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6B()); rm->SetUserInitialization(new Phys6B());
  rm->SetUserAction(new Prim6B()); rm->SetUserAction(new Step6B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // 通用自检：有效清除常数 lambda_eff = lambda_phys + lambda_bio
  double lambda_p = std::log(2.0) / 109.8; // 109.8 min
  double lambda_b = std::log(2.0) / 120.0; // 2 h clearance
  double lambda_eff = lambda_p + lambda_b; // effective clearance
  std::cout << "[T3-M6 Arm B] Effective lambda = " << lambda_eff << std::endl;
  delete rm; return 0;
}
