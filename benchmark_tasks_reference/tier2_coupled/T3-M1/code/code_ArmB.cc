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

class Det1B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    return new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);
  }
};
class Phys1B : public G4VModularPhysicsList {
public:
  Phys1B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim1B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(140.5*keV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step1B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { (void)s; }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det1B()); rm->SetUserInitialization(new Phys1B());
  rm->SetUserAction(new Prim1B()); rm->SetUserAction(new Step1B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // 盲区：通用自检写出了 Bateman 方程，但误认为分支比为 100%，缺失 87.5% 分支比 (0.875)
  double lambda_Mo = std::log(2.0) / (66.0 * 3600.0);
  double lambda_Tc = std::log(2.0) / (6.0 * 3600.0);
  double t = 24.0 * 3600.0;
  double N_Mo = 1.0e6 * std::exp(-lambda_Mo * t);
  double N_Tc = 1.0 * (lambda_Mo / (lambda_Tc - lambda_Mo)) * 1.0e6 * (std::exp(-lambda_Mo * t) - std::exp(-lambda_Tc * t));
  std::cout << "[T3-M1 Arm B] Bateman 100% Branch: N_Tc = " << N_Tc << ", N_Mo = " << N_Mo << std::endl;
  delete rm; return 0;
}
