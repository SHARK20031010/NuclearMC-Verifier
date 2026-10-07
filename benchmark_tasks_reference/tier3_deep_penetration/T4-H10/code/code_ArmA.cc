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
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T4-H10: 复合介质闪烁发光探测
// 组别: Arm A
// ============================================================================

static G4double gHomogeneousLight = 0.0;
static G4long gScintPlateHits = 0;

class T4H10Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matPlate = nist->BuildMaterialWithNewDensity("CompositeScint_Mat", "G4_POLYSTYRENE", 2.8 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* screenSolid = new G4Box("ScreenSolid", 5.0 * cm, 5.0 * cm, 0.1 * cm);
    auto* screenLog = new G4LogicalVolume(screenSolid, matPlate, "ScreenLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), screenLog, "ScreenPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H10Physics : public G4VModularPhysicsList {
public:
  T4H10Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H10Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(2.05 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -0.05 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H10SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "ScreenPhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 5.0 * keV) {
      gHomogeneousLight += edep / (25.0 * eV);
      gScintPlateHits++;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H10Detector());
  runManager->SetUserInitialization(new T4H10Physics());
  runManager->SetUserAction(new T4H10Generator());
  runManager->SetUserAction(new T4H10SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H10] Completed" << std::endl;

  delete runManager;
  return 0;
}
