/*
```guardrail-intent
{
  "F1a": {
    "v": "efficiency",
    "src": "U"
  },
  "F1b": {
    "v": "T4-H9 目标几何空间",
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
// 任务 T4-H9: 双层符合运动学几何探测器
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gScatterLayerEdep = 0.0;
static G4double gAbsorbLayerEdep = 0.0;
static G4long gComptonCameraCoincidences = 0;

class T4H9Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matSi = nist->FindOrBuildMaterial("G4_Si");
    auto* matCZT = nist->BuildMaterialWithNewDensity("HeavyCrystal_Mat", "G4_CADMIUM_TUNGSTATE", 5.8 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 40.0 * cm, 40.0 * cm, 40.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 康普顿相机双散射层: 散射层 (Scatterer) 与 吸收层 (Absorber)
    auto* l1Solid = new G4Box("ScatterSolid", 10.0 * cm, 10.0 * cm, 0.2 * cm);
    auto* l1Log = new G4LogicalVolume(l1Solid, matSi, "ScatterLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,5.0*cm), l1Log, "FirstPlatePhys_Scatter", worldLog, false, 1);

    auto* l2Solid = new G4Box("AbsorbSolid", 12.0 * cm, 12.0 * cm, 1.0 * cm);
    auto* l2Log = new G4LogicalVolume(l2Solid, matCZT, "AbsorbLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15.0*cm), l2Log, "SecondPlatePhys_Absorb", worldLog, false, 2);

    return worldPhys;
  }
};

class T4H9Physics : public G4VModularPhysicsList {
public:
  T4H9Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H9Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(662.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H9SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;
    G4String vol = pre->GetName();

    G4double edep = aStep->GetTotalEnergyDeposit() * aStep->GetTrack()->GetWeight();
    if (vol == "FirstPlatePhys_Scatter") {
      gScatterLayerEdep += edep;
    } else if (vol == "SecondPlatePhys_Absorb") {
      gAbsorbLayerEdep += edep;
    }
  }
};

class T4H9EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gScatterLayerEdep = 0.0;
    gAbsorbLayerEdep = 0.0;
  }
  void EndOfEventAction(const G4Event*) override {
    // 康普顿相机双散射层 (Scatter & Absorb) 双符合与运动学圆锥重构 (kinematics cone)
    if (gScatterLayerEdep > 10.0 * keV && gAbsorbLayerEdep > 50.0 * keV) {
      G4double E1 = gScatterLayerEdep; // 散射层能量
      G4double E2 = gAbsorbLayerEdep;  // 吸收层能量
      G4double E0 = E1 + E2;           // 重构初级入射光子能量
      G4double m_e = 511.0 * keV;

      // 康普顿角散射运动学方程: cos(theta) = 1 - m_e * (1/E2 - 1/E0)
      G4double cos_theta = 1.0 - m_e * (1.0 / E2 - 1.0 / E0);
      if (cos_theta >= -1.0 && cos_theta <= 1.0) {
        gComptonCameraCoincidences++; // 成功重构 Compton cone
      }
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H9Detector());
  runManager->SetUserInitialization(new T4H9Physics());
  runManager->SetUserAction(new T4H9Generator());
  runManager->SetUserAction(new T4H9EventAction());
  runManager->SetUserAction(new T4H9SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H9] Compton Camera Kinematics Coincidences: " 
            << gComptonCameraCoincidences << std::endl;

  delete runManager;
  std::_Exit(0);
}
