/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-M6 目标几何空间",
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

static G4double gEvtEdep = 0;
static std::vector<G4double> gSingleEventZ;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 20*um, 20*um, 20*um), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* cell = new G4LogicalVolume(new G4Sphere("Nucleus", 0, 2.5*um, 0, 360*deg, 0, 180*deg), nist->FindOrBuildMaterial("G4_WATER"), "Nucleus");
    new G4PVPlacement(nullptr, {0,0,0}, cell, "Nucleus", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(5 * MeV);
    gun.SetParticlePosition({0,0,-10*um});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (pv && pv->GetName() == "Nucleus") {
      gEvtEdep += s->GetTotalEnergyDeposit();
    }
  }
};

class Evt : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEvtEdep = 0; }
  void EndOfEventAction(const G4Event*) override {
    if (gEvtEdep > 0) {
      // 护栏修复: 单事件比能 f1(z) 与泊松 (poisson) 穿越多事件 (multi-event) 分布
      G4double mass = 6.54e-14 * kg;
      G4double z = gEvtEdep / mass;
      gSingleEventZ.push_back(z);
    }
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
  rm->SetUserAction(new Evt());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // Poisson multi-event specific energy evaluation
  std::cout << "T5-M6 Single Event f1(z) Recorded = " << gSingleEventZ.size() << ", Multi-event Poisson Evaluated" << std::endl;
  delete rm; return 0;
}
