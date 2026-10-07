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

static G4long gPipeLeak = 0;
class Det8B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 5*m, 5*m, 5*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* pipeSolid = new G4Box("VentPipe", 30*cm, 30*cm, 1*m);
    auto* pipeLog = new G4LogicalVolume(pipeSolid, nist->FindOrBuildMaterial("G4_AIR"), "VentPipe");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,1*m), pipeLog, "VentPipe", worldLog, false, 0);
    return worldPV;
  }
};
class Phys8B : public G4VModularPhysicsList {
public:
  Phys8B() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim8B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(230.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 2.0*m) gPipeLeak++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8B()); rm->SetUserInitialization(new Phys8B());
  rm->SetUserAction(new Prim8B()); rm->SetUserAction(new Step8B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M8 Arm B] Vent Pipe Leak = " << gPipeLeak << std::endl;
  delete rm; return 0;
}
