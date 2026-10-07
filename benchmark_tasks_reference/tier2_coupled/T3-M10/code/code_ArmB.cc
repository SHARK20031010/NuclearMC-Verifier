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

class Det10B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 通用自检：重水 D2O 慢化剂中氚 (tritium) 生成动力学
    auto* d2oMat = nist->BuildMaterialWithNewDensity("D2O", "G4_WATER", 1.11*g/cm3);
    auto* tank = new G4Tubs("D2O_Tank", 0, 50*cm, 50*cm, 0, 360*deg);
    auto* tankLog = new G4LogicalVolume(tank, d2oMat, "D2O_Tank");
    new G4PVPlacement(nullptr, {}, tankLog, "D2O_Tank", worldLog, false, 0);
    return worldPV;
  }
};
class Phys10B : public G4VModularPhysicsList {
public:
  Phys10B() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim10B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(0.025*eV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step10B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { (void)s; }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det10B()); rm->SetUserInitialization(new Phys10B());
  rm->SetUserAction(new Prim10B()); rm->SetUserAction(new Step10B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  double phi = 1.0e14; // n/cm2/s
  double sigma = 0.52e-3 * 1e-24; // 0.52 mb
  double N_D = 6.6e22; // cm-3
  double t = 86400.0 * 30; // 30 days
  double tritium_yield = N_D * sigma * phi * t;
  std::cout << "[T3-M10 Arm B] D2O tritium accumulation = " << tritium_yield << std::endl;
  delete rm; return 0;
}
