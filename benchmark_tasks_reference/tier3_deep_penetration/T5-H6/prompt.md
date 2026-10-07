# T5-H6 测试提示词

## A组：直接生成（原始需求）

# 任务 T5-H6 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
pBR322 超螺旋环状质粒 DNA 溶解于纯水中。模拟伽马射线电离水产生的自由基对质粒双链的攻击，根据单链断裂（转换为开环 Relaxed 构象）和双链断裂（转换为线性 Linear 构象）的发生概率计算构象转化百分比。

## 核心物理观测量：
- **目标观测量**：DNA 质粒环状结构在水溶液中受伽马照射向开环与线状断裂转化的辐射化学动力学


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T5-H6 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T5-H6 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
pBR322 超螺旋环状质粒 DNA 溶解于纯水中。模拟伽马射线电离水产生的自由基对质粒双链的攻击，根据单链断裂（转换为开环 Relaxed 构象）和双链断裂（转换为线性 Linear 构象）的发生概率计算构象转化百分比。

## 核心物理观测量：
- **目标观测量**：DNA 质粒环状结构在水溶液中受伽马照射向开环与线状断裂转化的辐射化学动力学


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
// 任务 T5-H6: 环状生物分子水溶液辐射化学构象动力学
// 组别: Arm A
// ============================================================================

static G4double gDoseToSolution = 0.0;

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
    if (!pre || pre->GetName() != "WorldPhys") return;

    // Arm A/B: 仅统计全液相均匀吸收剂量，缺少质粒构象转化动力学
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gDoseToSolution += edep;
    }

  }
};

class T5H6RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H6] Conformation kinetics done." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H6Detector());
  runManager->SetUserInitialization(new T5H6Physics());
  runManager->SetUserAction(new T5H6Generator());
  runManager->SetUserAction(new T5H6RunAction());
  runManager->SetUserAction(new T5H6SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H6] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

