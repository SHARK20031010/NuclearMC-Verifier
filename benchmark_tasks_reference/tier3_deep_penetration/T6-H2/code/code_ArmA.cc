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
// 任务 T6-H2: 四极聚焦透镜组相空间发射度椭圆传输
// 组别: Arm A
// ============================================================================

static G4double gFocalWaistEmittance = 0.0;
static G4long gTransmittedParticles = 0;

class T6H2QuadField : public G4MagneticField {
public:
  void GetFieldValue(const G4double Point[4], G4double *Bfield) const override {
    G4double gradient = 15.0 * tesla / m;
    Bfield[0] = gradient * Point[1]; // Bx = g * y
    Bfield[1] = gradient * Point[0]; // By = g * x
    Bfield[2] = 0.0;
  }
};

class T6H2Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");

    auto* worldSolid = new G4Box("WorldBox", 50.0 * cm, 50.0 * cm, 2.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 四极透镜区域
    auto* quadSolid = new G4Box("QuadSolid", 20.0 * cm, 20.0 * cm, 30.0 * cm);
    auto* quadLog = new G4LogicalVolume(quadSolid, vacuum, "QuadLog");

    auto* mag = new T6H2QuadField();
    auto* fldMgr = new G4FieldManager(mag);

    // Arm A/B: 缺少弦长误差控制器 ChordFinder / SetDeltaChord，导致强磁场弯转弦高失真发射度发散

    quadLog->SetFieldManager(fldMgr, true);

    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), quadLog, "QuadPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T6H2Physics : public G4VModularPhysicsList {
public:
  T6H2Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H2Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(50.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0.5 * mm, 0.5 * mm, -1.0 * m));
    gun.SetParticleMomentumDirection(G4ThreeVector(0.001, -0.001, 1).unit());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H2SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* post = aStep->GetPostStepPoint();
    if (post->GetPosition().z() >= 0.8 * m && aStep->GetPreStepPoint()->GetPosition().z() < 0.8 * m) {
      gTransmittedParticles++;
      G4double r_waist = post->GetPosition().perp();
      gFocalWaistEmittance += r_waist;
    }
  }
};

class T6H2RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H2] Beam optics complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H2Detector());
  runManager->SetUserInitialization(new T6H2Physics());
  runManager->SetUserAction(new T6H2Generator());
  runManager->SetUserAction(new T6H2RunAction());
  runManager->SetUserAction(new T6H2SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H2] Completed" << std::endl;

  delete runManager;
  return 0;
}
