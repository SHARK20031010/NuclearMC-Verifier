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
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

static G4double gZenithSum = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 10*m, 10*m, 10*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("mu-"));
    gun.SetParticleEnergy(4 * GeV);
    gun.SetParticlePosition({0, 0, 5*m});
    
    G4double xi = G4UniformRand();
    G4double cos_theta = std::pow(1.0 - xi, 1.0/3.0);
    G4double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
    G4double phi = 2.0 * M_PI * G4UniformRand();
    G4ThreeVector dir(sin_theta * std::cos(phi), sin_theta * std::sin(phi), -cos_theta);
    gun.SetParticleMomentumDirection(dir);
    gun.GeneratePrimaryVertex(ev);
    gZenithSum += std::acos(cos_theta);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step*) override {}
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "T6-M6 Mean Zenith Angle = " << gZenithSum/50 << " rad" << std::endl;
  std::_Exit(0);
}
