#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserTrackingAction.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include <iostream>
#include <cstdlib>

static G4long gTransCounts = 0;
class Det9A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 3*m, 3*m, 3*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* wallSolid = new G4Box("ConcreteWall", 1*m, 1*m, 5*cm);
    auto* wallLog = new G4LogicalVolume(wallSolid, nist->FindOrBuildMaterial("G4_CONCRETE"), "ConcreteWall");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), wallLog, "ConcreteWall", worldLog, false, 0);
    return worldPV;
  }
};
class Phys9A : public G4VModularPhysicsList {
public:
  Phys9A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim9A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(22));
    gun.SetParticleEnergy(1.33*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-50*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class TrackAction9A : public G4UserTrackingAction {
public:
  void PostUserTrackingAction(const G4Track* t) override {
    if (t->GetPosition().z() > 10*cm) gTransCounts++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det9A()); rm->SetUserInitialization(new Phys9A());
  rm->SetUserAction(new Prim9A()); rm->SetUserAction(new TrackAction9A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M9 Arm A] Transmitted = " << gTransCounts << std::endl;
  delete rm; return 0;
}
