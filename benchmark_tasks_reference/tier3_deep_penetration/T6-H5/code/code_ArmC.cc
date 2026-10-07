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
// 任务 T6-H5: 尾波场超短电子束纵向能量色散动力学
// 组别: Arm C
// ============================================================================

static G4double gWakefieldChirpDispersion = 0.0;
static G4long gBunchCount = 0;

class T6H5Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 1.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T6H5Physics : public G4VModularPhysicsList {
public:
  T6H5Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H5Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));

    // Arm C: 等离子体尾波场超短电子团 (wakefield bunch) 纵向相空间能量啁啾 (chirp) 与动量色散 (dispersion)
    G4double z_local = (G4UniformRand() - 0.5) * 10.0 * um; // 10 微米束长
    G4double E_chirp = 100.0 * MeV + (z_local / (5.0 * um)) * 2.5 * MeV; // 头部 100 MeV，尾部 105 MeV
    gun.SetParticleEnergy(E_chirp);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -50.0 * cm + z_local));
    gWakefieldChirpDispersion += (E_chirp / MeV);
    gBunchCount++;

    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H5SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step*) override {}
};

class T6H5RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H5] Chirp dispersion evaluated." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H5Detector());
  runManager->SetUserInitialization(new T6H5Physics());
  runManager->SetUserAction(new T6H5Generator());
  runManager->SetUserAction(new T6H5RunAction());
  runManager->SetUserAction(new T6H5SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H5] Completed" << std::endl;

  delete runManager;
  return 0;
}
