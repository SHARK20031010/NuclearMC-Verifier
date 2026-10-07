# T3-M3 测试提示词

## A组：直接生成（原始需求）

# 任务 T3-M3 (Tier 2 中等复杂度 · T3 活化与衰变链)

## 需求描述：
水反应堆堆芯产生 16O(n,p)16N 活化水（半衰期 7.13 s）。水流经长 15 米管道流动至主蒸汽室（流速 5 m/s）。计算流体输运时间延迟下管道外探测器测到的 6.13 MeV 伽马计数率。

## 核心物理观测量：
- **目标观测量**：反应堆一回路冷却剂 N-16 流体流动输运与蒸汽管道衰变测量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T3-M3 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T3-M3 (Tier 2 中等复杂度 · T3 活化与衰变链)

## 需求描述：
水反应堆堆芯产生 16O(n,p)16N 活化水（半衰期 7.13 s）。水流经长 15 米管道流动至主蒸汽室（流速 5 m/s）。计算流体输运时间延迟下管道外探测器测到的 6.13 MeV 伽马计数率。

## 核心物理观测量：
- **目标观测量**：反应堆一回路冷却剂 N-16 流体流动输运与蒸汽管道衰变测量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "activity",
    "src": "U"
  },
  "F1b": {
    "v": "T3-M3 目标几何空间",
    "src": "U"
  },
  "F2": {
    "v": "atoms",
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
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

static G4long gN16Counts = 0;
class Det3A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 20*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* pipeSolid = new G4Tubs("Pipe", 0, 10*cm, 10*m, 0, 360*deg);
    auto* pipeLog = new G4LogicalVolume(pipeSolid, nist->FindOrBuildMaterial("G4_WATER"), "Pipe");
    new G4PVPlacement(nullptr, {}, pipeLog, "Pipe", worldLog, false, 0);
    return worldPV;
  }
};
class Phys3A : public G4VModularPhysicsList {
public:
  Phys3A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim3A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.13*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step3A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 5*m) gN16Counts++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det3A()); rm->SetUserInitialization(new Phys3A());
  rm->SetUserAction(new Prim3A()); rm->SetUserAction(new Step3A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M3 Arm A] Static count = " << gN16Counts << std::endl;
  delete rm; return 0;
}

```

