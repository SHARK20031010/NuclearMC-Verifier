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
// 任务 T4-H9: 双层符合运动学几何探测器
// 组别: Arm B
// ============================================================================

static G4double gTotalSlabEdep = 0.0;

class T4H9Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matSi = nist->FindOrBuildMaterial("G4_Si");
    auto* matCZT = nist->BuildMaterialWithNewDensity("HeavyCrystal_Mat", "G4_CdWO4", 5.8 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 40.0 * cm, 40.0 * cm, 40.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* l1Solid = new G4Box("Layer1Solid", 10.0 * cm, 10.0 * cm, 0.2 * cm);
    auto* l1Log = new G4LogicalVolume(l1Solid, matSi, "Layer1Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,5.0*cm), l1Log, "FirstPlatePhys", worldLog, false, 1);

    auto* l2Solid = new G4Box("Layer2Solid", 12.0 * cm, 12.0 * cm, 1.0 * cm);
    auto* l2Log = new G4LogicalVolume(l2Solid, matCZT, "Layer2Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15.0*cm), l2Log, "SecondPlatePhys", worldLog, false, 2);

    return worldPhys;
  }
};

class T4H9Physics : public G4VModularPhysicsList {
public:
  T4H9Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H9Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(662.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H9SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 10.0 * keV) {
      gTotalSlabEdep += edep;
    }

  }
};

class T4H9EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gTotalSlabEdep = 0.0;
  }
  void EndOfEventAction(const G4Event*) override {

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H9Detector());
  runManager->SetUserInitialization(new T4H9Physics());
  runManager->SetUserAction(new T4H9Generator());
  runManager->SetUserAction(new T4H9EventAction());
  runManager->SetUserAction(new T4H9SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H9] Completed" << std::endl;

  delete runManager;
  return 0;
}
