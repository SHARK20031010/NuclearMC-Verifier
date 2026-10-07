# T3-M4 测试提示词

## A组：直接生成（原始需求）

# 任务 T3-M4 (Tier 2 中等复杂度 · T3 活化与衰变链)

## 需求描述：
Na-24 衰变后瞬间（皮秒内）发射 1.369 MeV 和 2.754 MeV 级联双伽马光子。在井型 NaI 探测器中模拟该级联过程，观察并统计 4.123 MeV 全吸收相加峰的计数与单峰面积之比。

## 核心物理观测量：
- **目标观测量**：24Na 衰变级联双光子真符合相加（True Coincidence Summing）峰


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T3-M4 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T3-M4 (Tier 2 中等复杂度 · T3 活化与衰变链)

## 需求描述：
Na-24 衰变后瞬间（皮秒内）发射 1.369 MeV 和 2.754 MeV 级联双伽马光子。在井型 NaI 探测器中模拟该级联过程，观察并统计 4.123 MeV 全吸收相加峰的计数与单峰面积之比。

## 核心物理观测量：
- **目标观测量**：24Na 衰变级联双光子真符合相加（True Coincidence Summing）峰


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
    "v": "T3-M4 目标几何空间",
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

static G4long gSingleHits = 0;
class Det4A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* naiSolid = new G4Tubs("NaIDet", 0, 4*cm, 4*cm, 0, 360*deg);
    auto* naiLog = new G4LogicalVolume(naiSolid, nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "NaIDet");
    new G4PVPlacement(nullptr, {}, naiLog, "NaIDet", worldLog, false, 0);
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
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.369*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step4A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTotalEnergyDeposit() > 1.0*MeV) gSingleHits++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det4A()); rm->SetUserInitialization(new Phys4A());
  rm->SetUserAction(new Prim4A()); rm->SetUserAction(new Step4A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M4 Arm A] Hits = " << gSingleHits << std::endl;
  delete rm; return 0;
}

```

