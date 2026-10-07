# T5-H10 测试提示词

## A组：直接生成（原始需求）

# 任务 T5-H10 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
高电荷态重离子（Au^54+）掠入射纳米水团簇。极短时间内剧烈剥离大量轨道电子形成高密度正电荷中心，模拟离散电离点产生的初始强静电势能分布与瞬态能量密度峰值。

## 核心物理观测量：
- **目标观测量**：高能金离子照射下纳米团簇库仑爆炸 (Coulomb Explosion) 初级微观能量转移


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T5-H10 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T5-H10 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
高电荷态重离子（Au^54+）掠入射纳米水团簇。极短时间内剧烈剥离大量轨道电子形成高密度正电荷中心，模拟离散电离点产生的初始强静电势能分布与瞬态能量密度峰值。

## 核心物理观测量：
- **目标观测量**：高能金离子照射下纳米团簇库仑爆炸 (Coulomb Explosion) 初级微观能量转移


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
    "v": "T5-H10 目标几何空间",
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
// 任务 T5-H10: 纳米团簇瞬态静电能量转移模拟
// 组别: Arm A
// ============================================================================

static G4double gStoppingLoss = 0.0;

class T5H10Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 100.0 * nm, 100.0 * nm, 100.0 * nm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 5 nm 纳米水团簇
    auto* clusSolid = new G4Sphere("ClusSolid", 0.0 * nm, 5.0 * nm, 0, 360*deg, 0, 180*deg);
    auto* clusLog = new G4LogicalVolume(clusSolid, water, "ClusLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), clusLog, "ClusPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H10Physics : public G4VModularPhysicsList {
public:
  T5H10Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H10Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    // 高电荷态重离子模拟
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(40.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -20.0 * nm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H10SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "ClusPhys") return;

    // Arm A/B: 仅常规电子阻止本领，缺少高密度正电荷中心强静电势能蓄积与爆炸释放
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gStoppingLoss += edep;
    }

  }
};

class T5H10RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H10] Energy transfer complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H10Detector());
  runManager->SetUserInitialization(new T5H10Physics());
  runManager->SetUserAction(new T5H10Generator());
  runManager->SetUserAction(new T5H10RunAction());
  runManager->SetUserAction(new T5H10SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H10] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

