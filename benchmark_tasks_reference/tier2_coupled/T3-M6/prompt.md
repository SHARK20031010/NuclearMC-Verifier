# T3-M6 测试提示词

## A组：直接生成（原始需求）

# 任务 T3-M6 (Tier 2 中等复杂度 · T3 活化与衰变链)

## 需求描述：
18F 物理半衰期 109.8 分钟，注射入人体后在膀胱和脑部浓聚，同时随尿液排泄（生物清除半衰期 2 小时）。计算在物理衰变与生物排泄双重机制下的膀胱壁累积吸收剂量。

## 核心物理观测量：
- **目标观测量**：人体注射 18F-FDG 脏器代谢与物理衰变联合清除剂量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T3-M6 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T3-M6 (Tier 2 中等复杂度 · T3 活化与衰变链)

## 需求描述：
18F 物理半衰期 109.8 分钟，注射入人体后在膀胱和脑部浓聚，同时随尿液排泄（生物清除半衰期 2 小时）。计算在物理衰变与生物排泄双重机制下的膀胱壁累积吸收剂量。

## 核心物理观测量：
- **目标观测量**：人体注射 18F-FDG 脏器代谢与物理衰变联合清除剂量


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
    "v": "T3-M6 目标几何空间",
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

static G4double gPromptDose = 0;
class Det6A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* organ = new G4Sphere("Bladder", 0, 4*cm, 0, 360*deg, 0, 180*deg);
    auto* organLog = new G4LogicalVolume(organ, nist->FindOrBuildMaterial("G4_WATER"), "Bladder");
    new G4PVPlacement(nullptr, {}, organLog, "Bladder", worldLog, false, 0);
    return worldPV;
  }
};
class Phys6A : public G4VModularPhysicsList {
public:
  Phys6A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim6A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e+"));
    gun.SetParticleEnergy(0.633*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gPromptDose += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6A()); rm->SetUserInitialization(new Phys6A());
  rm->SetUserAction(new Prim6A()); rm->SetUserAction(new Step6A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M6 Arm A] Prompt Dose = " << gPromptDose << std::endl;
  delete rm; return 0;
}

```

