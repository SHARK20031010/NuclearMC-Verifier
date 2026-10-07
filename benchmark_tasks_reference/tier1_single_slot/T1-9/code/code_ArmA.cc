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

static G4long gMuon = 0;
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 20*m, 20*m, 20*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* wpv = new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
    auto* rock = new G4LogicalVolume(new G4Box("Rock", 10*m, 10*m, 5*m), nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE"), "Rock");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,5*m), rock, "Rock", world, false, 0);
    return wpv;
  }
};
class Phys : public G4VModularPhysicsList {
public:
  Phys() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("mu-"));
    gun.SetParticleEnergy(4 * GeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,12*m));
        gun.SetParticleMomentumDirection(G4ThreeVector(0,0,-1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() <= 0.0) gMuon++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det()); rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim()); rm->SetUserAction(new Step());
  rm->Initialize(); rm->BeamOn(argc>1?std::atoi(argv[1]):100);
  std::cout << "T1-9 Muons through = " << gMuon << std::endl;
  delete rm; return 0;
}
