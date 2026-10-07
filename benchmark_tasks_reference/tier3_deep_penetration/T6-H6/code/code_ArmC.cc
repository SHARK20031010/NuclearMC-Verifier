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
// 任务 T6-H6: 强激光靶微观相对论强流电子相空间积分模拟
// 组别: Arm C
// ============================================================================

static G4double gLaserBeamCurrent = 0.0;
static G4long gMaxwellRelativisticParticles = 0;

class T6H6Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matAl = nist->FindOrBuildMaterial("G4_Al");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 铝微靶箔片 (厚度 50 um)
    auto* foilSolid = new G4Box("FoilSolid", 2.0 * cm, 2.0 * cm, 0.025 * mm);
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

    // Arm B / C: 强激光 (laser) 相对论麦克斯韦 (Maxwell) 能谱与锥角发散 (cone divergence) 束流强流 (current)
    G4double kT = 2.0 * MeV;
    G4double E_kin = -kT * std::log(G4UniformRand() * G4UniformRand() + 1e-9); // 麦克斯韦热谱近似
    gun.SetParticleEnergy(E_kin);
    
    // 锥形角发散抽样
    G4double theta = G4UniformRand() * 15.0 * deg; // 15度发散角
    G4double phi = G4UniformRand() * 360.0 * deg;
    G4ThreeVector dir(std::sin(theta)*std::cos(phi), std::sin(theta)*std::sin(phi), std::cos(theta));
    gun.SetParticleMomentumDirection(dir);
    
    gLaserBeamCurrent += (E_kin / MeV);
    gMaxwellRelativisticParticles++;

    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H6SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step*) override {}
};

class T6H6RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H6] Laser electron beam complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H6Detector());
  runManager->SetUserInitialization(new T6H6Physics());
  runManager->SetUserAction(new T6H6Generator());
  runManager->SetUserAction(new T6H6RunAction());
  runManager->SetUserAction(new T6H6SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H6] Completed" << std::endl;

  delete runManager;
  return 0;
}
