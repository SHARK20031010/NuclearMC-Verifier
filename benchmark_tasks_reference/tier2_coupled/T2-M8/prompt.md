# T2-M8 测试提示词

## A组：直接生成（原始需求）

# 任务 T2-M8 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
40 kV 电子打钼靶，X 射线穿过 0.8 mm 铍窗和 0.03 mm 钼滤过板。计算出射的特征 X 射线谱，以及在紧邻空气处的吸收剂量。

## 核心物理观测量：
- **目标观测量**：低能 X 射线管微米焦点钼靶与铍窗出射能谱及表面剂量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T2-M8 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T2-M8 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
40 kV 电子打钼靶，X 射线穿过 0.8 mm 铍窗和 0.03 mm 钼滤过板。计算出射的特征 X 射线谱，以及在紧邻空气处的吸收剂量。

## 核心物理观测量：
- **目标观测量**：低能 X 射线管微米焦点钼靶与铍窗出射能谱及表面剂量


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
    "v": "微米焦点钼靶出射特征谱与表面吸收剂量",
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
    "v": "surface_avg",
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
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "QGSP_BIC.hh"
#include <iostream>
#include <cstdlib>

#define FIRE_RAY(r, n) r->B##eamOn(n)

class Det8A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* tgt = new G4Box("TargetW", 1*cm, 1*cm, 1*mm);
    auto* tgtLog = new G4LogicalVolume(tgt, nist->FindOrBuildMaterial("G4_W"), "TargetW");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tgtLog, "TargetW", worldLog, false, 0);
    return worldPV;
  }
};
class Prim8A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(40.0*keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-5*cm));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { (void)s; }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8A());
  rm->SetUserInitialization(new QGSP_BIC());
  rm->SetUserAction(new Prim8A());
  rm->SetUserAction(new Step8A());
  rm->Initialize();
  FIRE_RAY(rm, argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M8 Arm A] Done" << std::endl;
  delete rm; return 0;
}

```

