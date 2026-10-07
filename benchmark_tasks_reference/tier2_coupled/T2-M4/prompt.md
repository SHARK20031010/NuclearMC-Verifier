# T2-M4 测试提示词

## A组：直接生成（原始需求）

# 任务 T2-M4 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
6 MV 伽马光子经直径 5 mm 的微型钨圆锥准直器准直后照射水箱。计算光野半影区（Penumbra 80%-20% 跌落距离）和输出因子（Output Factor），分箱精度 0.2 mm。

## 核心物理观测量：
- **目标观测量**：小野立体定向放射外科 (SRS) 锥光束离轴剂量跌落


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T2-M4 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T2-M4 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
6 MV 伽马光子经直径 5 mm 的微型钨圆锥准直器准直后照射水箱。计算光野半影区（Penumbra 80%-20% 跌落距离）和输出因子（Output Factor），分箱精度 0.2 mm。

## 核心物理观测量：
- **目标观测量**：小野立体定向放射外科 (SRS) 锥光束离轴剂量跌落


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
    "v": "微型圆锥准直器水箱半影区与输出因子",
    "src": "U"
  },
  "F2": {
    "v": "absorbed",
    "src": "U"
  },
  "F3": {
    "v": "Gy",
    "src": "A"
  },
  "F4": {
    "v": "distribution",
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
    "v": "curve",
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

static G4double gCoarseDose = 0;
class Det4A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* tankSolid = new G4Box("Tank", 20*cm, 20*cm, 20*cm);
    auto* tankLog = new G4LogicalVolume(tankSolid, nist->FindOrBuildMaterial("G4_WATER"), "Tank");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,20*cm), tankLog, "Tank", worldLog, false, 0);
    return worldPV;
  }
};
class Phys4A : public G4VModularPhysicsList {
public:
  Phys4A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim4A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(22));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step4A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gCoarseDose += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det4A()); rm->SetUserInitialization(new Phys4A());
  rm->SetUserAction(new Prim4A()); rm->SetUserAction(new Step4A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M4 Arm A] Coarse Dose = " << gCoarseDose << std::endl;
  delete rm; return 0;
}

```

