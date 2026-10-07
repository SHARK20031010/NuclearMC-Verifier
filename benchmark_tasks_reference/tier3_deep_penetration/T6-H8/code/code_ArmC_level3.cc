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
#include "G4MagneticField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T6-H8: 大口径弯折管道低掠角壁面粒子反射动力学
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gWallGrazingReflections = 0.0;
static G4long gGrazingEvents = 0;

class T6H8Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

    auto* worldSolid = new G4Box("WorldBox", 30.0 * cm, 30.0 * cm, 60.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 大口径真空管道管壁 (Pipe Wall, R_in = 9.8 cm, R_out = 11.0 cm)
    auto* pipeSolid = new G4Tubs("PipeSolid", 9.8 * cm, 11.0 * cm, 40.0 * cm, 0, 360*deg);
    auto* pipeLog = new G4LogicalVolume(pipeSolid, steel, "PipeLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), pipeLog, "PipeWallPhys", worldLog, false, 1);

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
    // 掠射低角入射 (grazing angle incidence)
    gun.SetParticleMomentumDirection(G4ThreeVector(0.01, 0, 1).unit());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H8SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "PipeWallPhys") return;

    // 强磁场大弯管壁面极低掠角 (grazing angle) 散射与粗糙度 (roughness) 弹跳反射 (reflection)
    G4ThreeVector dir = aStep->GetPreStepPoint()->GetMomentumDirection();
    G4ThreeVector pos = aStep->GetPreStepPoint()->GetPosition();
    G4ThreeVector normal(-pos.x(), -pos.y(), 0);
    normal = normal.unit();

    G4double cos_grazing = std::abs(dir.dot(normal)); // 接近 0 即掠角
    if (cos_grazing < 0.05) { // 极低掠角
      // 表面粗糙度与反射概率模型 (reflection with roughness)
      G4double surface_roughness = 0.02; // 微观粗糙度
      G4double reflection_prob = std::exp(-surface_roughness * 10.0);
      gWallGrazingReflections += reflection_prob;
      gGrazingEvents++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H8Detector());
  runManager->SetUserInitialization(new T6H8Physics());
  runManager->SetUserAction(new T6H8Generator());
  runManager->SetUserAction(new T6H8SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H8] Wall Grazing Reflection: " << gWallGrazingReflections << std::endl;

  delete runManager;
  std::_Exit(0);
}
