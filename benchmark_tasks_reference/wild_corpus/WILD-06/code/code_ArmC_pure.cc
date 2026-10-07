// WILD-06 Fixed Code - Arm C Pure First Principles
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
#include "G4EmStandardPhysics.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4OpticalPhysics.hh"
#include "G4MaterialPropertiesTable.hh"
#include <iostream>

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* scMat = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
    auto* mpt = new G4MaterialPropertiesTable();
    std::vector<G4double> en = {2.0*eV, 3.5*eV};
    std::vector<G4double> rindex = {1.58, 1.58};
    std::vector<G4double> scint = {1.0, 1.0};
    mpt->AddProperty("RINDEX", en, rindex);
    mpt->AddProperty("SCINTILLATIONCOMPONENT1", en, scint);
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000./MeV);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 1.0*ns);
    scMat->SetMaterialPropertiesTable(mpt);

    // Principle 5: Microscopic Non-linear Response and Complete Excited State Degrees of Freedom.
    // Highly ionizing recoil protons in plastic scintillators undergo non-radiative quenching (Birks saturation).
    // Neglecting Birks quenching assumes an idealized linear response, overestimating recoil proton light yield by ~3x.
    // We activate Birks non-linear saturation for polyvinyltoluene plastic scintillator:
    scMat->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);

    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 1*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* det = new G4LogicalVolume(new G4Box("Det", 5*cm, 5*cm, 5*cm), scMat, "Det");
    new G4PVPlacement(nullptr, {0,0,0}, det, "Det", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Phys : public G4VModularPhysicsList {
public:
  Phys() {
    RegisterPhysics(new G4EmStandardPhysics());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
    RegisterPhysics(new G4OpticalPhysics());
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(14.0 * MeV);
    gun.SetParticlePosition({0,0,-10*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 5);
  std::cout << "WILD-06 Simulation completed" << std::endl;
  delete rm; 
  std::_Exit(0);
}
