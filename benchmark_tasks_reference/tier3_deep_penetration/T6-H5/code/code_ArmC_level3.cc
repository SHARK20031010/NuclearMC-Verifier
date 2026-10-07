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
// 任务 T6-H5: 尾波场超短电子束纵向能量色散动力学
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalChirpedBunchEnergy = 0.0;
static G4long gDispersedElectrons = 0;

class T6H5Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");

    auto* worldSolid = new G4Box("WorldBox", 50.0 * cm, 50.0 * cm, 100.0 * cm);
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

    // 等离子体尾波场 (plasma wakefield) 超短电子团相空间能量啁啾 (energy chirp)
    // 束团内部纵向位置与能量强相关: E(z) = E0 + chirp * z_bunch
    G4double E0 = 100.0 * MeV;
    G4double bunch_z = (G4UniformRand() - 0.5) * 100.0 * um; // 微米级束团 (bunch)
    G4double energy_chirp_slope = 0.5 * MeV / um; // 纵向啁啾斜率 (chirp)
    G4double chirped_energy = E0 + energy_chirp_slope * bunch_z;

    gun.SetParticleEnergy(chirped_energy);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -50.0 * cm + bunch_z));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);

    gTotalChirpedBunchEnergy += chirped_energy / MeV;
    gDispersedElectrons++;
  }
};

class T6H5SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step*) override {}
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H5Detector());
  runManager->SetUserInitialization(new T6H5Physics());
  runManager->SetUserAction(new T6H5Generator());
  runManager->SetUserAction(new T6H5SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H5] Plasma Wakefield Energy Chirp Bunch completed. Avg Energy: " 
            << gTotalChirpedBunchEnergy / gDispersedElectrons << " MeV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
