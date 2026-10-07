/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-H4 目标几何空间",
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
// 任务 T5-H4: 超高剂量率脉冲下自由基瞬态复合阻断动力学模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalFlashEdep = 0.0;
static G4double gNetEffectiveRadicalYield = 0.0;
static G4long gRecombinationEvents = 0;

class T5H4Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 2.0 * cm, 2.0 * cm, 2.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T5H4Physics : public G4VModularPhysicsList {
public:
  T5H4Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H4Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(10.0 * MeV); // 10 MeV FLASH 超高剂量率电子脉冲
    gun.SetParticlePosition(G4ThreeVector(0, 0, -1.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H4SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 1.0 * keV) {
      gTotalFlashEdep += edep;

      // FLASH 超高剂量率下瞬态双分子自由基自复合压制 (bimolecular radical recombination)
      // 辐射初级产额水化电子 e_aq 与羟基自由基 OH
      G4double standard_G_value = 2.8; // 经典线性 G-value (molecules / 100 eV)
      G4double primary_radicals = (edep / (100.0 * eV)) * standard_G_value;

      // 双分子复合反应速率与自由基局部浓度的平方成正比: e_aq + OH -> OH-
      G4double doseRate_local = 1.0e6; // FLASH 脉冲等效瞬时剂量率 (Gy/s)
      G4double recombination_fraction = 1.0 - 1.0 / (1.0 + 0.001 * doseRate_local);

      // 自复合消耗部分自由基，有效逃逸 G-value 下降
      G4double recombined_radicals = primary_radicals * recombination_fraction;
      G4double escaped_radicals = primary_radicals - recombined_radicals;

      gNetEffectiveRadicalYield += escaped_radicals;
      gRecombinationEvents++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H4Detector());
  runManager->SetUserInitialization(new T5H4Physics());
  runManager->SetUserAction(new T5H4Generator());
  runManager->SetUserAction(new T5H4SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H4] FLASH bimolecular recombination completed. Escaped: " 
            << gNetEffectiveRadicalYield << std::endl;

  delete runManager;
  std::_Exit(0);
}
