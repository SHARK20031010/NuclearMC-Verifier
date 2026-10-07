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
// 任务 T5-H2: 重离子超高 LET 径迹微剂量分布与横向结构模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gCoreEnergy = 0.0;
static G4double gPenumbraEnergy = 0.0;
static G4long gTrackRadialHits = 0;

class T5H2Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 10.0 * um, 10.0 * um, 10.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* trackCyl = new G4Tubs("TrackCyl", 0.0 * um, 5.0 * um, 5.0 * um, 0, 360*deg);
    auto* trackLog = new G4LogicalVolume(trackCyl, water, "TrackLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), trackLog, "TrackPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H2Physics : public G4VModularPhysicsList {
public:
  T5H2Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H2Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(20.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -4.0 * um));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H2SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "TrackPhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      // 重离子径向微剂量能量密度分布: 径迹核区 (Core) 与 半影区 (Penumbra)
      G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
      G4double r_radial = std::hypot(pos.x(), pos.y());
      G4double r_core_boundary = 10.0 * nm; // Core 半径约 10 nm

      if (r_radial <= r_core_boundary) {
        gCoreEnergy += edep; // 极高激发密度的 Core 径迹核区
      } else {
        gPenumbraEnergy += edep; // 远距离 delta-ray 电子主导的 Penumbra 半影区
      }
      gTrackRadialHits++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H2Detector());
  runManager->SetUserInitialization(new T5H2Physics());
  runManager->SetUserAction(new T5H2Generator());
  runManager->SetUserAction(new T5H2SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H2] Heavy Ion Track Radial Core: " << gCoreEnergy / keV 
            << " keV, Penumbra: " << gPenumbraEnergy / keV << " keV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
