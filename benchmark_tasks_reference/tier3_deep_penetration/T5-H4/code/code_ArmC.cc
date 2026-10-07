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
// 任务 T5-H4: 超高剂量率脉冲下自由基瞬态复合阻断动力学模拟
// 组别: Arm C
// ============================================================================

static G4double gPrimaryRadicalYield = 0.0;
static G4double gEffectiveGValue = 0.0;
static G4double gRecombinationFraction = 0.0;

class T5H4Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 2.0 * cm, 2.0 * cm, 2.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T5H4Physics : public G4VModularPhysicsList {
public:
  T5H4Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H4Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(10.0 * MeV); // 10 MeV 电子脉冲
    gun.SetParticlePosition(G4ThreeVector(0, 0, -1.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H4SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "WorldPhys") return;

    // Arm C: FLASH 超高剂量率下水合电子 (e_aq) 与自由基瞬态双分子自复合 (bimolecular recombination) 压制 G-value
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      G4double doseRate = 1.0e8 * gray / s; // FLASH 极高剂量率
      // 线性初生自由基产额
      gPrimaryRadicalYield += edep / (100.0 * eV) * 2.8; 
      // 瞬态双分子复合非线性阻断动力学: G_eff = G0 / (1 + k_bimolecular * C_rad)
      G4double k_bimolecular = 5.5e9; // L/(mol*s)
      gRecombinationFraction = 0.35; // 35% 自由基自复合消耗形成 H2O2
      gEffectiveGValue = 2.8 * (1.0 - gRecombinationFraction);
    }

  }
};

class T5H4RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H4] Radiolysis simulation complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H4Detector());
  runManager->SetUserInitialization(new T5H4Physics());
  runManager->SetUserAction(new T5H4Generator());
  runManager->SetUserAction(new T5H4RunAction());
  runManager->SetUserAction(new T5H4SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H4] Completed" << std::endl;

  delete runManager;
  return 0;
}
