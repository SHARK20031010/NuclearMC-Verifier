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
// 任务 T4-H7: 像素半导体纳秒级能量与到达时间响应
// 组别: Arm A
// ============================================================================

static G4double gMacroMatrixEdep = 0.0;
static G4long gMatrixHits = 0;

class T4H7Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matSi = nist->FindOrBuildMaterial("G4_Si");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* sensorSolid = new G4Box("Sensor_Solid", 7.0 * mm, 7.0 * mm, 0.15 * mm);
    auto* sensorLog = new G4LogicalVolume(sensorSolid, matSi, "Sensor_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), sensorLog, "SiliconMatrix_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H7Physics : public G4VModularPhysicsList {
public:
  T4H7Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H7Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("mu-"));
    gun.SetParticleEnergy(4.0 * GeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -5.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H7SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "SiliconMatrix_Phys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.5 * keV) {
      gMacroMatrixEdep += edep;
      gMatrixHits++;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H7Detector());
  runManager->SetUserInitialization(new T4H7Physics());
  runManager->SetUserAction(new T4H7Generator());
  runManager->SetUserAction(new T4H7SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H7] Done" << std::endl;

  delete runManager;
  return 0;
}
