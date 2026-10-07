# T5-H3 测试提示词

## A组：直接生成（原始需求）

# 任务 T5-H3 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
在微米球腔中统计质子和光子照射下微剂量比能单事件分布 f1(z)。基于双辐射作用微剂量模型，计算代表单径迹致死事件的线性系数 alpha 与双径迹亚致死损伤累积的二次项 beta，重构细胞存活率曲线。

## 核心物理观测量：
- **目标观测量**：微剂量学两阶段比能累积理论与辐射诱发细胞存活率曲线 (LQ 模型参数 alpha/beta) 计算


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T5-H3 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T5-H3 (Tier 3 高等复杂度 · T5 微剂量与化学)

## 需求描述：
在微米球腔中统计质子和光子照射下微剂量比能单事件分布 f1(z)。基于双辐射作用微剂量模型，计算代表单径迹致死事件的线性系数 alpha 与双径迹亚致死损伤累积的二次项 beta，重构细胞存活率曲线。

## 核心物理观测量：
- **目标观测量**：微剂量学两阶段比能累积理论与辐射诱发细胞存活率曲线 (LQ 模型参数 alpha/beta) 计算


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
    "v": "T5-H3 目标几何空间",
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
// 任务 T5-H3: 微剂量学比能累积理论与细胞存活率曲线模型
// 组别: Arm A
// ============================================================================

static G4double gMacroscopicAbsorbedDose = 0.0;

class T5H3Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * um, 5.0 * um, 5.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 1 微米组织等效微腔球
    auto* sphereSolid = new G4Sphere("MicroSphere", 0.0 * um, 0.5 * um, 0, 360*deg, 0, 180*deg);
    auto* sphereLog = new G4LogicalVolume(sphereSolid, water, "MicroSphereLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), sphereLog, "MicroSpherePhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H3Physics : public G4VModularPhysicsList {
public:
  T5H3Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H3Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(3.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -2.0 * um));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H3SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "MicroSpherePhys") return;

    // Arm A: 简单宏观吸收剂量累加，缺少微剂量比能分布单事件谱与双辐射作用细胞存活率参数推导
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gMacroscopicAbsorbedDose += edep;
    }

  }
};

class T5H3RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H3] Microdosimetric analysis finished." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H3Detector());
  runManager->SetUserInitialization(new T5H3Physics());
  runManager->SetUserAction(new T5H3Generator());
  runManager->SetUserAction(new T5H3RunAction());
  runManager->SetUserAction(new T5H3SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H3] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

