# T2-M6 测试提示词

## A组：直接生成（原始需求）

# 任务 T2-M6 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
眼球模型内植入包含 5 颗 125I 密封籽源的黄铜敷贴器（Eye Plaque）。计算眼球巩膜靶区和后极部视神经敏感区域的吸收剂量比。

## 核心物理观测量：
- **目标观测量**：眼眶黑色素瘤 125I 放射性粒子敷贴器巩膜与视神经剂量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T2-M6 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T2-M6 (Tier 2 中等复杂度 · T2 剂量与放疗物理)

## 需求描述：
眼球模型内植入包含 5 颗 125I 密封籽源的黄铜敷贴器（Eye Plaque）。计算眼球巩膜靶区和后极部视神经敏感区域的吸收剂量比。

## 核心物理观测量：
- **目标观测量**：眼眶黑色素瘤 125I 放射性粒子敷贴器巩膜与视神经剂量


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
    "v": "眼球巩膜靶区与视神经敏感区吸收剂量",
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

static G4double gTotalDose = 0;
class Det6A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* sphereSolid = new G4Sphere("WaterSphere", 0, 12*mm, 0, 360*deg, 0, 180*deg);
    auto* sphereLog = new G4LogicalVolume(sphereSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterSphere");
    new G4PVPlacement(nullptr, {}, sphereLog, "WaterSphere", worldLog, false, 0);
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
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(27.0*keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,10*mm));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gTotalDose += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6A()); rm->SetUserInitialization(new Phys6A());
  rm->SetUserAction(new Prim6A()); rm->SetUserAction(new Step6A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M6 Arm A] Total Dose = " << gTotalDose << std::endl;
  delete rm; return 0;
}

```

