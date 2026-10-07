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
// 任务 T4-H2: 超低本底 HPGe 井型探测器真符合求和校正
// 组别: Arm C
// ============================================================================

static G4double gTotalEdep = 0.0;
static G4double gEventEdep = 0.0;
static G4long gTCS_SummingPeaks = 0;

class T4H2Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matGe = nist->FindOrBuildMaterial("G4_Ge");

    auto* worldSolid = new G4Box("WorldBox", 30.0 * cm, 30.0 * cm, 30.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* hpgeSolid = new G4Tubs("HPGe_Solid", 0.0 * mm, 35.0 * mm, 35.0 * mm, 0, 360*deg);
    auto* hpgeLog = new G4LogicalVolume(hpgeSolid, matGe, "HPGe_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), hpgeLog, "HPGe_WellPhys", worldLog, false, 1);

    auto* cavitySolid = new G4Tubs("CavitySolid", 0.0 * mm, 8.0 * mm, 20.0 * mm, 0, 360*deg);
    auto* cavityLog = new G4LogicalVolume(cavitySolid, air, "WellCavityLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15.0*mm), cavityLog, "WellCavityPhys", hpgeLog, false, 2);

    return worldPhys;
  }
};

class T4H2Physics : public G4VModularPhysicsList {
public:
  T4H2Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H2Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {

    // Arm C: 模拟 133Ba 复杂多级联伽马发射 (81 keV 与 276 keV 级联发射)
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleEnergy(81.0 * keV);
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);

    gun.SetParticleEnergy(276.0 * keV);
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);

  }
};

class T4H2SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "HPGe_WellPhys") return;
    G4double edep = aStep->GetTotalEnergyDeposit();
    gEventEdep += edep;
    gTotalEdep += edep;
  }
};

class T4H2EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gEventEdep = 0.0;
  }
  void EndOfEventAction(const G4Event*) override {

    // Arm C: 评估真符合求和 (TCS, True Coincidence Summing) 和峰 (Summing Peak)
    if (gEventEdep > 350.0 * keV && gEventEdep < 362.0 * keV) {
      gTCS_SummingPeaks++;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H2Detector());
  runManager->SetUserInitialization(new T4H2Physics());
  runManager->SetUserAction(new T4H2Generator());
  runManager->SetUserAction(new T4H2EventAction());
  runManager->SetUserAction(new T4H2SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H2] TotalEdep: " << gTotalEdep / keV << " keV" << std::endl;

  delete runManager;
  return 0;
}
