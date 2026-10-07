// WILD-06 Buggy Code (CERN Forum Post)
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
    mpt->AddProperty("RINDEX", en, rindex);
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000./MeV);
    scMat->SetMaterialPropertiesTable(mpt);
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
  delete rm; return 0;
}
