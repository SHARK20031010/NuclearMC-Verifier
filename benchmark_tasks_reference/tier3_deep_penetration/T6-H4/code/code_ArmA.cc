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
// 任务 T6-H4: 超高能宇宙线大气层广延空气簇射纵向发展模拟
// 组别: Arm A
// ============================================================================

static G4double gUniformColumnEdep = 0.0;

class T6H4Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * km, 5.0 * km, 20.0 * km);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T6H4Physics : public G4VModularPhysicsList {
public:
  T6H4Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H4Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.0e5 * GeV); // 100 TeV 超高能宇宙线
    gun.SetParticlePosition(G4ThreeVector(0, 0, 15.0 * km));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H4SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "WorldPhys") return;

    // Arm A: 常密度空气均匀柱，缺少大气密度随海拔指数衰减与纵向广延大气簇射纵剖面
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gUniformColumnEdep += edep;
    }

  }
};

class T6H4RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H4] Uniform column simulation completed." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H4Detector());
  runManager->SetUserInitialization(new T6H4Physics());
  runManager->SetUserAction(new T6H4Generator());
  runManager->SetUserAction(new T6H4RunAction());
  runManager->SetUserAction(new T6H4SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H4] Completed" << std::endl;

  delete runManager;
  return 0;
}
