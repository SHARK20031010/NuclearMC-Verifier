# T5-H4 测试提示词

## A组：直接生成（原始需求）

# 任务 T5-H4 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
在超高瞬时剂量率（> 10^7 Gy/s）条件下，超高密度自由基促使 ·OH + ·OH -> H2O2 和 e_aq- + ·OH -> OH- 等双分子复合反应速率剧烈增加，导致有效扩散自由基产额降低。模拟自由基初生空间重叠度对终态自由基 G 值的非线性压制。

## 核心物理观测量：
- **目标观测量**：FLASH 超高剂量率下水溶液中水合电子 (e_aq-) 与自由基瞬态双分子自复合阻断模型


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T5-H4 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T5-H4 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
在超高瞬时剂量率（> 10^7 Gy/s）条件下，超高密度自由基促使 ·OH + ·OH -> H2O2 和 e_aq- + ·OH -> OH- 等双分子复合反应速率剧烈增加，导致有效扩散自由基产额降低。模拟自由基初生空间重叠度对终态自由基 G 值的非线性压制。

## 核心物理观测量：
- **目标观测量**：FLASH 超高剂量率下水溶液中水合电子 (e_aq-) 与自由基瞬态双分子自复合阻断模型


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
// 任务 T5-H4: 超高剂量率脉冲下自由基瞬态复合阻断动力学模拟
// 组别: Arm A
// ============================================================================

static G4double gStandardYield = 0.0;

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
    gun.SetParticleEnergy(10.0 * MeV); // 10 MeV 电子脉冲
    gun.SetParticlePosition(G4ThreeVector(0, 0, -1.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H4SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "WorldPhys") return;

    // Arm A/B: 常规剂量率线性产额假设，缺少超高剂量率下双分子自由基剧烈自复合压制
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gStandardYield += edep / (100.0 * eV) * 2.8;
    }

  }
};

class T5H4RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H4] Radiolysis simulation complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H4Detector());
  runManager->SetUserInitialization(new T5H4Physics());
  runManager->SetUserAction(new T5H4Generator());
  runManager->SetUserAction(new T5H4RunAction());
  runManager->SetUserAction(new T5H4SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H4] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

