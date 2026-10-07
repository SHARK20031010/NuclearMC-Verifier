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
// 任务 T6-H10: LHC 前向极小角极端辐射场次级产额模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gForwardCalorimeterEdep = 0.0;
static G4long gForwardHits = 0;

class T6H10Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matW = nist->FindOrBuildMaterial("G4_W"); // 钨吸能块 (tungsten absorber)

    auto* worldSolid = new G4Box("WorldBox", 2.0 * m, 2.0 * m, 150.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // LHC 前向极小角量能器 (forward calorimeter, eta > 8.0)
    // 放置于相互作用点前向 z = 140 m 处
    auto* calSolid = new G4Box("ForwardCalorimeterSolid", 10.0 * cm, 10.0 * cm, 50.0 * cm);
    auto* calLog = new G4LogicalVolume(calSolid, matW, "ForwardCalorimeterLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 140.0 * m), calLog, "ForwardCalorimeterPhys", worldLog, false, 1);

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
    // LHC 相互作用点前向极小角 (pseudo-rapidity eta ~ 8.5)
    gun.SetParticleMomentumDirection(G4ThreeVector(0.0001, 0.0001, 1).unit());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H10SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "ForwardCalorimeterPhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gForwardCalorimeterEdep += edep;
      gForwardHits++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H10Detector());
  runManager->SetUserInitialization(new T6H10Physics());
  runManager->SetUserAction(new T6H10Generator());
  runManager->SetUserAction(new T6H10SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H10] LHC Forward Calorimeter Edep: " 
            << gForwardCalorimeterEdep / GeV << " GeV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
