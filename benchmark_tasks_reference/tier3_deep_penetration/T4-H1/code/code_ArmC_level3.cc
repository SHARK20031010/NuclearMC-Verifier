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
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T4-H1: 全数字 TOF-PET 双探头 LYSO 深度依赖 (DOI) 纳秒符合与时间游走
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalEdep = 0.0;
static G4long gCoincidences = 0;
static std::map<std::string, G4double> gHitTimes;

class T4H1Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matLYSO = nist->BuildMaterialWithNewDensity("LYSO_Mat", "G4_LUCITE", 7.1 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 50.0 * cm, 50.0 * cm, 50.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* lysoSolid = new G4Box("LYSO_Solid", 10.0 * cm, 10.0 * cm, 1.0 * cm);
    auto* lysoLogL = new G4LogicalVolume(lysoSolid, matLYSO, "LYSO_LogL");
    auto* lysoLogR = new G4LogicalVolume(lysoSolid, matLYSO, "LYSO_LogR");

    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, -15.0 * cm), lysoLogL, "LYSO_L", worldLog, false, 1);
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 15.0 * cm), lysoLogR, "LYSO_R", worldLog, false, 2);

    return worldPhys;
  }
};

class T4H1Physics : public G4VModularPhysicsList {
public:
  T4H1Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H1Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(511.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));
    gun.GeneratePrimaryVertex(anEvent);
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H1SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;
    std::string volName = pre->GetName();
    if (volName != "LYSO_L" && volName != "LYSO_R") return;

    // 采用全局因果时钟 GetGlobalTime()，结合相互作用深度 (DOI) 进行时间游走 (time walk) 校正
    G4double globalTime = aStep->GetPostStepPoint()->GetGlobalTime();
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.1 * MeV) {
      G4ThreeVector hitPos = aStep->GetPostStepPoint()->GetPosition();
      G4double depth_DOI = std::abs(hitPos.z()) - 14.0 * cm; // DOI 相互作用深度
      G4double v_eff = 12.0 * cm / ns; // 晶体光传输等效群速
      G4double time_walk_DOI = depth_DOI / v_eff; // 时间游走延迟
      G4double hitTime_corrected_TOF = globalTime - time_walk_DOI;

      gHitTimes[volName] = hitTime_corrected_TOF;
      gTotalEdep += edep;
    }
  }
};

class T4H1EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gHitTimes.clear();
  }
  void EndOfEventAction(const G4Event*) override {
    if (gHitTimes.count("LYSO_L") && gHitTimes.count("LYSO_R")) {
      G4double dt = std::abs(gHitTimes["LYSO_L"] - gHitTimes["LYSO_R"]);
      if (dt < 0.3 * ns) {
        gCoincidences++;
      }
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H1Detector());
  runManager->SetUserInitialization(new T4H1Physics());
  runManager->SetUserAction(new T4H1Generator());
  runManager->SetUserAction(new T4H1EventAction());
  runManager->SetUserAction(new T4H1SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H1] Coincidences: " << gCoincidences 
            << " TotalEdep: " << gTotalEdep / MeV << " MeV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
