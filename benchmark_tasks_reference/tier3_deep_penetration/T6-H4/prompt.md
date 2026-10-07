# T6-H4 测试提示词

## A组：直接生成（原始需求）

# 任务 T6-H4 (Tier 3 高等复杂度 · T6 束流输运与复杂源项)

## 需求描述：
高能质子垂直进入地球大气层（大气密度随海拔呈指数分布）。模拟初级核反应激发的多代强子级联与 pi0 衰变引发的电磁级联簇射，统计大气层不同气压深度处带电粒子数（纵向簇射剖面 Gaisser-Hillas 分布）。

## 核心物理观测量：
- **目标观测量**：超高能宇宙线（10^15 eV）在地球大气层中引发广泛空气簇射 (EAS) 纵向发展曲线


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T6-H4 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T6-H4 (Tier 3 高等复杂度 · T6 束流输运与复杂源项)

## 需求描述：
高能质子垂直进入地球大气层（大气密度随海拔呈指数分布）。模拟初级核反应激发的多代强子级联与 pi0 衰变引发的电磁级联簇射，统计大气层不同气压深度处带电粒子数（纵向簇射剖面 Gaisser-Hillas 分布）。

## 核心物理观测量：
- **目标观测量**：超高能宇宙线（10^15 eV）在地球大气层中引发广泛空气簇射 (EAS) 纵向发展曲线


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "fluence",
    "src": "U"
  },
  "F1b": {
    "v": "T6-H4 目标几何空间",
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

## 你此前初步生成的 Geant4 基线代码 (Arm A 缺陷代码)：
```cpp
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
#include "G4Run.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4MagneticField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4ClassicalRK4.hh"
#include "G4Mag_UsualEqRhs.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T6-H4: 超高能宇宙线大气层广延空气簇射纵向发展模拟
// 组别: Arm A
// ============================================================================

static G4double gUniformColumnEdep = 0.0;

class T6H4Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * km, 5.0 * km, 20.0 * km);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T6H4Physics : public G4VModularPhysicsList {
public:
  T6H4Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H4Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.0e5 * GeV); // 100 TeV 超高能宇宙线
    gun.SetParticlePosition(G4ThreeVector(0, 0, 15.0 * km));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H4SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "WorldPhys") return;

    // Arm A: 常密度空气均匀柱，缺少大气密度随海拔指数衰减与纵向广延大气簇射纵剖面
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gUniformColumnEdep += edep;
    }

  }
};

class T6H4RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H4] Uniform column simulation completed." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H4Detector());
  runManager->SetUserInitialization(new T6H4Physics());
  runManager->SetUserAction(new T6H4Generator());
  runManager->SetUserAction(new T6H4RunAction());
  runManager->SetUserAction(new T6H4SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H4] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

