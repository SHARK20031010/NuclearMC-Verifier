// WILD-01 Fixed Code
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
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "FTFP_BERT.hh"
#include <iostream>

static G4long gSecondaryCount = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldMat = nist->FindOrBuildMaterial("G4_AIR");
    auto* targetMat = nist->FindOrBuildMaterial("G4_Pb");
    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 2*m), worldMat, "W");
    auto* target = new G4LogicalVolume(new G4Box("Target", 20*cm, 20*cm, 50*cm), targetMat, "Target");
    new G4PVPlacement(nullptr, {0,0,25*cm}, target, "Target", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.0 * GeV);
    gun.SetParticlePosition({0,0,-10*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* track = s->GetTrack();
    if (track->GetCurrentStepNumber() == 1 && track->GetParentID() > 0 && track->GetParticleDefinition()->GetParticleName() == "neutron") {
      gSecondaryCount++;
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new FTFP_BERT());
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 20);
  std::cout << "WILD-01 Secondary Neutrons = " << gSecondaryCount << std::endl;
  delete rm;
  std::_Exit(0);
}
