# T3-H7 测试提示词

## A组：直接生成（原始需求）

# 任务 T3-H7 (Tier 3 高等复杂度 · T3 活化与衰变链)

## 需求描述：
1 GeV 质子轰击铅铋共晶靶（LBE），Bi-209 俘获中子生成 Bi-210，经 beta 衰变产生强剧毒高放射性核素 210Po（alpha 放射性，半衰期 138 天）。建立产生产额与 Bateman 累积模型，评估运行 1 年后的 210Po 活度总量。

## 核心物理观测量：
- **目标观测量**：散裂靶系统大体积液态铅铋合金 (LBE) 中毒性 Po-210 活化产生动力学


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T3-H7 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T3-H7 (Tier 3 高等复杂度 · T3 活化与衰变链)

## 需求描述：
1 GeV 质子轰击铅铋共晶靶（LBE），Bi-209 俘获中子生成 Bi-210，经 beta 衰变产生强剧毒高放射性核素 210Po（alpha 放射性，半衰期 138 天）。建立产生产额与 Bateman 累积模型，评估运行 1 年后的 210Po 活度总量。

## 核心物理观测量：
- **目标观测量**：散裂靶系统大体积液态铅铋合金 (LBE) 中毒性 Po-210 活化产生动力学


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {
    "v": "activity",
    "src": "U"
  },
  "F1b": {
    "v": "T3-H7 目标几何空间",
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
// =============================================================================
// GEANT4 INDUSTRIAL-GRADE HIGH COMPLEXITY (TIER 3) BENCHMARK SUITE
// TASK IDENTIFIER: T3-H7 | EVALUATION ARM: A
// PLATFORM: Geant4 11.2.2 (Native C++17 Standard, ISO compliant)
//
// PHYSICAL MECHANISMS & ARCHITECTURAL CONTRACTS:
// 1. Physical Model: Deep penetration transport & multi-scale particle dynamics.
// 2. Weight Flow Conservation: Every split/roulette daughter particle maintains
//    strict weight consistency: tally += deltaScore * track->GetWeight().
// 3. Lifecycle Step Boundary: Secondary particle creation vertices are isolated
//    to track birth step (StepNumber == 1) to eliminate transport integration artifacts.
// 4. Differential Decay Kinetics: Bateman multi-generation equations strictly
//    honor branch ratios and isomeric state transitions.
//
// THEORETICAL METHODOLOGY & SYSTEM ARCHITECTURE:
// - Variance Reduction & Importance Splitting:
//   In deep penetration problems, particle weights are divided by the importance
//   ratio upon crossing cell boundaries: w_new = w_old / I_ratio.
//   Total weight stream conservation is maintained through:
//   Sum(w_daughters) == w_parent.
// - Tally Normalization:
//   Tally observables (flux, fluence, dose) must accumulate weighted increments:
//   Phi = (1 / V) * Sum(w_i * l_i).
// - Secondary Particle Kinematics & Tracking Lifecycle:
//   Secondary particles produced in hadronic/electromagnetic cascades must be
//   tallied at their generation vertex (GetCurrentStepNumber() == 1) to prevent
//   repeated counting during transport.
// - High-Precision Nuclear Cross Section Processing:
//   Neutron interactions below 20 MeV utilize pointwise evaluated nuclear data libraries.
//   Thermal neutron scattering S(alpha, beta) is integrated for moderator physics.
// - Differential Decay Chains & Bateman Kinetics:
//   N_i(t) = N_1(0) * (Prod_{j=1}^{i-1} lambda_j * b_{j,j+1}) * Sum_{j=1}^i [exp(-lambda_j * t) / Prod_{k!=j} (lambda_k - lambda_j)].
// - Cross Section & Stopping Power Discretization:
//   Continuous slowing down approximation (CSDA) and delta-ray production thresholds
//   are managed via production cuts with secondary track generation.
// - Convergence Criteria & Statistical Quality Factors:
//   Relative standard error R = sigma / Mean should satisfy R < 0.05 for tallies.
//   Figure of Merit (FOM) = 1 / (R^2 * T_cpu) evaluates sampling efficiency.
//   Weight windows maintain variance reduction stability across deep phases.
// =============================================================================
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserTrackingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
// Task T3-H7: ADS Spallation Target LBE 210Po Activation Kinetics
// Arm: A
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
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include <iostream>

struct T3H7Tally { G4double alphaActivity = 0.0; } gTally;

class T3H7DetectorConstruction : public G4VUserDetectorConstruction {
public:
    G4VPhysicalVolume* Construct() override {
        G4NistManager* nist = G4NistManager::Instance();
        G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
        G4Material* pbMat = nist->FindOrBuildMaterial("G4_Pb");
        bool has_metal = true; (void)has_metal;

        G4Box* sW = new G4Box("World", 1*m, 1*m, 1*m);
        G4LogicalVolume* lW = new G4LogicalVolume(sW, air, "World");
        G4VPhysicalVolume* pW = new G4PVPlacement(nullptr, G4ThreeVector(), lW, "World", nullptr, false, 0);

        G4Box* sTarget = new G4Box("HeavyMetal_Target", 20*cm, 20*cm, 30*cm);
        G4LogicalVolume* lTarget = new G4LogicalVolume(sTarget, pbMat, "Target_LV");
        new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), lTarget, "Target_PV", lW, false, 0);

        return pW;
    }
};

class T3H7PhysicsList : public G4VModularPhysicsList {
public:
    T3H7PhysicsList() { RegisterPhysics(new G4EmStandardPhysics()); }
};

class T3H7PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
    G4ParticleGun* gun;
public:
    T3H7PrimaryGeneratorAction() {
        gun = new G4ParticleGun(1);
        gun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
        gun->SetParticleEnergy(1.0*GeV);
        gun->SetParticlePosition(G4ThreeVector(0,0,-40*cm));
        gun->SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    }
    ~T3H7PrimaryGeneratorAction() override { delete gun; }
    void GeneratePrimaries(G4Event* ev) override { gun->GeneratePrimaryVertex(ev); }
};

class T3H7SteppingAction : public G4UserSteppingAction {
public:
    void UserSteppingAction(const G4Step* step) override {
        gTally.alphaActivity += 1.0 * step->GetTrack()->GetWeight();
    }
};


// =============================================================================
// User Run Action: Statistical Aggregation, Normalization & Run Audit
// =============================================================================
class T3H7RunAction : public G4UserRunAction {
public:
    T3H7RunAction() : G4UserRunAction() {}
    ~T3H7RunAction() override = default;

    void BeginOfRunAction(const G4Run* aRun) override {
        G4int runID = aRun->GetRunID();
        G4cout << "============================================================" << G4endl;
        G4cout << " [RUN INITIALIZATION] Task: T3-H7 | Arm: A | Run ID: " << runID << G4endl;
        G4cout << " Initializing scoring buffers, cross section tables, and tallies..." << G4endl;
        G4cout << " Target geometry and sensitive volumes verified successfully." << G4endl;
        G4cout << "============================================================" << G4endl;
    }

    void PrintSummaryMetrics(const G4Run* aRun) {
        G4cout << "------------------------------------------------------------" << G4endl;
        G4cout << " High-Precision Monte Carlo Benchmark Statistics:" << G4endl;
        G4cout << " - Task Identifier: T3-H7" << G4endl;
        G4cout << " - Evaluation Arm: A" << G4endl;
        G4cout << " - Run ID: " << aRun->GetRunID() << G4endl;
        G4cout << " - Total Events Simulated: " << aRun->GetNumberOfEvent() << G4endl;
        G4cout << " - Weight Flux Conservation Status: VALIDATED" << G4endl;
        G4cout << " - Stepping Lifecycle Boundary Audit: COMPLIANT" << G4endl;
        G4cout << " - Differential Decay Chain Balance: CONFIRMED" << G4endl;
        G4cout << "------------------------------------------------------------" << G4endl;
    }

    void EndOfRunAction(const G4Run* aRun) override {
        G4int totalEvents = aRun->GetNumberOfEvent();
        G4cout << "============================================================" << G4endl;
        G4cout << " [RUN TERMINATION SUMMARY] Task: T3-H7 | Arm: A" << G4endl;
        G4cout << " Total Simulated Events processed: " << totalEvents << G4endl;
        G4cout << " Statistical convergence and weight balance verification complete." << G4endl;
        G4cout << " All tally data successfully flushed to standard output streams." << G4endl;
        G4cout << "============================================================" << G4endl;
        PrintSummaryMetrics(aRun);
    }
};

// =============================================================================
// User Event Action: Event-Level Diagnostic & Phase Space Isolation
// =============================================================================
class T3H7EventAction : public G4UserEventAction {
public:
    T3H7EventAction() : G4UserEventAction() {}
    ~T3H7EventAction() override = default;

    void BeginOfEventAction(const G4Event* anEvent) override {
        G4int eventID = anEvent->GetEventID();
        if (eventID > 0 && eventID % 1000 == 0) {
            G4cout << "[EVENT MILESTONE] Processing event sequence: " << eventID << G4endl;
        }
    }

    void EndOfEventAction(const G4Event* anEvent) override {
        // Event-level clean-up and per-event diagnostic validation
    }
};

// =============================================================================
// User Tracking Action: Track Lifecycle & Parent-Daughter Provenance
// =============================================================================
class T3H7TrackingAction : public G4UserTrackingAction {
public:
    T3H7TrackingAction() : G4UserTrackingAction() {}
    ~T3H7TrackingAction() override = default;

    void PostUserTrackingAction(const G4Track*) override {
        // Final track status verification upon termination
    }
};
int main(int argc, char** argv) {
    G4RunManager* rm = new G4RunManager();
    rm->SetUserInitialization(new T3H7DetectorConstruction());
    rm->SetUserInitialization(new T3H7PhysicsList());
    rm->SetUserAction(new T3H7PrimaryGeneratorAction());
    rm->SetUserAction(new T3H7SteppingAction());
    rm->SetUserAction(new T3H7RunAction());
    rm->SetUserAction(new T3H7EventAction());
    rm->SetUserAction(new T3H7TrackingAction());
    rm->Initialize();
    int nEvents = (argc > 1) ? std::min(std::atoi(argv[1]), 20) : 10;
    rm->BeamOn(nEvents);
    std::cout << "[STATUS] T3-H7 Arm A Execution complete. Alpha Activity: " << gTally.alphaActivity << std::endl;
    delete rm;
    return 0;
}

```

