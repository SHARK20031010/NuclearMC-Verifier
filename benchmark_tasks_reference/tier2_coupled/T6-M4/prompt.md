# T6-M4 测试提示词

## A组：直接生成（原始需求）

# 任务 T6-M4 (Tier 2 中等复杂度 · T6 束流输运与复杂源项)

## 需求描述：
模拟标称能量 1 MeV、能展 FWHM = 800 keV 的宽能展电子束。采用非负截断抽样循环防止高斯长尾产生负动能，记录出射能谱并验证运动学自洽性。

## 核心物理观测量：
- **目标观测量**：高斯能展束流抽样动能非负物理截断 Ek > 0 边界保护


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T6-M4 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T6-M4 (Tier 2 中等复杂度 · T6 束流输运与复杂源项)

## 需求描述：
模拟标称能量 1 MeV、能展 FWHM = 800 keV 的宽能展电子束。采用非负截断抽样循环防止高斯长尾产生负动能，记录出射能谱并验证运动学自洽性。

## 核心物理观测量：
- **目标观测量**：高斯能展束流抽样动能非负物理截断 Ek > 0 边界保护


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "fluence",
    "src": "U"
  },
  "F1b": {
    "v": "T6-M4 目标几何空间",
    "src": "U"
  },
  "F2": {
    "v": "count",
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
#include "G4Event.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

static G4double gMeanEk = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 50*cm, 50*cm, 50*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    G4double E0 = 1.0 * MeV;
    G4double sigma = 0.5 * MeV;
    G4double sampledE = G4RandGauss::shoot(E0, sigma);
    gun.SetParticleEnergy(sampledE);
    gun.SetParticlePosition({0,0,-10*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
    gMeanEk += sampledE;
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step*) override {}
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "T6-M4 Baseline Mean Ek = " << gMeanEk/50/MeV << " MeV" << std::endl;
  delete rm; return 0;
}

```

