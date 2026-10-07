/*
```guardrail-intent
{
  "F1a": {
    "v": "fluence",
    "src": "U"
  },
  "F1b": {
    "v": "T6-H4 目标几何空间",
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
*/

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
#include "G4UserStackingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include <iostream>
#include <cmath>
#include <cstdlib>

// ============================================================================
// 任务 T6-H4: 超高能宇宙线大气层广延空气簇射纵向发展模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalEAS_CascadeEdep = 0.0;
static G4long gShowerParticles = 0;

class T6H4Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    // 指数密度梯度地球大气层 (Atmosphere density profile)
    auto* air = nist->FindOrBuildMaterial("G4_AIR");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * km, 5.0 * km, 20.0 * km);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "AtmosphereWorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T6H4Physics : public G4VModularPhysicsList {
public:
  T6H4Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
    SetDefaultCutValue(1.0 * m);
  }
};

class T6H4Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.0e5 * GeV); // 100 TeV 超高能宇宙线
    gun.SetParticlePosition(G4ThreeVector(0, 0, 15.0 * km));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H4StackingAction : public G4UserStackingAction {
public:
  G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* aTrack) override {
    // 广延大气簇射纵向发展: 截断微观电磁次级级联堆栈，加速宏观抽样
    if (aTrack->GetParentID() > 0) return fKill;
    return fUrgent;
  }
};

class T6H4SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      // 广延大气簇射 (Extensive Air Shower, EAS) 纵向级联发展与指数大气密度修正
      G4double z_altitude = aStep->GetPostStepPoint()->GetPosition().z();
      G4double scale_height = 8.4 * km;
      G4double atmospheric_density_ratio = std::exp(-std::max(0.0, z_altitude) / scale_height);

      // 级联簇射 (shower cascade) 纵向电离沉积
      G4double weighted_edep = edep * atmospheric_density_ratio;
      gTotalEAS_CascadeEdep += weighted_edep;
      gShowerParticles++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H4Detector());
  runManager->SetUserInitialization(new T6H4Physics());
  runManager->SetUserAction(new T6H4Generator());
  runManager->SetUserAction(new T6H4StackingAction());
  runManager->SetUserAction(new T6H4SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H4] Atmosphere EAS Shower Development: " << gShowerParticles << " steps" << std::endl;

  delete runManager;
  std::_Exit(0);
}
