/*
```guardrail-intent
{
  "F1a": {
    "v": "efficiency",
    "src": "U"
  },
  "F1b": {
    "v": "T4-H5 目标几何空间",
    "src": "U"
  },
  "F2": {
    "v": "absolute",
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
// 任务 T4-H5: 中子伽马双探测有机闪烁体脉冲形状甄别
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalFastCharge = 0.0;
static G4double gTotalSlowCharge = 0.0;
static G4long gNeutronEvents = 0;
static G4long gGammaEvents = 0;

class T4H5Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matScint = nist->BuildMaterialWithNewDensity("PlasticScint_Mat", "G4_POLYSTYRENE", 1.03 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 30.0 * cm, 30.0 * cm, 30.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // EJ-299 / EJ-276 有机闪烁体 (Scint)
    auto* ejSolid = new G4Tubs("Scint_Solid", 0.0 * cm, 2.5 * cm, 2.5 * cm, 0, 360*deg);
    auto* ejLog = new G4LogicalVolume(ejSolid, matScint, "Scint_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), ejLog, "Scint_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H5Physics : public G4VModularPhysicsList {
public:
  T4H5Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H5Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    if (G4UniformRand() < 0.5) {
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton")); // 反冲质子 (中子信号)
      gun.SetParticleEnergy(1.5 * MeV);
    } else {
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-")); // 康普顿电子 (伽马信号)
      gun.SetParticleEnergy(500.0 * keV);
    }
    gun.SetParticlePosition(G4ThreeVector(0, 0, -5.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H5SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "Scint_Phys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    G4double stepLen = aStep->GetStepLength();
    if (edep <= 1.0 * keV || stepLen <= 0.0) return;

    // Birks' law 淬灭模型与快/慢发光成分时域积分比脉冲形状甄别 (PSD)
    G4double dEdx = edep / stepLen;
    G4double kB_Birks = 0.0126 * cm / MeV; // Birks 常数
    G4double quenched_yield = edep / (1.0 + kB_Birks * dEdx);

    G4String pName = aStep->GetTrack()->GetDefinition()->GetParticleName();
    G4double fastFraction = 0.85;
    G4double slowFraction = 0.15; // 慢成分/尾部积分 (slow / tail)

    if (pName == "proton") {
      // 强电离重带电粒子激发高三重态密度，慢发光比例增加 (PSD 增大)
      fastFraction = 0.60;
      slowFraction = 0.40;
    }

    G4double q_fast = quenched_yield * fastFraction;
    G4double q_slow = quenched_yield * slowFraction; // tail/slow component
    gTotalFastCharge += q_fast;
    gTotalSlowCharge += q_slow;

    // 计算脉冲形状甄别因子 PSD = slow / (fast + slow)
    G4double psd_parameter = q_slow / (q_fast + q_slow);
    if (psd_parameter > 0.25) {
      gNeutronEvents++;
    } else {
      gGammaEvents++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H5Detector());
  runManager->SetUserInitialization(new T4H5Physics());
  runManager->SetUserAction(new T4H5Generator());
  runManager->SetUserAction(new T4H5SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H5] Scintillator PSD completed. Neutrons: " << gNeutronEvents 
            << " Gammas: " << gGammaEvents << std::endl;

  delete runManager;
  std::_Exit(0);
}
