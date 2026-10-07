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

static G4long gStreamCounts = 0;
class Det6B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* fe = nist->FindOrBuildMaterial("G4_Fe");
    auto* plate1 = new G4Box("Steel_1", 20*cm, 50*cm, 10*cm);
    auto* log1 = new G4LogicalVolume(plate1, fe, "Steel_1");
    new G4PVPlacement(nullptr, G4ThreeVector(-20.1*cm, 0, 0), log1, "Steel_1", worldLog, false, 0);

    auto* plate2 = new G4Box("Steel_2", 20*cm, 50*cm, 10*cm);
    auto* log2 = new G4LogicalVolume(plate2, fe, "Steel_2");
    new G4PVPlacement(nullptr, G4ThreeVector(20.1*cm, 0, 0), log2, "Steel_2", worldLog, false, 1);
    return worldPV;
  }
};
class Phys6B : public G4VModularPhysicsList {
public:
  Phys6B() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim6B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(14.0*MeV);
    gun.SetParticlePosition(G4ThreeVector((G4UniformRand()-0.5)*4*mm, 0, -30*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    if (pos.z() > 15*cm && std::abs(pos.x()) < 2*mm) gStreamCounts++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6B()); rm->SetUserInitialization(new Phys6B());
  rm->SetUserAction(new Prim6B()); rm->SetUserAction(new Step6B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M6 Arm B] Gap Streaming Counts = " << gStreamCounts << std::endl;
  delete rm; return 0;
}
