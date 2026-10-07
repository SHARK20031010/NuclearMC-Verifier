/*
```guardrail-intent
{
  "F1a": {
    "v": "efficiency",
    "src": "U"
  },
  "F1b": {
    "v": "T4-H10 目标几何空间",
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
// 任务 T4-H10: 复合介质闪烁发光探测
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gEscapedLightPhotons = 0.0;
static G4long gScintPlateHits = 0;

class T4H10Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    // 6LiF / ZnS(Ag) 复合微米晶粒介质 (LiF / ZnS micro-grain phosphor)
    auto* matPlate = nist->BuildMaterialWithNewDensity("LiF_ZnS_CompositeScint", "G4_POLYSTYRENE", 2.8 * g/cm3);
    matPlate->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* screenSolid = new G4Box("ScreenSolid_ZnS", 5.0 * cm, 5.0 * cm, 0.1 * cm);
    auto* screenLog = new G4LogicalVolume(screenSolid, matPlate, "ScreenLog_LiF_ZnS");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), screenLog, "ScreenPhys_ZnS", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H10Physics : public G4VModularPhysicsList {
public:
  T4H10Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H10Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(2.05 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -0.05 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H10SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "ScreenPhys_ZnS") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 5.0 * keV) {
      // 6LiF/ZnS(Ag) 非透明微米颗粒光传输与强自吸收自散射微观逃逸模型 (escape)
      G4double rawLight = edep / (25.0 * eV); // ZnS 晶粒初级产光

      // 微观微粒散射与深度自吸收衰减因子
      G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
      G4double depth_in_grain = 0.05 * cm - pos.z(); // 距表面深度
      G4double mu_scattering_grain = 40.0 / cm; // ZnS 颗粒散射与自吸收系数
      G4double escape_prob = std::exp(-mu_scattering_grain * depth_in_grain); // 逃逸概率 escape

      G4double escapedPhotons = rawLight * escape_prob;
      gEscapedLightPhotons += escapedPhotons;
      gScintPlateHits++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H10Detector());
  runManager->SetUserInitialization(new T4H10Physics());
  runManager->SetUserAction(new T4H10Generator());
  runManager->SetUserAction(new T4H10SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H10] ZnS LiF Escaped Light: " << gEscapedLightPhotons << std::endl;

  delete runManager;
  std::_Exit(0);
}
