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
// 任务 T4-H6: 同轴 HPGe 探测器电极权场与脉冲波形
// 组别: Arm B
// ============================================================================

static G4double gTotalChargeSimple = 0.0;
static G4long gTotalHits = 0;

class T4H6Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matGe = nist->FindOrBuildMaterial("G4_Ge");

    auto* worldSolid = new G4Box("WorldBox", 30.0 * cm, 30.0 * cm, 30.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* coaxSolid = new G4Tubs("HPGeCyl_Solid", 5.0 * mm, 35.0 * mm, 30.0 * mm, 0, 360*deg);
    auto* coaxLog = new G4LogicalVolume(coaxSolid, matGe, "HPGeCyl_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), coaxLog, "HPGeCyl_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H6Physics : public G4VModularPhysicsList {
public:
  T4H6Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H6Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.332 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -10.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H6SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "HPGeCyl_Phys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 1.0 * keV) {
      gTotalChargeSimple += edep;
      gTotalHits++;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H6Detector());
  runManager->SetUserInitialization(new T4H6Physics());
  runManager->SetUserAction(new T4H6Generator());
  runManager->SetUserAction(new T4H6SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H6] Completed" << std::endl;

  delete runManager;
  return 0;
}
