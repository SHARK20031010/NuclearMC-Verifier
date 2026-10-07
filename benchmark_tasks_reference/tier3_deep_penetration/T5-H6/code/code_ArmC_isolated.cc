/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-H6 目标几何空间",
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
// 任务 T5-H6: 环状生物分子水溶液辐射化学构象动力学
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gSupercoiledPlasmidFraction = 1.0;
static G4double gRelaxedPlasmidFraction = 0.0;
static G4double gLinearPlasmidFraction = 0.0;
static G4double gTotalSolutionDose = 0.0;

class T5H6Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * mm, 5.0 * mm, 5.0 * mm);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T5H6Physics : public G4VModularPhysicsList {
public:
  T5H6Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H6Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.25 * MeV); // 60Co 伽马辐射
    gun.SetParticlePosition(G4ThreeVector(0, 0, -2.0 * mm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H6SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 1.0 * keV) {
      gTotalSolutionDose += edep;

      // 质粒 DNA (plasmid) 构象转变动力学: 超螺旋 (supercoiled) -> 开环 (relaxed) -> 线性 (linear)
      // 单链断裂 (SSB) 导致超螺旋松弛为开环 relaxed 构象；双链断裂 (DSB) 形成 linear 构象
      G4double k_ssb = 1.5e-3; // SSB 速率常数
      G4double k_dsb = 1.0e-4; // DSB 速率常数

      G4double delta_dose = edep / (1.0 * MeV);
      G4double d_relaxed = k_ssb * delta_dose * gSupercoiledPlasmidFraction;
      G4double d_linear = k_dsb * delta_dose * (gSupercoiledPlasmidFraction + gRelaxedPlasmidFraction);

      gSupercoiledPlasmidFraction -= (d_relaxed + d_linear);
      gRelaxedPlasmidFraction += (d_relaxed - d_linear);
      gLinearPlasmidFraction += d_linear;

      if (gSupercoiledPlasmidFraction < 0.0) gSupercoiledPlasmidFraction = 0.0;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H6Detector());
  runManager->SetUserInitialization(new T5H6Physics());
  runManager->SetUserAction(new T5H6Generator());
  runManager->SetUserAction(new T5H6SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H6] Plasmid Conformation: Supercoiled=" << gSupercoiledPlasmidFraction 
            << " Relaxed=" << gRelaxedPlasmidFraction 
            << " Linear=" << gLinearPlasmidFraction << std::endl;

  delete runManager;
  std::_Exit(0);
}
