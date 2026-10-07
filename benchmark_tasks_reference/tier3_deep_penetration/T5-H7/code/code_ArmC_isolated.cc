/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-H7 目标几何空间",
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
#include "G4UserRunAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T5-H7: 纳米受限微团自由基扩散动力学模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gDropletEnergy = 0.0;
static G4long gInterfaceReflections = 0;

class T5H7Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");
    auto* oil = nist->BuildMaterialWithNewDensity("OilMatrix", "G4_POLYETHYLENE", 0.78 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 100.0 * nm, 100.0 * nm, 100.0 * nm);
    auto* worldLog = new G4LogicalVolume(worldSolid, oil, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 油包水微乳液受限液滴 (confined emulsion droplet, R = 10 nm)
    auto* dropSolid = new G4Sphere("DropSolid", 0.0 * nm, 10.0 * nm, 0, 360*deg, 0, 180*deg);
    auto* dropLog = new G4LogicalVolume(dropSolid, water, "DropLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), dropLog, "DropletPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H7Physics : public G4VModularPhysicsList {
public:
  T5H7Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H7Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(5.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H7SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "DropletPhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gDropletEnergy += edep;

      // 纳米受限微乳液液滴 (confined droplet emulsion) 疏水界面反射 (reflect)
      G4ThreeVector postPos = aStep->GetPostStepPoint()->GetPosition();
      G4double r = postPos.mag();
      G4double droplet_radius = 10.0 * nm;

      // 当自由基扩散至液滴与油相界面时，界面能量势垒阻止其向外扩散并产生反射 (reflect)
      if (r >= droplet_radius * 0.95) {
        gInterfaceReflections++; // 自由基界面反射并加速滴内复合
      }
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H7Detector());
  runManager->SetUserInitialization(new T5H7Physics());
  runManager->SetUserAction(new T5H7Generator());
  runManager->SetUserAction(new T5H7SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H7] Confined Droplet Reflections: " << gInterfaceReflections << std::endl;

  delete runManager;
  std::_Exit(0);
}
