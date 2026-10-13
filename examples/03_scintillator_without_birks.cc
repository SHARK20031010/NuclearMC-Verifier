// =============================================================================
// Example 03: Missing Birks Quenching in Scintillator (PHYS-INV-BIRKS-01)
// -----------------------------------------------------------------------------
// Physical Setup:
//   Plastic scintillator (5x5x5 cm) irradiated by 1 MeV gammas and 1 MeV neutrons.
// Defect Description:
//   The material properties table defines `SCINTILLATIONYIELD`, but fails to
//   configure the non-linear Birks quenching constant (`SetBirksConstant`).
//   Without Birks saturation, Geant4 assumes a linear light output with dE/dx,
//   causing the scintillation yield for high-LET recoil protons and ions
//   to be severely overestimated by a factor of 2x to 3x!
// Expected Output of NuclearMC-Verifier:
//   Flags PHYS-INV-BIRKS-01 / SCINTILLATOR_BIRKS_QUENCHING_CONTRACT,
//   diagnosing the defect and suggesting:
//   `scintMaterial->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);`.
// =============================================================================

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalPhoton.hh"
#include "G4PVPlacement.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysListFactory.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserSteppingAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include <iostream>

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* sc = new G4Material("PlasticScint", 1.032 * g / cm3, 2);
    sc->AddElement(nist->FindOrBuildElement("C"), 10);
    sc->AddElement(nist->FindOrBuildElement("H"), 11);

    const G4int N = 2;
    G4double en[N] = {1.0 * eV, 6.0 * eV};
    G4double rind[N] = {1.58, 1.58};
    G4double comp[N] = {1.0, 1.0};

    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", en, rind, N);
    mpt->AddProperty("SCINTILLATIONCOMPONENT1", en, comp, N);
    // 🚨 SILENT DEFECT: Scintillation yield configured WITHOUT Birks quenching constant
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV);
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0 * ns);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    sc->SetMaterialPropertiesTable(mpt);

    auto* world = new G4PVPlacement(nullptr, {}, new G4LogicalVolume(
        new G4Box("world", 20*cm, 20*cm, 20*cm),
        nist->FindOrBuildMaterial("G4_AIR"), "world"), "world", nullptr, false, 0);

    new G4PVPlacement(nullptr, {}, new G4LogicalVolume(
        new G4Box("sc", 2.5*cm, 2.5*cm, 2.5*cm), sc, "sc"), "sc",
        world->GetLogicalVolume(), false, 0);

    return world;
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    // Scoring energy deposit and scintillation counts
  }
};

class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun gun;
public:
  Primary() : gun(1) {
    gun.SetParticleEnergy(1.0 * MeV);
    gun.SetParticlePosition({0, 0, -5 * cm});
    gun.SetParticleMomentumDirection({0, 0, 1});
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
  }
  void GeneratePrimaries(G4Event* ev) override { gun.GeneratePrimaryVertex(ev); }
};

class Action : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Primary);
    SetUserAction(new Stepping);
  }
};

int main() {
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  auto* fac = new G4PhysListFactory;
  auto* pl = (G4VModularPhysicsList*)fac->GetReferencePhysList("QGSP_BIC_HP");
  pl->RegisterPhysics(new G4OpticalPhysics);
  rm->SetUserInitialization(pl);
  rm->SetUserInitialization(new Action);
  rm->Initialize();
  rm->BeamOn(50);
  delete rm;
  return 0;
}
