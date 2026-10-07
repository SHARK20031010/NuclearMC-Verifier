# T2-M10 测试提示词

## A组：直接生成（原始需求）

# 任务 T2-M10 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
290 MeV/u 碳离子束照射头颈部水幻体，在侧向 90 度和 45 度角距离靶点 50 cm 处放置探测器，评估由靶核和弹核碎裂产生的杂散中子对正常组织的非靶剂量。

## 核心物理观测量：
- **目标观测量**：重离子束治疗中的出射次级中子与伽马侧向散射杂散剂量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T2-M10 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T2-M10 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
290 MeV/u 碳离子束照射头颈部水幻体，在侧向 90 度和 45 度角距离靶点 50 cm 处放置探测器，评估由靶核和弹核碎裂产生的杂散中子对正常组织的非靶剂量。

## 核心物理观测量：
- **目标观测量**：重离子束治疗中的出射次级中子与伽马侧向散射杂散剂量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "dose",
    "src": "U"
  },
  "F1b": {
    "v": "侧向 45/90 度角探测器杂散中子剂量",
    "src": "U"
  },
  "F2": {
    "v": "equivalent",
    "src": "U"
  },
  "F3": {
    "v": "Sv",
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
    "no_sievert",
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
#include "G4Cons.hh"
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

static G4double gCentralDose = 0;
class Det10A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 3*m, 3*m, 3*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* phantomSolid = new G4Box("Phantom", 15*cm, 15*cm, 20*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "Phantom");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,20*cm), phantomLog, "Phantom", worldLog, false, 0);
    return worldPV;
  }
};
class Phys10A : public G4VModularPhysicsList {
public:
  Phys10A() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim10A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("GenericIon"));
    gun.SetParticleCharge(6);
    gun.SetParticleEnergy(290.0*MeV * 12);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step10A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gCentralDose += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det10A()); rm->SetUserInitialization(new Phys10A());
  rm->SetUserAction(new Prim10A()); rm->SetUserAction(new Step10A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M10 Arm A] Central Dose = " << gCentralDose << std::endl;
  delete rm; return 0;
}

```

