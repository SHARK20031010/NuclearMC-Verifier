# T4-H1 测试提示词

## A组：直接生成（原始需求）

# 任务 T4-H1 (Tier 3 高等复杂度 · T4 探测器与符合)

## 需求描述：
由两块 30×30 矩阵式 LYSO 闪烁阵列（厚度 20 mm）构成的 TOF-PET 探测器。考虑光子在晶体内相互作用深度（DOI）引起的光传输时延，加入时间游走（Time Walk）校正算法，在 300 ps 时间符合窗内筛选湮灭光子对，输出重构飞行时间差谱。

## 核心物理观测量：
- **目标观测量**：全数字化 TOF-PET 双探头 LYSO 晶体深度依赖 (DOI) 纳秒符合与时间游走校正


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T4-H1 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T4-H1 (Tier 3 高等复杂度 · T4 探测器与符合)

## 需求描述：
由两块 30×30 矩阵式 LYSO 闪烁阵列（厚度 20 mm）构成的 TOF-PET 探测器。考虑光子在晶体内相互作用深度（DOI）引起的光传输时延，加入时间游走（Time Walk）校正算法，在 300 ps 时间符合窗内筛选湮灭光子对，输出重构飞行时间差谱。

## 核心物理观测量：
- **目标观测量**：全数字化 TOF-PET 双探头 LYSO 晶体深度依赖 (DOI) 纳秒符合与时间游走校正


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
    "v": "T4-H1 目标几何空间",
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
// 任务 T4-H1: 全数字 TOF-PET 双探头 LYSO 深度依赖 (DOI) 纳秒符合与时间游走
// 组别: Arm A
// ============================================================================

static G4double gTotalEdep = 0.0;
static G4long gCoincidences = 0;
static std::map<std::string, G4double> gHitTimes;

class T4H1Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matLYSO = nist->BuildMaterialWithNewDensity("LYSO_Mat", "G4_LUCITE", 7.1 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 50.0 * cm, 50.0 * cm, 50.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* lysoSolid = new G4Box("LYSO_Solid", 10.0 * cm, 10.0 * cm, 1.0 * cm);
    auto* lysoLogL = new G4LogicalVolume(lysoSolid, matLYSO, "LYSO_LogL");
    auto* lysoLogR = new G4LogicalVolume(lysoSolid, matLYSO, "LYSO_LogR");

    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, -15.0 * cm), lysoLogL, "LYSO_L", worldLog, false, 1);
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 15.0 * cm), lysoLogR, "LYSO_R", worldLog, false, 2);

    return worldPhys;
  }
};

class T4H1Physics : public G4VModularPhysicsList {
public:
  T4H1Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H1Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(511.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
    gun.GeneratePrimaryVertex(anEvent);
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H1SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;
    std::string volName = pre->GetName();
    if (volName != "LYSO_L" && volName != "LYSO_R") return;


    G4double hitTime = aStep->GetPostStepPoint()->GetLocalTime();
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.1 * MeV) {
      gHitTimes[volName] = hitTime;
      gTotalEdep += edep;
    }

  }
};

class T4H1EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gHitTimes.clear();
  }
  void EndOfEventAction(const G4Event*) override {
    if (gHitTimes.count("LYSO_L") && gHitTimes.count("LYSO_R")) {
      G4double dt = std::abs(gHitTimes["LYSO_L"] - gHitTimes["LYSO_R"]);
      if (dt < 0.3 * ns) {
        gCoincidences++;
      }
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H1Detector());
  runManager->SetUserInitialization(new T4H1Physics());
  runManager->SetUserAction(new T4H1Generator());
  runManager->SetUserAction(new T4H1EventAction());
  runManager->SetUserAction(new T4H1SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H1] Coincidences: " << gCoincidences 
            << " TotalEdep: " << gTotalEdep / MeV << " MeV" << std::endl;

  delete runManager;
  return 0;
}

```

