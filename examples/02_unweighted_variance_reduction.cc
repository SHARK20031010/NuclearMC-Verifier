// =============================================================================
// Example 02: Unweighted Tally in Variance Reduction (PST-INV-VARIANCE-01)
// -----------------------------------------------------------------------------
// Physical Setup:
//   14 MeV neutrons penetrating an iron shield (50x50x50 cm) along Z-axis.
// Defect Description:
//   SteppingAction accumulates flux tally across z=100cm boundary:
//     gFlux += 1.0;
//   In variance reduction or weighted transport, scoring tracks without
//   multiplying by `step->GetTrack()->GetWeight()` violates statistical
//   unbiasedness and produces completely distorted flux expectations.
// Expected Output of NuclearMC-Verifier:
//   Flags PST-INV-VARIANCE-01, highlights line, and provides exact fix diff:
//   `gFlux += s->GetTrack()->GetWeight();`.
// =============================================================================

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
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include <iostream>

static double gFlux = 0.0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 2*m, 2*m, 2*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* wpv = new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
    auto* fe = new G4LogicalVolume(new G4Box("Fe", 50*cm, 50*cm, 50*cm), nist->FindOrBuildMaterial("G4_Fe"), "Fe");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,50*cm), fe, "Fe", world, false, 0);
    return wpv;
  }
};

class Phys : public G4VModularPhysicsList {
public:
  Phys() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
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
    if (s->GetPostStepPoint()->GetPosition().z() >= 100.0*cm && s->GetPreStepPoint()->GetPosition().z() < 100.0*cm) {
      // 🚨 SILENT DEFECT: Scoring unweighted track unit (+1.0) instead of track weight
      gFlux += 1.0;
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
  rm->BeamOn(100);
  std::cout << "Penetration Flux = " << gFlux << std::endl;
  delete rm;
  return 0;
}
