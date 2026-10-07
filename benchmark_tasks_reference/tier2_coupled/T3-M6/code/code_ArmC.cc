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

class Det6C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    return new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);
  }
};
class Phys6C : public G4VModularPhysicsList {
public:
  Phys6C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim6C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e+"));
    gun.SetParticleEnergy(0.633*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { (void)s; }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6C()); rm->SetUserInitialization(new Phys6C());
  rm->SetUserAction(new Prim6C()); rm->SetUserAction(new Step6C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // 守恒护栏：18F 物理半衰期 109.8 min 与生物排泄清除 (bio clearance) 耦合
  double T_phys = 109.8; // min
  double T_bio = 120.0;  // min
  double lambda_p = std::log(2.0) / T_phys;
  double lambda_b = std::log(2.0) / T_bio;
  double effective_lambda = lambda_p + lambda_b; // combined clearance
  double cum_dose_factor = 1.0 / effective_lambda;
  std::cout << "[T3-M6 Arm C] 18F cumulative dose factor = " << cum_dose_factor << std::endl;
  delete rm; return 0;
}
