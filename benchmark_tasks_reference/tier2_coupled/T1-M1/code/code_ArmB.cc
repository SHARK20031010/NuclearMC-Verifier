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

static G4double gExit_flux = 0;
class Det1B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 5*m, 5*m, 5*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* conc = nist->FindOrBuildMaterial("G4_CONCRETE");
    auto* maze1Solid = new G4Box("maze_wall_1", 1.2*m, 0.3*m, 1.5*m);
    auto* maze1Log = new G4LogicalVolume(maze1Solid, conc, "maze_wall_1");
    new G4PVPlacement(nullptr, G4ThreeVector(-0.5*m, 0, -0.5*m), maze1Log, "maze_wall_1", worldLog, false, 0);

    auto* maze2Solid = new G4Box("maze_wall_2", 1.2*m, 0.3*m, 1.5*m);
    auto* maze2Log = new G4LogicalVolume(maze2Solid, conc, "maze_wall_2");
    new G4PVPlacement(nullptr, G4ThreeVector(0.5*m, 0, 0.5*m), maze2Log, "maze_wall_2", worldLog, false, 1);
    return worldPV;
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
    gun.SetParticleEnergy(2.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(-1.5*m, 0, -1.5*m));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step1B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 1.8*m) {
      gExit_flux += 1.0; // exit flux tally
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det1B()); rm->SetUserInitialization(new Phys1B());
  rm->SetUserAction(new Prim1B()); rm->SetUserAction(new Step1B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M1 Arm B] Exit flux = " << gExit_flux << std::endl;
  delete rm; return 0;
}
