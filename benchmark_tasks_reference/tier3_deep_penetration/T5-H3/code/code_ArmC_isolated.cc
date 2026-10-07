/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-H3 目标几何空间",
    "src": "U"
  },
  "F2": {
    "v": "y",
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
*/

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
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T5-H3: 微剂量学比能累积理论与细胞存活率曲线模型
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gSpecificEnergyZ_Sum = 0.0;
static G4double gSpecificEnergyZ2_Sum = 0.0;
static G4long gMicroEventCount = 0;
static G4double gSurvivalFraction = 1.0;

class T5H3Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * um, 5.0 * um, 5.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 1 微米微剂量学敏感体积球 (MicroSphere, d = 1 um)
    auto* sphereSolid = new G4Sphere("MicroSphere", 0.0 * um, 0.5 * um, 0, 360*deg, 0, 180*deg);
    auto* sphereLog = new G4LogicalVolume(sphereSolid, water, "MicroSphereLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), sphereLog, "MicroSpherePhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H3Physics : public G4VModularPhysicsList {
public:
  T5H3Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H3Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(3.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -2.0 * um));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H3SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "MicroSpherePhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      // 微剂量比能 z = epsilon / m
      G4double r_site = 0.5 * um;
      G4double volume = (4.0 / 3.0) * M_PI * std::pow(r_site, 3);
      G4double rho = 1.0 * g / cm3;
      G4double mass = rho * volume;
      G4double z = edep / mass; // 单步微剂量比能 z (Gray)

      gSpecificEnergyZ_Sum += z;
      gSpecificEnergyZ2_Sum += z * z;
      gMicroEventCount++;

      // 双辐射作用理论 (TDRA) 与线性二次 (LQ) 模型细胞存活率 (survival fraction)
      G4double alpha_0 = 0.2; // Gy^-1
      G4double beta_val = 0.05; // Gy^-2 (beta)
      G4double D = gSpecificEnergyZ_Sum / gray;
      G4double alpha_eff = alpha_0 + beta_val * (gSpecificEnergyZ2_Sum / (gSpecificEnergyZ_Sum + 1e-12));
      gSurvivalFraction = std::exp(-alpha_eff * D - beta_val * D * D); // LQ survival curve
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H3Detector());
  runManager->SetUserInitialization(new T5H3Physics());
  runManager->SetUserAction(new T5H3Generator());
  runManager->SetUserAction(new T5H3SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H3] Microdosimetry LQ model. Hits: " << gMicroEventCount 
            << " Survival: " << gSurvivalFraction << std::endl;

  delete runManager;
  std::_Exit(0);
}
