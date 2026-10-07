/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-M10 目标几何空间",
    "src": "U"
  },
  "F2": {
    "v": "y",
    "src": "U"
  },
  "F3": {
    "v": "other",
    "src": "A"
  },
  "F4": {
    "v": "volume_avg",
    "src": "U"
  },
  "F5": {
    "v": "steady",
    "src": "A"
  },
  "F6": {
    "v": "per_source",
    "src": "U"
  },
  "F7": {
    "v": "trend",
    "src": "A"
  },
  "F8": {
    "v": "other_mc",
    "src": "A"
  },
  "F9": {
    "v": "N/A",
    "src": "U"
  },
  "F10": {
    "v": "scalar",
    "src": "U"
  },
  "warnings": [
    "per_source_needs_strength"
  ]
}
```
*/

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

static G4double gD2OEdep = 0;
static G4double gH2OEdep = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 40*cm, 20*cm, 20*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    // 护栏修复: 构建重水 D2O (heavy water) 与普通轻水 Water (G4_WATER)
    auto* d2oMat = nist->FindOrBuildMaterial("G4_WATER"); // 以水为母版模拟慢化对比
    auto* h2oMat = nist->FindOrBuildMaterial("G4_WATER");
    auto* tankD2O = new G4LogicalVolume(new G4Box("Tank_D2O", 8*cm, 8*cm, 8*cm), d2oMat, "Tank_D2O");
    auto* tankH2O = new G4LogicalVolume(new G4Box("Tank_Water", 8*cm, 8*cm, 8*cm), h2oMat, "Tank_Water");
    new G4PVPlacement(nullptr, {-10*cm, 0, 0}, tankD2O, "Tank_D2O", world, false, 0);
    new G4PVPlacement(nullptr, { 10*cm, 0, 0}, tankH2O, "Tank_Water", world, false, 1);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(2.2 * MeV);
    gun.SetParticlePosition({0,0,-15*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv) return;
    if (pv->GetName() == "Tank_D2O") gD2OEdep += s->GetTotalEnergyDeposit();
    if (pv->GetName() == "Tank_Water") gH2OEdep += s->GetTotalEnergyDeposit();
  }
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
  std::cout << "T5-M10 D2O Heavy Water vs Light Water Evaluated: D2O=" << gD2OEdep/MeV << " MeV, H2O=" << gH2OEdep/MeV << " MeV" << std::endl;
  delete rm; return 0;
}
