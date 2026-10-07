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
// 任务 T5-H3: 微剂量学比能累积理论与细胞存活率曲线模型
// 组别: Arm A
// ============================================================================

static G4double gMacroscopicAbsorbedDose = 0.0;

class T5H3Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 5.0 * um, 5.0 * um, 5.0 * um);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 1 微米组织等效微腔球
    auto* sphereSolid = new G4Sphere("MicroSphere", 0.0 * um, 0.5 * um, 0, 360*deg, 0, 180*deg);
    auto* sphereLog = new G4LogicalVolume(sphereSolid, water, "MicroSphereLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), sphereLog, "MicroSpherePhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H3Physics : public G4VModularPhysicsList {
public:
  T5H3Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H3Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(3.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -2.0 * um));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H3SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "MicroSpherePhys") return;

    // Arm A: 简单宏观吸收剂量累加，缺少微剂量比能分布单事件谱与双辐射作用细胞存活率参数推导
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gMacroscopicAbsorbedDose += edep;
    }

  }
};

class T5H3RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H3] Microdosimetric analysis finished." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H3Detector());
  runManager->SetUserInitialization(new T5H3Physics());
  runManager->SetUserAction(new T5H3Generator());
  runManager->SetUserAction(new T5H3RunAction());
  runManager->SetUserAction(new T5H3SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H3] Completed" << std::endl;

  delete runManager;
  return 0;
}
