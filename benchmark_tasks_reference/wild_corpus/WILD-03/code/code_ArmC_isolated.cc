// WILD-03 Fixed Code
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
#include <iostream>
#include <vector>

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");
    
    // 配置光学与闪烁/切伦科夫材料属性表 (Slot D4_MATERIAL_PROPERTIES)
    auto* mpt = new G4MaterialPropertiesTable();
    std::vector<G4double> photonEnergy = {2.0 * eV, 3.5 * eV};
    std::vector<G4double> rindex = {1.33, 1.34};
    std::vector<G4double> absorption = {50.0 * m, 50.0 * m};
    std::vector<G4double> scintComponent = {1.0, 1.0};
    
    mpt->AddProperty("RINDEX", photonEnergy.data(), rindex.data(), photonEnergy.size());
    mpt->AddProperty("ABSLENGTH", photonEnergy.data(), absorption.data(), photonEnergy.size());
    mpt->AddProperty("SCINTILLATIONCOMPONENT1", photonEnergy.data(), scintComponent.data(), photonEnergy.size());
    mpt->AddConstProperty("SCINTILLATIONYIELD", 500. / MeV);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 10. * ns);
    
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

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());
  pl->RegisterPhysics(new G4OpticalPhysics());
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 10);
  std::cout << "WILD-03 Simulation completed" << std::endl;
  delete rm;
  std::_Exit(0);
}
