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
// 任务 T5-H6: 环状生物分子水溶液辐射化学构象动力学
// 组别: Arm C
// ============================================================================

static G4double gSupercoiledFrac = 1.0;
static G4double gRelaxedFrac = 0.0;
static G4double gLinearFrac = 0.0;
static G4long gPlasmidHits = 0;

class T5H6Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * mm, 5.0 * mm, 5.0 * mm);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T5H6Physics : public G4VModularPhysicsList {
public:
  T5H6Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H6Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.25 * MeV); // 60Co 伽马辐射
    gun.SetParticlePosition(G4ThreeVector(0, 0, -2.0 * mm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H6SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "WorldPhys") return;

    // Arm C: DNA 质粒 (plasmid) 构象动力学: 超螺旋 supercoiled 向开环 relaxed 与线状 linear 转化
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 10.0 * eV) {
      gPlasmidHits++;
      G4double prob_ssb = 0.70; // 产生单链断裂形成 relaxed 开环构象概率
      G4double prob_dsb = 0.08; // 产生双链断裂形成 linear 线性构象概率
      
      if (G4UniformRand() < prob_dsb) {
        gLinearFrac += 0.02;
        gSupercoiledFrac -= 0.02;
      } else if (G4UniformRand() < prob_ssb) {
        gRelaxedFrac += 0.02;
        gSupercoiledFrac -= 0.02;
      }
    }

  }
};

class T5H6RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H6] Conformation kinetics done." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H6Detector());
  runManager->SetUserInitialization(new T5H6Physics());
  runManager->SetUserAction(new T5H6Generator());
  runManager->SetUserAction(new T5H6RunAction());
  runManager->SetUserAction(new T5H6SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H6] Completed" << std::endl;

  delete runManager;
  return 0;
}
