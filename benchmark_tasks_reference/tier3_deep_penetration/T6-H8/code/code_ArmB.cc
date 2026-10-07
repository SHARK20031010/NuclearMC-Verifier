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
#include "G4MagneticField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4ClassicalRK4.hh"
#include "G4Mag_UsualEqRhs.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T6-H8: 大口径弯折管道低掠角壁面粒子反射动力学
// 组别: Arm B
// ============================================================================

static G4double gTotalWallLoss = 0.0;

class T6H8Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

    auto* worldSolid = new G4Box("WorldBox", 50.0 * cm, 50.0 * cm, 1.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 管道几何 (内径 10 cm, 外径 11 cm)
    auto* pipeSolid = new G4Tubs("PipeSolid", 9.8 * cm, 11.0 * cm, 40.0 * cm, 0, 360*deg);
    auto* pipeLog = new G4LogicalVolume(pipeSolid, steel, "PipeLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), pipeLog, "PipePhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T6H8Physics : public G4VModularPhysicsList {
public:
  T6H8Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H8Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(100.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(9.6 * cm, 0, -35.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0.01, 0, 1).unit()); // 擦边入射
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H8SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "PipePhys") return;

    // Arm A/B: 仅 100% 理想吸收管壁，缺少极低掠射角擦边弹跳散射与粗糙度退化
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gTotalWallLoss += edep;
    }

  }
};

class T6H8RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H8] Wall interaction complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H8Detector());
  runManager->SetUserInitialization(new T6H8Physics());
  runManager->SetUserAction(new T6H8Generator());
  runManager->SetUserAction(new T6H8RunAction());
  runManager->SetUserAction(new T6H8SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H8] Completed" << std::endl;

  delete runManager;
  return 0;
}
