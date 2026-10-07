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
// 任务 T6-H10: LHC 前向极小角极端辐射场次级产额模拟
// 组别: Arm A
// ============================================================================

static G4double gCentralTally = 0.0;

class T6H10Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matW = nist->FindOrBuildMaterial("G4_W");

    auto* worldSolid = new G4Box("WorldBox", 50.0 * cm, 50.0 * cm, 150.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 前向小角度量能器 (距对撞点 140 m)
    auto* calSolid = new G4Box("CaloSolid", 10.0 * cm, 10.0 * cm, 50.0 * cm);
    auto* calLog = new G4LogicalVolume(calSolid, matW, "CaloLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,140.0*m), calLog, "CaloPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T6H10Physics : public G4VModularPhysicsList {
public:
  T6H10Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H10Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("pi+"));
    gun.SetParticleEnergy(1.0 * TeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleMomentumDirection(G4ThreeVector(0.0001, 0.0001, 1).unit()); // 前向微小角
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H10SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "CaloPhys") return;

    // Arm A/B: 仅中心快度桶部均匀探测，缺少极端前向极小角高辐射环境专用量能器建模
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gCentralTally += edep;
    }

  }
};

class T6H10RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H10] Forward shower complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H10Detector());
  runManager->SetUserInitialization(new T6H10Physics());
  runManager->SetUserAction(new T6H10Generator());
  runManager->SetUserAction(new T6H10RunAction());
  runManager->SetUserAction(new T6H10SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H10] Completed" << std::endl;

  delete runManager;
  return 0;
}
