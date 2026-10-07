# T6-H9 测试提示词

## A组：直接生成（原始需求）

# 任务 T6-H9 (Tier 3 高等复杂度 · T6 束流输运与复杂源项)

## 需求描述：
由多片环形金属散热翼片组成的复杂三维阵列。放射源从中心管道逸出，在多次阴影遮挡与几何间隙中穿透与散射，利用网格重要性分布计算最外侧探测器的接收角通量。

## 核心物理观测量：
- **目标观测量**：空间核动力系统环形排热辐射散热器复杂三维空间相空间视因子与杂散通量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T6-H9 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T6-H9 (Tier 3 高等复杂度 · T6 束流输运与复杂源项)

## 需求描述：
由多片环形金属散热翼片组成的复杂三维阵列。放射源从中心管道逸出，在多次阴影遮挡与几何间隙中穿透与散射，利用网格重要性分布计算最外侧探测器的接收角通量。

## 核心物理观测量：
- **目标观测量**：空间核动力系统环形排热辐射散热器复杂三维空间相空间视因子与杂散通量


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "fluence",
    "src": "U"
  },
  "F1b": {
    "v": "T6-H9 目标几何空间",
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
#include "G4UserRunAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4MagneticField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4ClassicalRK4.hh"
#include "G4Mag_UsualEqRhs.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T6-H9: 空间核动力环形排热系统相空间角系数视因子计算
// 组别: Arm A
// ============================================================================

static G4double gUniformCoolingTally = 0.0;

class T6H9Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matBe = nist->FindOrBuildMaterial("G4_Be");

    auto* worldSolid = new G4Box("WorldBox", 1.0 * m, 1.0 * m, 1.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 散热阵列几何 (外径 40 cm)
    auto* bodySolid = new G4Tubs("CylinderSolid", 10.0 * cm, 40.0 * cm, 20.0 * cm, 0, 360*deg);
    auto* bodyLog = new G4LogicalVolume(bodySolid, matBe, "CylinderLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), bodyLog, "CylinderPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T6H9Physics : public G4VModularPhysicsList {
public:
  T6H9Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H9Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(100.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(11.0 * cm, 0, 0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H9SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "CylinderPhys") return;

    // Arm A/B: 仅简化各向同性均匀向外辐射，未建模相邻环状翼片三维视角相互遮挡
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gUniformCoolingTally += edep;
    }

  }
};

class T6H9RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H9] Cooling geometry evaluated." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H9Detector());
  runManager->SetUserInitialization(new T6H9Physics());
  runManager->SetUserAction(new T6H9Generator());
  runManager->SetUserAction(new T6H9RunAction());
  runManager->SetUserAction(new T6H9SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H9] Completed" << std::endl;

  delete runManager;
  return 0;
}

```

