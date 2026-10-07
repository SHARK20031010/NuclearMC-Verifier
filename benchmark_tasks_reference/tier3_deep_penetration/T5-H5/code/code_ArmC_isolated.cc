/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-H5 目标几何空间",
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
// 任务 T5-H5: 硼中子俘获治疗反冲带电粒子亚细胞微剂量学
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gCellMembraneDose = 0.0;
static G4double gCellNucleusDose = 0.0;
static G4long gNucleusHits = 0;

class T5H5Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * um, 20.0 * um, 20.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 细胞结构 (Cell): 细胞体外膜与细胞核 (Nucleus)
    auto* cellSolid = new G4Sphere("CellSphere", 0.0 * um, 8.0 * um, 0, 360*deg, 0, 180*deg);
    auto* cellLog = new G4LogicalVolume(cellSolid, water, "CellLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), cellLog, "CellPhys", worldLog, false, 1);

    auto* nucSolid = new G4Sphere("NucSphere", 0.0 * um, 4.0 * um, 0, 360*deg, 0, 180*deg);
    auto* nucLog = new G4LogicalVolume(nucSolid, water, "NucLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), nucLog, "CellNucleusPhys", cellLog, false, 2);

    return worldPhys;
  }
};

class T5H5Physics : public G4VModularPhysicsList {
public:
  T5H5Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H5Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    // 硼中子俘获治疗 (BNCT): 10B(n,alpha)7Li 反应产物反冲发射
    G4ParticleGun gun(1);
    gun.SetParticlePosition(G4ThreeVector(6.0 * um, 0, 0)); // 硼化合物定位于细胞膜附近

    // 发射 1.47 MeV alpha 粒子
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(1.47 * MeV);
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H5SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;
    G4String name = pre->GetName();

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep <= 0.0) return;

    // BNCT 亚细胞微剂量学: 区分细胞核 (nucleus) 与 细胞质/膜能量沉积
    if (name == "CellNucleusPhys") {
      gCellNucleusDose += edep;
      gNucleusHits++;
    } else if (name == "CellPhys") {
      gCellMembraneDose += edep;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H5Detector());
  runManager->SetUserInitialization(new T5H5Physics());
  runManager->SetUserAction(new T5H5Generator());
  runManager->SetUserAction(new T5H5SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H5] BNCT Subcellular Microdosimetry. Nucleus Edep: " 
            << gCellNucleusDose / keV << " keV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
