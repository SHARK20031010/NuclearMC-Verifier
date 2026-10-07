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
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T5-H5: 硼中子俘获治疗反冲带电粒子亚细胞微剂量学
// 组别: Arm A
// ============================================================================

static G4double gAverageCellDose = 0.0;

class T5H5Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 50.0 * um, 50.0 * um, 50.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 细胞浆体 (半径 10 um)
    auto* cellSolid = new G4Sphere("CellSoma", 0.0 * um, 10.0 * um, 0, 360*deg, 0, 180*deg);
    auto* cellLog = new G4LogicalVolume(cellSolid, water, "CellLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), cellLog, "CellPhys", worldLog, false, 1);

    // 内部亚结构 (半径 4 um)
    auto* nucSolid = new G4Sphere("NucSphere", 0.0 * um, 4.0 * um, 0, 360*deg, 0, 180*deg);
    auto* nucLog = new G4LogicalVolume(nucSolid, water, "NucLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), nucLog, "InnerCorePhys", cellLog, false, 2);

    return worldPhys;
  }
};

class T5H5Physics : public G4VModularPhysicsList {
public:
  T5H5Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H5Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    // 模拟 10B 裂变反冲碎片 (1.47 MeV alpha 粒子)
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(1.47 * MeV);
    gun.SetParticlePosition(G4ThreeVector(2.0 * um, 0, 0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H5SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;

    // Arm A/B: 仅统计全细胞宏观平均吸收剂量，缺少细胞膜与细胞核亚细胞微剂量解耦
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gAverageCellDose += edep;
    }

  }
};

class T5H5RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H5] Recoil energy tally done." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H5Detector());
  runManager->SetUserInitialization(new T5H5Physics());
  runManager->SetUserAction(new T5H5Generator());
  runManager->SetUserAction(new T5H5RunAction());
  runManager->SetUserAction(new T5H5SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H5] Completed" << std::endl;

  delete runManager;
  return 0;
}
