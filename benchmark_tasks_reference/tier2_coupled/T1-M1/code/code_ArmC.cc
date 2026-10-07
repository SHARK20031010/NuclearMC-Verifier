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

static G4double g_exit_gamma_flux = 0;
static G4double g_n_out_flux = 0;
class Det1C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 6*m, 6*m, 6*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* conc = nist->FindOrBuildMaterial("G4_CONCRETE");
    auto* mazeWall1 = new G4Box("Maze_Wall_1", 1.5*m, 0.4*m, 2.0*m);
    auto* mazeLog1 = new G4LogicalVolume(mazeWall1, conc, "Maze_Wall_1");
    new G4PVPlacement(nullptr, G4ThreeVector(-0.8*m, 0, -1.0*m), mazeLog1, "Maze_Wall_1", worldLog, false, 0);

    auto* mazeWall2 = new G4Box("Maze_Wall_2", 1.5*m, 0.4*m, 2.0*m);
    auto* mazeLog2 = new G4LogicalVolume(mazeWall2, conc, "Maze_Wall_2");
    new G4PVPlacement(nullptr, G4ThreeVector(0.8*m, 0, 1.0*m), mazeLog2, "Maze_Wall_2", worldLog, false, 1);
    return worldPV;
  }
};
class Phys1C : public G4VModularPhysicsList {
public:
  Phys1C() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim1C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    if (G4UniformRand() > 0.5) {
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
      gun.SetParticleEnergy(2.0*MeV);
    } else {
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
      gun.SetParticleEnergy(2.0*MeV);
    }
    gun.SetParticlePosition(G4ThreeVector(-2.0*m, 0, -2.5*m));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step1C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 2.5*m && s->GetPreStepPoint()->GetPosition().z() <= 2.5*m) {
      auto name = s->GetTrack()->GetParticleDefinition()->GetParticleName();
      if (name == "gamma") g_exit_gamma_flux += 1.0;
      else if (name == "neutron") g_n_out_flux += 1.0;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det1C()); rm->SetUserInitialization(new Phys1C());
  rm->SetUserAction(new Prim1C()); rm->SetUserAction(new Step1C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M1 Arm C] Exit gamma flux = " << g_exit_gamma_flux << ", n_out flux = " << g_n_out_flux << std::endl;
  delete rm; return 0;
}
