// WILD-09 Isolated Physics Fixed Code
// Topic: Thermal neutron spectrum in water moderator does not Maxwellianize
// Fix: Use standard NIST material "G4_WATER" so that G4ThermalNeutrons can match
//      the S(alpha, beta) thermal neutron scattering data ("h_water") for bound Hydrogen.

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
#include <iostream>

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    // Using standard NIST material "G4_WATER" allows G4ThermalNeutrons / G4ParticleHPThermalScattering
    // to identify the material and bind the S(alpha, beta) thermal scattering cross section (h_water).
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
  return 0;
}
