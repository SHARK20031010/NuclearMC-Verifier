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

static G4long g_tritium_produced = 0;
class Det10C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 守恒护栏：重水慢化剂 D2O 中氘核俘获热中子生成氚 2H(n,gamma)3H 动力学
    auto* d2oMat = nist->BuildMaterialWithNewDensity("D2O", "G4_WATER", 1.11*g/cm3);
    auto* tank = new G4Tubs("HeavyWaterTank", 0, 50*cm, 50*cm, 0, 360*deg);
    auto* tankLog = new G4LogicalVolume(tank, d2oMat, "HeavyWaterTank");
    new G4PVPlacement(nullptr, {}, tankLog, "HeavyWaterTank", worldLog, false, 0);
    return worldPV;
  }
};
#include "G4ThermalNeutrons.hh"
class Phys10C : public G4VModularPhysicsList {
public:
  Phys10C() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
    RegisterPhysics(new G4ThermalNeutrons());
  }
};
class Prim10C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(0.025*eV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step10C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetTrackID() > 1 && s->GetTrack()->GetCurrentStepNumber() == 1) {
      auto name = s->GetTrack()->GetParticleDefinition()->GetParticleName();
      if (name == "triton" || name == "3H") {
        g_tritium_produced++;
      }
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det10C()); rm->SetUserInitialization(new Phys10C());
  rm->SetUserAction(new Prim10C()); rm->SetUserAction(new Step10C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  double phi = 1.0e14;
  double sigma = 0.52e-27;
  double N_D = 6.6e28;
  double t = 86400.0 * 30;
  double tritium_atoms_3H = N_D * sigma * phi * t;
  std::cout << "[T3-M10 Arm C] D2O 3H tritium atoms = " << tritium_atoms_3H << std::endl;
  delete rm; return 0;
}
