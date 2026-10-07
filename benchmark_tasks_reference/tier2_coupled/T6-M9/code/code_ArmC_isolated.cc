/*
```guardrail-intent
{
  "F1a": {
    "v": "fluence",
    "src": "U"
  },
  "F1b": {
    "v": "T6-M9 目标几何空间",
    "src": "U"
  },
  "F2": {
    "v": "count",
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

static G4double gWeightedPhotons = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 50*cm, 50*cm, 50*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* target = new G4LogicalVolume(new G4Box("Target", 2*cm, 2*cm, 1*mm), nist->FindOrBuildMaterial("G4_W"), "Target");
    new G4PVPlacement(nullptr, {0,0,0}, target, "Target", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(6 * MeV);
    gun.SetParticlePosition({0,0,-5*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* track = s->GetTrack();
    if (track->GetParticleDefinition()->GetParticleName() == "gamma") {
      // 护栏修复: 定向轫致辐射分裂 (DBS split) 减方差技术，统计时乘入粒子统计权重 (GetWeight)
      G4double weight = track->GetWeight();
      if (s->GetPreStepPoint()->GetMomentumDirection().z() > 0.90) {
        gWeightedPhotons += weight;
      }
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
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "T6-M9 DBS Split Weight Tally Sampled, Weighted Sum = " << gWeightedPhotons << std::endl;
  delete rm; return 0;
}
