/*
```guardrail-intent
{
  "F1a": {
    "v": "microdosimetry",
    "src": "U"
  },
  "F1b": {
    "v": "T5-H9 目标几何空间",
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
// 任务 T5-H9: 碳纳米管微结构低能电子传输微剂量学模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gSecondaryElectronYield = 0.0;
static G4long gNanotubeScatteringHits = 0;

class T5H9Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matCarbon = nist->BuildMaterialWithNewDensity("CNT_Carbon", "G4_GRAPHITE", 1.8 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 1.0 * um, 1.0 * um, 1.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 碳纳米管 (Carbon Nanotube, CNT) 中空通道: 内径 2 nm, 外径 3 nm, 长 400 nm
    auto* tubeSolid = new G4Tubs("PoreTube", 2.0 * nm, 3.0 * nm, 200.0 * nm, 0, 360*deg);
    auto* tubeLog = new G4LogicalVolume(tubeSolid, matCarbon, "PoreLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tubeLog, "NanotubePhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H9Physics : public G4VModularPhysicsList {
public:
  T5H9Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H9Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(500.0 * eV); // 低能轰击电子
    gun.SetParticlePosition(G4ThreeVector(0, 0, -300.0 * nm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H9SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "NanotubePhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 1.0 * eV) {
      // 碳纳米管 (nanotube / CNT) 微通道中多重碰撞反射与真二次电子发射 (secondary SE emission)
      G4double se_work_function = 4.5 * eV;
      G4double delta_SE = (edep / se_work_function) * 0.15; // 真二次电子产额
      gSecondaryElectronYield += delta_SE;
      gNanotubeScatteringHits++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H9Detector());
  runManager->SetUserInitialization(new T5H9Physics());
  runManager->SetUserAction(new T5H9Generator());
  runManager->SetUserAction(new T5H9SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H9] Carbon Nanotube Secondary Electron Yield: " 
            << gSecondaryElectronYield << std::endl;

  delete runManager;
  std::_Exit(0);
}
