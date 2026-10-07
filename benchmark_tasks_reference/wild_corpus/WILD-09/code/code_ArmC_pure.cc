// WILD-09 Fixed Code - Arm C Pure First Principles
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
#include "G4ThermalNeutrons.hh"
#include "G4EmStandardPhysics.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include <iostream>

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    // Principle 5: Microscopic Thermal State and Lattice/Molecular Scattering Completeness.
    // For thermal neutron moderation below 4 eV, neutrons interact with molecular binding states
    // described by the S(alpha, beta) thermal scattering law.
    // Geant4's G4ThermalNeutrons associates S(alpha, beta) datasets with standard NIST material "G4_WATER"
    // (or "TS_H_of_Water"). A generic custom material named "H2O" bypasses thermal scattering lookup.
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 1*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* mod = new G4LogicalVolume(new G4Box("Tank", 20*cm, 20*cm, 20*cm), water, "Tank");
    new G4PVPlacement(nullptr, {0,0,0}, mod, "Tank", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Phys : public G4VModularPhysicsList {
public:
  Phys() {
    RegisterPhysics(new G4EmStandardPhysics());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
    // G4ThermalNeutrons attaches S(alpha, beta) thermal scattering model to the HP neutron elastic process;
    // G4HadronElasticPhysicsHP must be registered for the underlying elastic process to exist.
    RegisterPhysics(new G4HadronElasticPhysicsHP());
    RegisterPhysics(new G4ThermalNeutrons());
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(0.025 * eV);
    gun.SetParticlePosition({0,0,-15*cm});
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
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 10);
  std::cout << "WILD-09 Thermal Moderator completed" << std::endl;
  delete rm; 
  std::_Exit(0);
}
