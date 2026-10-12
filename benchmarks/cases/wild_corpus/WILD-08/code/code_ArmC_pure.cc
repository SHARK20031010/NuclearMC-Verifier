// WILD-08 Fixed Code - Arm C Pure First Principles
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
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include <iostream>
#include <cmath>

static double gSigmaN = 0.0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 1*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* det = new G4LogicalVolume(new G4Box("Det", 5*cm, 5*cm, 5*cm), nist->FindOrBuildMaterial("G4_Ge"), "Det");
    new G4PVPlacement(nullptr, {0,0,0}, det, "Det", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.332 * MeV);
    gun.SetParticlePosition({0,0,-10*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    double edep = s->GetTotalEnergyDeposit();
    if (edep > 0) {
      double mean_eh_pairs = edep / (2.96 * eV);
      // Principle 5: Microscopic Non-linear Response and Discrete Excitation Correlation.
      // Semiconductor charge carrier creation is constrained by microscopic energy conservation,
      // resulting in sub-Poisson discrete fluctuations characterized by the Fano factor (F ~ 0.10 for HPGe).
      // Assuming pure Poisson statistics (F = 1) overestimates carrier variance by 1/F ~ 10x,
      // making the simulated peak width sqrt(10) ~ 3.16x too broad.
      const double fano = 0.10;
      double var_N = fano * mean_eh_pairs; 
      gSigmaN += std::sqrt(var_N);
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
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 10);
  std::cout << "WILD-08 Carrier Sigma_N = " << gSigmaN << std::endl;
  delete rm; 
  std::_Exit(0);
}
