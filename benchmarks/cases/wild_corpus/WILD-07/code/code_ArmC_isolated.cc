// WILD-07: Photonuclear reaction with 50 MeV electrons on tungsten converter target
// Fixed code based on first-principles physics derivation and Geant4 lifecycle contracts.

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
#include "FTFP_BERT.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Neutron.hh"
#include <iostream>
#include <cstdlib>

static G4long gNeutrons = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 1*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* target = new G4LogicalVolume(new G4Box("Target", 5*cm, 5*cm, 2*mm), nist->FindOrBuildMaterial("G4_W"), "Target");
    new G4PVPlacement(nullptr, {0,0,0}, target, "Target", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Phys : public FTFP_BERT {
public:
  Phys() : FTFP_BERT() {
    ReplacePhysics(new G4EmStandardPhysics_option4());
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(50.0 * MeV);
    gun.SetParticlePosition({0,0,-10*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* track = s->GetTrack();
    if (track->GetParticleDefinition() == G4Neutron::NeutronDefinition() &&
        track->GetCurrentStepNumber() == 1) {
      gNeutrons++;
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  int nEvents = argc > 1 ? std::atoi(argv[1]) : 10;
  rm->BeamOn(nEvents);
  std::cout << "WILD-07 Neutrons = " << gNeutrons << std::endl;
  delete rm;
  std::_Exit(0);
}
