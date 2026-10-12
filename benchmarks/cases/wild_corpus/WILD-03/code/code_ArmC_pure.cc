// WILD-03 Fixed Code - Arm C Pure First Principles
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
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4OpticalPhysics.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include <iostream>

static G4long gScintPhotons = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");
    auto* mpt = new G4MaterialPropertiesTable();

    // Principle 5: Microscopic non-linear response and complete excited state degrees of freedom.
    // For optical photon generation and transport, the medium must possess:
    // 1. Refractive index (RINDEX) to define phase space velocity and boundary propagation;
    // 2. Excited state transition spectrum (SCINTILLATIONCOMPONENT1);
    // 3. Scintillation yield, sub-Poisson fluctuation scale (RESOLUTIONSCALE), and decay lifetimes.
    std::vector<G4double> photonEnergy = { 2.0 * eV, 3.5 * eV };
    std::vector<G4double> rIndex = { 1.33, 1.33 };
    std::vector<G4double> scintSpectrum = { 1.0, 1.0 };

    mpt->AddProperty("RINDEX", photonEnergy, rIndex);
    mpt->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy, scintSpectrum);
    mpt->AddConstProperty("SCINTILLATIONYIELD", 5000.0 / MeV);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 1.0 * ns);
    mpt->AddConstProperty("SCINTILLATIONYIELD1", 1.0);

    water->SetMaterialPropertiesTable(mpt);
    auto* world = new G4LogicalVolume(new G4Box("W", 50*cm, 50*cm, 50*cm), water, "W");
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(10 * MeV);
    gun.SetParticlePosition({0,0,-10*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetTrack()->GetParticleDefinition()->GetParticleName() == "opticalphoton" &&
        step->GetTrack()->GetCurrentStepNumber() == 1) {
      gScintPhotons++;
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());
  pl->RegisterPhysics(new G4OpticalPhysics());
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 10);
  std::cout << "WILD-03 Optical Photons Generated = " << gScintPhotons << std::endl;
  std::cout << "WILD-03 Simulation completed" << std::endl;
  delete rm; 
  std::_Exit(0);
}
