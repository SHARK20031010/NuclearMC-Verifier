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

class Det2B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    return new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);
  }
};
class Phys2B : public G4VModularPhysicsList {
public:
  Phys2B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim2B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.0*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step2B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { (void)s; }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det2B()); rm->SetUserInitialization(new Phys2B());
  rm->SetUserAction(new Prim2B()); rm->SetUserAction(new Step2B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // 通用自检：停堆裂变产物衰变热发热功率积分
  double P0 = 1000.0;
  double t = 100.0;
  double decay_heat_power = 0.0622 * P0 * std::pow(t, -0.2);
  std::cout << "[T3-M2 Arm B] Decay heat power at 100s = " << decay_heat_power << std::endl;
  delete rm; return 0;
}
