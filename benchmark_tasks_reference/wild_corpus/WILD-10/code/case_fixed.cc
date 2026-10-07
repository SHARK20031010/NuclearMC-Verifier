// WILD-10 Fixed Code
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
#include "G4UniformMagField.hh"
#include "G4FieldManager.hh"
#include "G4ChordFinder.hh"
#include "G4TransportationManager.hh"
#include <iostream>

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 2*m, 2*m, 2*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* magVol = new G4LogicalVolume(new G4Box("Dipole", 50*cm, 50*cm, 50*cm), nist->FindOrBuildMaterial("G4_AIR"), "Dipole");
    auto* magField = new G4UniformMagField(G4ThreeVector(0, 1.5*tesla, 0));
    auto* fieldMgr = new G4FieldManager(magField);
    // 修复：创建 ChordFinder 并显式限制最大弦长容差为 0.1 mm
    fieldMgr->CreateChordFinder(magField);
    fieldMgr->GetChordFinder()->SetDeltaChord(0.1 * mm);
    magVol->SetFieldManager(fieldMgr, true);
    new G4PVPlacement(nullptr, {0,0,0}, magVol, "Dipole", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(100.0 * MeV);
    gun.SetParticlePosition({0,0,-20*cm});
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
  std::cout << "WILD-10 Dipole transport completed" << std::endl;
  delete rm; return 0;
}
