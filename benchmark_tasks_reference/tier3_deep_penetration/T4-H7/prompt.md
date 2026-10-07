# T4-H7 测试提示词

## A组：直接生成（原始需求）

# 任务 T4-H7 (Tier 3 高等复杂度 · T4 探测器与符合)

## 需求描述：
256×256 硅像素探测器（像素尺寸 55 微米）。带电粒子穿越产生电荷云并在相邻像素扩散分享。模拟电荷扩散高斯模型，输出各激活像素的到达时间戳（Time of Arrival）与积分电荷量（Time over Threshold）。

## 核心物理观测量：
- **目标观测量**：像素化半导体探测器 (Timepix3) 纳秒级同时测能量 (ToT) 与时间戳 (ToA)


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T4-H7 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T4-H7 (Tier 3 高等复杂度 · T4 探测器与符合)

## 需求描述：
256×256 硅像素探测器（像素尺寸 55 微米）。带电粒子穿越产生电荷云并在相邻像素扩散分享。模拟电荷扩散高斯模型，输出各激活像素的到达时间戳（Time of Arrival）与积分电荷量（Time over Threshold）。

## 核心物理观测量：
- **目标观测量**：像素化半导体探测器 (Timepix3) 纳秒级同时测能量 (ToT) 与时间戳 (ToA)


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
    "v": "T4-H7 目标几何空间",
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
// 任务 T4-H7: 像素半导体纳秒级能量与到达时间响应
// 组别: Arm A
// ============================================================================

static G4double gMacroMatrixEdep = 0.0;
static G4long gMatrixHits = 0;

class T4H7Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matSi = nist->FindOrBuildMaterial("G4_Si");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* sensorSolid = new G4Box("Sensor_Solid", 7.0 * mm, 7.0 * mm, 0.15 * mm);
    auto* sensorLog = new G4LogicalVolume(sensorSolid, matSi, "Sensor_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), sensorLog, "SiliconMatrix_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H7Physics : public G4VModularPhysicsList {
public:
  T4H7Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H7Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("mu-"));
    gun.SetParticleEnergy(4.0 * GeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -5.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H7SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "SiliconMatrix_Phys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.5 * keV) {
      gMacroMatrixEdep += edep;
      gMatrixHits++;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H7Detector());
  runManager->SetUserInitialization(new T4H7Physics());
  runManager->SetUserAction(new T4H7Generator());
  runManager->SetUserAction(new T4H7SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H7] Done" << std::endl;

  delete runManager;
  return 0;
}

```

