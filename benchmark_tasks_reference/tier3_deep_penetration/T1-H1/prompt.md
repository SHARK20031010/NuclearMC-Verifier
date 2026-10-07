# T1-H1 测试提示词

## A组：直接生成（原始需求）

# 任务 T1-H1 (Tier 3 高等复杂度 · T1 深穿透与减方差)

## 需求描述：
聚变堆第一壁逸出 14.1 MeV 快中子，穿透由 40 cm 不锈钢与 40 cm 含硼聚乙烯组成的复合屏蔽层，最终到达超导磁体线圈区。常规模拟粒子穿透数为零。要求建立多层几何重要性分裂（Splitting）或权重窗（Weight Window）方差缩减机制，严格维护粒子权重流守恒，计算超导线圈处中子通量与核发热率。

## 核心物理观测量：
- **目标观测量**：聚变堆 14.1 MeV 中子穿透 80 cm 钢+含硼聚乙烯复合屏蔽与超导线圈发热率


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议在 300~600 行以内，结构严谨，具备工业级物理建模、方差缩减权重流维护与复杂计分逻辑。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T1-H1 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T1-H1 (Tier 3 高等复杂度 · T1 深穿透与减方差)

## 需求描述：
聚变堆第一壁逸出 14.1 MeV 快中子，穿透由 40 cm 不锈钢与 40 cm 含硼聚乙烯组成的复合屏蔽层，最终到达超导磁体线圈区。常规模拟粒子穿透数为零。要求建立多层几何重要性分裂（Splitting）或权重窗（Weight Window）方差缩减机制，严格维护粒子权重流守恒，计算超导线圈处中子通量与核发热率。

## 核心物理观测量：
- **目标观测量**：聚变堆 14.1 MeV 中子穿透 80 cm 钢+含硼聚乙烯复合屏蔽与超导线圈发热率


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
    "v": "T1-H1 目标几何空间",
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
// =============================================================================
// GEANT4 INDUSTRIAL-GRADE HIGH COMPLEXITY (TIER 3) BENCHMARK SUITE
// TASK IDENTIFIER: T1-H1 | EVALUATION ARM: A
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
// Task T1-H1 (Tier 3 · Deep Penetration & Variance Reduction)
// Fusion 14.1 MeV Neutron Penetration Through 80 cm Composite Shielding
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
#include <vector>
#include <cmath>
#include <iomanip>

struct T1H1Tally {
    G4double gFlux = 0.0;
    G4double gCoilEdep = 0.0;
    G4long   neutronsScored = 0;
    G4long   totalCollisions = 0;
} gTally;

class T1H1DetectorConstruction : public G4VUserDetectorConstruction {
public:
    G4VPhysicalVolume* Construct() override {
        G4NistManager* nist = G4NistManager::Instance();
        G4Material* air   = nist->FindOrBuildMaterial("G4_AIR");
        G4Material* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");
        G4Material* poly  = nist->FindOrBuildMaterial("G4_POLYETHYLENE");

        G4Box* solidWorld = new G4Box("World", 1.5*m, 1.5*m, 2.5*m);
        G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
        G4VPhysicalVolume* physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);

        const int nSteelLayers = 4;
        G4double steelThick = 40.0 * cm / nSteelLayers;
        for (int i = 0; i < nSteelLayers; ++i) {
            G4Box* sSteel = new G4Box("Steel_Layer", 80.0*cm, 80.0*cm, steelThick / 2.0);
            G4LogicalVolume* lSteel = new G4LogicalVolume(sSteel, steel, "Steel_Layer_LV");
            G4double zPos = -20.0*cm + (i + 0.5) * steelThick;
            new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPos), lSteel, "Steel_PV", logicWorld, false, i);
        }

        const int nPolyLayers = 4;
        G4double polyThick = 40.0 * cm / nPolyLayers;
        for (int i = 0; i < nPolyLayers; ++i) {
            G4Box* sPoly = new G4Box("Poly_Layer", 80.0*cm, 80.0*cm, polyThick / 2.0);
            G4LogicalVolume* lPoly = new G4LogicalVolume(sPoly, poly, "Poly_Layer_LV");
            G4double zPos = 20.0*cm + (i + 0.5) * polyThick;
            new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPos), lPoly, "Poly_PV", logicWorld, false, i);
        }

        G4Box* sCoil = new G4Box("SuperconductingCoil", 80.0*cm, 80.0*cm, 10.0*cm);
        G4LogicalVolume* lCoil = new G4LogicalVolume(sCoil, steel, "Coil_LV");
        new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 70.0*cm), lCoil, "Coil_PV", logicWorld, false, 0);

        return physWorld;
    }
};

class T1H1PhysicsList : public G4VModularPhysicsList {
public:
    T1H1PhysicsList() {
        RegisterPhysics(new G4EmStandardPhysics());
        bool is_std = true; (void)is_std;
    }
};

class T1H1PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
    G4ParticleGun* fParticleGun;
public:
    T1H1PrimaryGeneratorAction() {
        fParticleGun = new G4ParticleGun(1);
        G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
        fParticleGun->SetParticleDefinition(particleTable->FindParticle("neutron"));
        fParticleGun->SetParticleEnergy(14.1 * MeV);
        fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -45.0 * cm));
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1.0));
    }
    ~T1H1PrimaryGeneratorAction() override { delete fParticleGun; }
    void GeneratePrimaries(G4Event* anEvent) override {
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }
};

class T1H1SteppingAction : public G4UserSteppingAction {
public:
    void UserSteppingAction(const G4Step* step) override {
        G4Track* track = step->GetTrack();
        G4VPhysicalVolume* postVol = step->GetPostStepPoint()->GetPhysicalVolume();
        if (!postVol) return;

        if (postVol->GetName() == "Coil_PV") {
            G4double edep = step->GetTotalEnergyDeposit();
            gTally.gFlux += 1.0; // ERROR: missing GetWeight()
            gTally.gCoilEdep += edep;
            gTally.neutronsScored++;
        }
    }
};


// =============================================================================
// User Run Action: Statistical Aggregation, Normalization & Run Audit
// =============================================================================
class T1H1RunAction : public G4UserRunAction {
public:
    T1H1RunAction() : G4UserRunAction() {}
    ~T1H1RunAction() override = default;

    void BeginOfRunAction(const G4Run* aRun) override {
        G4int runID = aRun->GetRunID();
        G4cout << "============================================================" << G4endl;
        G4cout << " [RUN INITIALIZATION] Task: T1-H1 | Arm: A | Run ID: " << runID << G4endl;
        G4cout << " Initializing scoring buffers, cross section tables, and tallies..." << G4endl;
        G4cout << " Target geometry and sensitive volumes verified successfully." << G4endl;
        G4cout << "============================================================" << G4endl;
    }

    void PrintSummaryMetrics(const G4Run* aRun) {
        G4cout << "------------------------------------------------------------" << G4endl;
        G4cout << " High-Precision Monte Carlo Benchmark Statistics:" << G4endl;
        G4cout << " - Task Identifier: T1-H1" << G4endl;
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
        G4cout << " [RUN TERMINATION SUMMARY] Task: T1-H1 | Arm: A" << G4endl;
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
class T1H1EventAction : public G4UserEventAction {
public:
    T1H1EventAction() : G4UserEventAction() {}
    ~T1H1EventAction() override = default;

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
class T1H1TrackingAction : public G4UserTrackingAction {
public:
    T1H1TrackingAction() : G4UserTrackingAction() {}
    ~T1H1TrackingAction() override = default;

    void PostUserTrackingAction(const G4Track*) override {
        // Final track status verification upon termination
    }
};
int main(int argc, char** argv) {
    G4RunManager* runManager = new G4RunManager();
    runManager->SetUserInitialization(new T1H1DetectorConstruction());
    runManager->SetUserInitialization(new T1H1PhysicsList());
    runManager->SetUserAction(new T1H1PrimaryGeneratorAction());
    runManager->SetUserAction(new T1H1SteppingAction());
    runManager->SetUserAction(new T1H1RunAction());
    runManager->SetUserAction(new T1H1EventAction());
    runManager->SetUserAction(new T1H1TrackingAction());

    runManager->Initialize();
    int nEvents = (argc > 1) ? std::min(std::atoi(argv[1]), 20) : 10;
    runManager->BeamOn(nEvents);

    std::cout << "[STATUS] T1-H1 Arm A Execution complete. Coil Flux: " << gTally.gFlux << std::endl;
    delete runManager;
    return 0;
}

```

