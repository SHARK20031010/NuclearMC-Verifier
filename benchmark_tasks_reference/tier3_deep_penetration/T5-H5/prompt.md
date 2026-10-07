# T5-H5 测试提示词

## A组：直接生成（原始需求）

# 任务 T5-H5 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
热中子在含有 10B 的肿瘤细胞质与细胞核中发生裂解反应。产生 1.47 MeV alpha 粒子和 0.84 MeV 7Li 反冲核（射程仅 5~9 微米）。计算反冲带电粒子在细胞膜、胞质和细胞核内部的离散线能分布与微局部剂量。

## 核心物理观测量：
- **目标观测量**：硼中子俘获治疗 (BNCT) 中 10B(n,alpha)7Li 反冲核超高局部微剂量学线能与亚细胞沉积


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T5-H5 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T5-H5 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
热中子在含有 10B 的肿瘤细胞质与细胞核中发生裂解反应。产生 1.47 MeV alpha 粒子和 0.84 MeV 7Li 反冲核（射程仅 5~9 微米）。计算反冲带电粒子在细胞膜、胞质和细胞核内部的离散线能分布与微局部剂量。

## 核心物理观测量：
- **目标观测量**：硼中子俘获治疗 (BNCT) 中 10B(n,alpha)7Li 反冲核超高局部微剂量学线能与亚细胞沉积


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



## 前置参数确认 (Ring 0 Intent Confirmation)：
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
// 组别: Arm A
// ============================================================================

static G4double gAverageCellDose = 0.0;

class T5H5Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 50.0 * um, 50.0 * um, 50.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 细胞浆体 (半径 10 um)
    auto* cellSolid = new G4Sphere("CellSoma", 0.0 * um, 10.0 * um, 0, 360*deg, 0, 180*deg);
    auto* cellLog = new G4LogicalVolume(cellSolid, water, "CellLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), cellLog, "CellPhys", worldLog, false, 1);

    // 内部亚结构 (半径 4 um)
    auto* nucSolid = new G4Sphere("NucSphere", 0.0 * um, 4.0 * um, 0, 360*deg, 0, 180*deg);
    auto* nucLog = new G4LogicalVolume(nucSolid, water, "NucLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), nucLog, "InnerCorePhys", cellLog, false, 2);

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
    // 模拟 10B 裂变反冲碎片 (1.47 MeV alpha 粒子)
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(1.47 * MeV);
    gun.SetParticlePosition(G4ThreeVector(2.0 * um, 0, 0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H5SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;

    // Arm A/B: 仅统计全细胞宏观平均吸收剂量，缺少细胞膜与细胞核亚细胞微剂量解耦
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gAverageCellDose += edep;
    }

  }
};

class T5H5RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H5] Recoil energy tally done." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H5Detector());
  runManager->SetUserInitialization(new T5H5Physics());
  runManager->SetUserAction(new T5H5Generator());
  runManager->SetUserAction(new T5H5RunAction());
  runManager->SetUserAction(new T5H5SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H5] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

