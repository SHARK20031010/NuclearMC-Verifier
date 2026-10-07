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

#include "G4HadronPhysicsQGSP_BIC_HP.hh"
static G4long gUncollided = 0, gScatter = 0;
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 2*m, 2*m, 2*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* wpv = new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
    auto* wall = new G4LogicalVolume(new G4Box("Wall", 1*m, 1*m, 50*cm), nist->FindOrBuildMaterial("G4_CONCRETE"), "Wall");
    auto* duct = new G4LogicalVolume(new G4Tubs("Duct", 0, 5*cm, 50*cm, 0, 360*deg), nist->FindOrBuildMaterial("G4_AIR"), "Duct");
    new G4PVPlacement(nullptr, {}, duct, "Duct", wall, false, 0);
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,50*cm), wall, "Wall", world, false, 0);
    return wpv;
  }
};
class Phys : public G4VModularPhysicsList {
public:
  Phys() { RegisterPhysics(new G4EmStandardPhysics_option4()); RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP()); }
};
class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(14 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() >= 100*cm && s->GetPreStepPoint()->GetPosition().z() < 100*cm) {
      if (s->GetTrack()->GetCurrentStepNumber() <= 2) gUncollided++; else gScatter++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det()); rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim()); rm->SetUserAction(new Step());
  rm->Initialize(); rm->BeamOn(argc>1?std::atoi(argv[1]):100);
  std::cout << "T1-10 uncollided = " << gUncollided << ", scatter = " << gScatter << std::endl;
  delete rm; return 0;
}
