# T5-H1 测试提示词

## A组：直接生成（原始需求）

# 任务 T5-H1 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
单能质子入射细胞核。在物理阶段使用 Geant4-DNA 追踪离散逐次电离与激发；在化学阶段追踪自由基在皮秒至微秒内的扩散重组反应；统计 2 nm 双螺旋 DNA 上的直接电离断裂与 ·OH 间接攻击断裂，输出复杂双链断裂簇（Cluster DSB）产额。

## 核心物理观测量：
- **目标观测量**：质子布拉格峰区纳米尺度 DNA 复杂断裂 (DSB 聚集簇) 逐事件径迹结构与化学自由基复合


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T5-H1 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T5-H1 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
单能质子入射细胞核。在物理阶段使用 Geant4-DNA 追踪离散逐次电离与激发；在化学阶段追踪自由基在皮秒至微秒内的扩散重组反应；统计 2 nm 双螺旋 DNA 上的直接电离断裂与 ·OH 间接攻击断裂，输出复杂双链断裂簇（Cluster DSB）产额。

## 核心物理观测量：
- **目标观测量**：质子布拉格峰区纳米尺度 DNA 复杂断裂 (DSB 聚集簇) 逐事件径迹结构与化学自由基复合


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
    "v": "T5-H1 目标几何空间",
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
// 任务 T5-H1: 质子布拉格峰区纳米尺度复杂断裂簇逐事件径迹结构模拟
// 组别: Arm A
// ============================================================================

static G4double gVoxelAbsorbedDose = 0.0;
static G4long gTotalIonizations = 0;

class T5H1Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 1.0 * um, 1.0 * um, 1.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 纳米尺度靶区几何 (直径 2 nm 圆柱模拟双螺旋纤维)
    auto* tgtSolid = new G4Tubs("TgtSolid", 0.0 * nm, 1.0 * nm, 50.0 * nm, 0, 360*deg);
    auto* tgtLog = new G4LogicalVolume(tgtSolid, water, "TgtLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tgtLog, "BioCellPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H1Physics : public G4VModularPhysicsList {
public:
  T5H1Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H1Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.0 * MeV); // 布拉格峰高 LET 区质子
    gun.SetParticlePosition(G4ThreeVector(0, 0, -100.0 * nm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H1SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "BioCellPhys") return;

    // Arm A/B: 仅宏观水体微元均匀吸收剂量连续积分，缺少纳米尺度 DNA 离散双螺旋与复杂断裂簇
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gVoxelAbsorbedDose += edep;
      gTotalIonizations++;
    }

  }
};

class T5H1RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    std::cout << "[T5-H1] Starting microdosimetric simulation run..." << std::endl;
  }
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H1] Run finished successfully." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H1Detector());
  runManager->SetUserInitialization(new T5H1Physics());
  runManager->SetUserAction(new T5H1Generator());
  runManager->SetUserAction(new T5H1RunAction());
  runManager->SetUserAction(new T5H1SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H1] Execution Completed" << std::endl;

  delete runManager;
  return 0;
}

```

