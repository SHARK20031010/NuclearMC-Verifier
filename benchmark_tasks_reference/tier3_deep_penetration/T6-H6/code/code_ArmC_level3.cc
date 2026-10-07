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
// 任务 T6-H6: 强激光靶微观相对论强流电子相空间积分模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalLaserHotElectronEnergy = 0.0;
static G4long gLaserElectronCount = 0;

class T6H6Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matAl = nist->FindOrBuildMaterial("G4_Al");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* foilSolid = new G4Box("FoilSolid", 2.0 * cm, 2.0 * cm, 0.01 * cm);
    auto* foilLog = new G4LogicalVolume(foilSolid, matAl, "FoilLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), foilLog, "FoilPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T6H6Physics : public G4VModularPhysicsList {
public:
  T6H6Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H6Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticlePosition(G4ThreeVector(0, 0, -5.0 * cm));

    // 强激光 (laser) 相互作用相对论麦克斯韦热分布 (relativistic Maxwell-Boltzmann)
    // f(E) ~ exp(-E / T_hot), 伴随前向锥形角发散 (cone divergence)
    G4double T_hot = 1.5 * MeV; // Ponderomotive 温度
    G4double sample_energy = -T_hot * std::log(G4UniformRand() + 1e-10); // Maxwellian tail
    if (sample_energy < 50.0 * keV) sample_energy = 50.0 * keV;

    // 前向圆锥角发散角 (cone divergence angle)
    G4double theta_cone_max = 25.0 * deg;
    G4double theta = G4UniformRand() * theta_cone_max;
    G4double phi = G4UniformRand() * 360.0 * deg;
    G4ThreeVector dir(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));

    gun.SetParticleEnergy(sample_energy);
    gun.SetParticleMomentumDirection(dir.unit());
    gun.GeneratePrimaryVertex(anEvent);

    gTotalLaserHotElectronEnergy += sample_energy / MeV;
    gLaserElectronCount++;
  }
};

class T6H6SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step*) override {}
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H6Detector());
  runManager->SetUserInitialization(new T6H6Physics());
  runManager->SetUserAction(new T6H6Generator());
  runManager->SetUserAction(new T6H6SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H6] Relativistic Laser Electron Cone Beam complete. Avg E: " 
            << gTotalLaserHotElectronEnergy / gLaserElectronCount << " MeV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
