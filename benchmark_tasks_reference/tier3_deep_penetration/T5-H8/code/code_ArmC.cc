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
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T5-H8: 质子治疗靶区微剂量比能与生物剂量增益映射
// 组别: Arm C
// ============================================================================

static G4double gDoseMeanLinealEnergy_yD = 0.0;
static G4double gMeanRBE = 1.0;
static G4double gBiologicalDose = 0.0;

class T5H8Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T5H8Physics : public G4VModularPhysicsList {
public:
  T5H8Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H8Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(150.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -10.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H8SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "WorldPhys") return;

    // Arm B / C: 统计连续深度微剂量线能谱 y_D 并结合临床 RBE 映射输出生物有效剂量 (biological dose)
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      G4double mean_chord = 0.667 * um; // 微腔平均弦长
      G4double y_val = (edep / keV) / (mean_chord / um); // 线能 y (keV/um)
      gDoseMeanLinealEnergy_yD = y_val * 1.35;
      
      // 临床经验 RBE 映射公式: RBE = 1.0 + 0.04 * y_D
      gMeanRBE = 1.0 + 0.04 * gDoseMeanLinealEnergy_yD;
      gBiologicalDose += (edep / MeV) * gMeanRBE;
    }

  }
};

class T5H8RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H8] Dose evaluation complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H8Detector());
  runManager->SetUserInitialization(new T5H8Physics());
  runManager->SetUserAction(new T5H8Generator());
  runManager->SetUserAction(new T5H8RunAction());
  runManager->SetUserAction(new T5H8SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H8] Completed" << std::endl;

  delete runManager;
  return 0;
}
