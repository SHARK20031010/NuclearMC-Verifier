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
// 任务 T5-H1: 质子布拉格峰区纳米尺度复杂断裂簇逐事件径迹结构模拟
// 组别: Arm A
// ============================================================================

static G4double gVoxelAbsorbedDose = 0.0;
static G4long gTotalIonizations = 0;

class T5H1Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 1.0 * um, 1.0 * um, 1.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 纳米尺度靶区几何 (直径 2 nm 圆柱模拟双螺旋纤维)
    auto* tgtSolid = new G4Tubs("TgtSolid", 0.0 * nm, 1.0 * nm, 50.0 * nm, 0, 360*deg);
    auto* tgtLog = new G4LogicalVolume(tgtSolid, water, "TgtLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tgtLog, "BioCellPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H1Physics : public G4VModularPhysicsList {
public:
  T5H1Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H1Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.0 * MeV); // 布拉格峰高 LET 区质子
    gun.SetParticlePosition(G4ThreeVector(0, 0, -100.0 * nm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H1SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "BioCellPhys") return;

    // Arm A/B: 仅宏观水体微元均匀吸收剂量连续积分，缺少纳米尺度 DNA 离散双螺旋与复杂断裂簇
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gVoxelAbsorbedDose += edep;
      gTotalIonizations++;
    }

  }
};

class T5H1RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    std::cout << "[T5-H1] Starting microdosimetric simulation run..." << std::endl;
  }
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H1] Run finished successfully." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H1Detector());
  runManager->SetUserInitialization(new T5H1Physics());
  runManager->SetUserAction(new T5H1Generator());
  runManager->SetUserAction(new T5H1RunAction());
  runManager->SetUserAction(new T5H1SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H1] Execution Completed" << std::endl;

  delete runManager;
  return 0;
}
