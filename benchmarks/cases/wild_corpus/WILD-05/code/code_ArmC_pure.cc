// WILD-05 Fixed Code - Arm C Pure First Principles
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
#include "Randomize.hh"
#include "FTFP_BERT.hh"
#include <iostream>
#include <cmath>

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 1*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1 * MeV);
    G4double R = 5.0 * cm;
    // Principle 1: Differential Phase Space Measure Invariance (Jacobian Conservation).
    // In polar coordinates dA = r dr dphi. For uniform areal distribution, P(r) dr = 2r/R^2 dr.
    // Inverse transform: r = R * sqrt(u), where u ~ Uniform(0,1).
    // Sampling r = R * u causes 1/r artificial center concentration (measure distortion).
    G4double r = R * std::sqrt(G4UniformRand()); 
    G4double phi = 2.0 * M_PI * G4UniformRand();
    gun.SetParticlePosition({r * std::cos(phi), r * std::sin(phi), 0});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new FTFP_BERT());
  rm->SetUserAction(new Prim());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 10);
  std::cout << "WILD-05 Generation completed" << std::endl;
  delete rm; 
  std::_Exit(0);
}
