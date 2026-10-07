# T5-H7 测试提示词

## A组：直接生成（原始需求）

# 任务 T5-H7 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
纳米级油包水微乳液液滴（直径 20 nm）。带电粒子在液滴内部产生初生自由基，油水界面表现为疏水反射边界。模拟自由基在受限微空间内的限制扩散动力学及分子寿命延长效应。

## 核心物理观测量：
- **目标观测量**：纳秒脉冲辐射诱发超微受限微乳液液滴内部自由基扩散与界面反射阻挡


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T5-H7 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T5-H7 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
纳米级油包水微乳液液滴（直径 20 nm）。带电粒子在液滴内部产生初生自由基，油水界面表现为疏水反射边界。模拟自由基在受限微空间内的限制扩散动力学及分子寿命延长效应。

## 核心物理观测量：
- **目标观测量**：纳秒脉冲辐射诱发超微受限微乳液液滴内部自由基扩散与界面反射阻挡


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
// 任务 T5-H7: 纳米受限微团自由基扩散动力学模拟
// 组别: Arm A
// ============================================================================

static G4double gVesicleEnergy = 0.0;

class T5H7Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");
    auto* oil = nist->BuildMaterialWithNewDensity("OilMatrix", "G4_POLYETHYLENE", 0.78 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 100.0 * nm, 100.0 * nm, 100.0 * nm);
    auto* worldLog = new G4LogicalVolume(worldSolid, oil, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 20 nm 直径水滴微囊
    auto* dropSolid = new G4Sphere("DropSolid", 0.0 * nm, 10.0 * nm, 0, 360*deg, 0, 180*deg);
    auto* dropLog = new G4LogicalVolume(dropSolid, water, "DropLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), dropLog, "DropPhys", worldLog, false, 1);

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
    if (!pre || pre->GetName() != "DropPhys") return;

    // Arm A/B: 仅开放边界自由扩散，未建模界面疏水反射阻挡效应
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gVesicleEnergy += edep;
    }

  }
};

class T5H7RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H7] Drop boundary tracking complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H7Detector());
  runManager->SetUserInitialization(new T5H7Physics());
  runManager->SetUserAction(new T5H7Generator());
  runManager->SetUserAction(new T5H7RunAction());
  runManager->SetUserAction(new T5H7SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H7] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

