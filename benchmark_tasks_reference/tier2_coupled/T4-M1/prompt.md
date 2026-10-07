# T4-M1 测试提示词

## A组：直接生成（原始需求）

# 任务 T4-M1 (Tier 2 中等复杂度 · T4 探测器与符合)

## 需求描述：
中心湮灭点源发射一对背对背 511 keV 光子，两侧各放置一块 LYSO 闪烁晶体探测器。读取实验室全局时钟（GlobalTime），在 [450, 550] keV 能量窗和 |tA - tB| <= 5 ns 时间窗内筛选真符合事件。

## 核心物理观测量：
- **目标观测量**：PET 双探头 511 keV 纳秒时间符合窗与能量窗测量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T4-M1 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T4-M1 (Tier 2 中等复杂度 · T4 探测器与符合)

## 需求描述：
中心湮灭点源发射一对背对背 511 keV 光子，两侧各放置一块 LYSO 闪烁晶体探测器。读取实验室全局时钟（GlobalTime），在 [450, 550] keV 能量窗和 |tA - tB| <= 5 ns 时间窗内筛选真符合事件。

## 核心物理观测量：
- **目标观测量**：PET 双探头 511 keV 纳秒时间符合窗与能量窗测量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "efficiency",
    "src": "U"
  },
  "F1b": {
    "v": "T4-M1 目标几何空间",
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
#include "G4Event.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4PhysicalConstants.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

static G4long gCoincidences = 0;
static G4double gEdepA = 0.0, gEdepB = 0.0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 50*cm, 50*cm, 50*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* detA = new G4LogicalVolume(new G4Box("DetA", 2.5*cm, 2.5*cm, 1*cm), nist->FindOrBuildMaterial("G4_WATER"), "DetA");
    auto* detB = new G4LogicalVolume(new G4Box("DetB", 2.5*cm, 2.5*cm, 1*cm), nist->FindOrBuildMaterial("G4_WATER"), "DetB");
    new G4PVPlacement(nullptr, {-15*cm, 0, 0}, detA, "DetA", world, false, 0);
    new G4PVPlacement(nullptr, { 15*cm, 0, 0}, detB, "DetB", world, false, 1);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(511 * keV);
    gun.SetParticlePosition({0,0,0});
    gun.SetParticleMomentumDirection({-1,0,0}); gun.GeneratePrimaryVertex(ev);
    gun.SetParticleMomentumDirection({1,0,0}); gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv) return;
    G4double t = s->GetPreStepPoint()->GetLocalTime(); // 缺陷1: 误用 GetLocalTime
    G4double edep = s->GetTotalEnergyDeposit();
    if (pv->GetName() == "DetA" && edep > 0) gEdepA += edep;
    if (pv->GetName() == "DetB" && edep > 0) gEdepB += edep;
    if (gEdepA > 450*keV && gEdepB > 450*keV) {
      gCoincidences++;
      gEdepA = gEdepB = 0;
    }
  }
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
  std::cout << "T4-M1 Coincidence Counts = " << gCoincidences << std::endl;
  delete rm; return 0;
}

```

