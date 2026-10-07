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
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "G4UserLimits.hh"
#include "G4StepLimiterPhysics.hh"
static double gEdep = 0.0;
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 1*um, 1*um, 1*um), nist->FindOrBuildMaterial("G4_WATER"), "W");
    auto* gold = new G4LogicalVolume(new G4Sphere("Au", 0, 50*nm, 0, 360*deg, 0, 180*deg), nist->FindOrBuildMaterial("G4_Au"), "Au");
    gold->SetUserLimits(new G4UserLimits(10*nm)); // 修复：挂载纳米微步长限制
    world->SetUserLimits(new G4UserLimits(50*nm));
    new G4PVPlacement(nullptr, {}, gold, "Au", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};
class Phys : public G4VModularPhysicsList {
public:
  Phys() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4StepLimiterPhysics()); // 修复：注册步长限制器物理
  }
};
class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(50 * keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-500*nm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { gEdep += s->GetTotalEnergyDeposit(); }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det()); rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim()); rm->SetUserAction(new Step());
  rm->Initialize(); rm->BeamOn(argc>1?std::atoi(argv[1]):100);
  std::cout << "T2-8 Edep = " << gEdep << std::endl;
  delete rm; return 0;
}
