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
// 任务 T5-H7: 纳米受限微团自由基扩散动力学模拟
// 组别: Arm A
// ============================================================================

static G4double gVesicleEnergy = 0.0;

class T5H7Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");
    auto* oil = nist->BuildMaterialWithNewDensity("OilMatrix", "G4_POLYETHYLENE", 0.78 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 100.0 * nm, 100.0 * nm, 100.0 * nm);
    auto* worldLog = new G4LogicalVolume(worldSolid, oil, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 20 nm 直径水滴微囊
    auto* dropSolid = new G4Sphere("DropSolid", 0.0 * nm, 10.0 * nm, 0, 360*deg, 0, 180*deg);
    auto* dropLog = new G4LogicalVolume(dropSolid, water, "DropLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), dropLog, "DropPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H7Physics : public G4VModularPhysicsList {
public:
  T5H7Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H7Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(5.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H7SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "DropPhys") return;

    // Arm A/B: 仅开放边界自由扩散，未建模界面疏水反射阻挡效应
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gVesicleEnergy += edep;
    }

  }
};

class T5H7RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H7] Drop boundary tracking complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H7Detector());
  runManager->SetUserInitialization(new T5H7Physics());
  runManager->SetUserAction(new T5H7Generator());
  runManager->SetUserAction(new T5H7RunAction());
  runManager->SetUserAction(new T5H7SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H7] Completed" << std::endl;

  delete runManager;
  return 0;
}
