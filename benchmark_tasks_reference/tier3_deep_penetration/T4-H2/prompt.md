# T4-H2 测试提示词

## A组：直接生成（原始需求）

# 任务 T4-H2 (Tier 3 高等复杂度 · T4 探测器与符合)

## 需求描述：
井型高纯锗探测器（探测效率接近 4pi）。放入 133Ba 或 152Eu 复杂多伽马级联衰变源。模拟级联光子在同一事件中同时沉积能量引起的真符合相加（TCS），计算各单能特征峰面积的符合丢失因子与和峰产生比。

## 核心物理观测量：
- **目标观测量**：超低本底 HPGe 宽能能谱仪高灵敏井型探测器真符合求和 (TCS) 校正因子


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T4-H2 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T4-H2 (Tier 3 高等复杂度 · T4 探测器与符合)

## 需求描述：
井型高纯锗探测器（探测效率接近 4pi）。放入 133Ba 或 152Eu 复杂多伽马级联衰变源。模拟级联光子在同一事件中同时沉积能量引起的真符合相加（TCS），计算各单能特征峰面积的符合丢失因子与和峰产生比。

## 核心物理观测量：
- **目标观测量**：超低本底 HPGe 宽能能谱仪高灵敏井型探测器真符合求和 (TCS) 校正因子


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "efficiency",
    "src": "U"
  },
  "F1b": {
    "v": "T4-H2 目标几何空间",
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
// 任务 T4-H2: 超低本底 HPGe 井型探测器真符合求和校正
// 组别: Arm A
// ============================================================================

static G4double gTotalEdep = 0.0;
static G4double gEventEdep = 0.0;
static G4long gSinglePhotons = 0;

class T4H2Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matGe = nist->FindOrBuildMaterial("G4_Ge");

    auto* worldSolid = new G4Box("WorldBox", 30.0 * cm, 30.0 * cm, 30.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* hpgeSolid = new G4Tubs("HPGe_Solid", 0.0 * mm, 35.0 * mm, 35.0 * mm, 0, 360*deg);
    auto* hpgeLog = new G4LogicalVolume(hpgeSolid, matGe, "HPGe_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), hpgeLog, "HPGe_DetectorPhys", worldLog, false, 1);

    auto* cavitySolid = new G4Tubs("CavitySolid", 0.0 * mm, 8.0 * mm, 20.0 * mm, 0, 360*deg);
    auto* cavityLog = new G4LogicalVolume(cavitySolid, air, "CentralBoreLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15.0*mm), cavityLog, "CentralBorePhys", hpgeLog, false, 2);

    return worldPhys;
  }
};

class T4H2Physics : public G4VModularPhysicsList {
public:
  T4H2Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H2Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {

    // Arm A/B: 单光子独立发射，未考虑级联衰变引起的真符合相加
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleEnergy(356.0 * keV);
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);

  }
};

class T4H2SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "HPGe_DetectorPhys") return;
    G4double edep = aStep->GetTotalEnergyDeposit();
    gEventEdep += edep;
    gTotalEdep += edep;
  }
};

class T4H2EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gEventEdep = 0.0;
  }
  void EndOfEventAction(const G4Event*) override {

    if (gEventEdep > 350.0 * keV) {
      gSinglePhotons++;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H2Detector());
  runManager->SetUserInitialization(new T4H2Physics());
  runManager->SetUserAction(new T4H2Generator());
  runManager->SetUserAction(new T4H2EventAction());
  runManager->SetUserAction(new T4H2SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H2] TotalEdep: " << gTotalEdep / keV << " keV" << std::endl;

  delete runManager;
  return 0;
}

```

