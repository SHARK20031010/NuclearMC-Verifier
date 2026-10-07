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
// 任务 T4-H4: 多通道板纳秒超快脉冲时序分布与离子后脉冲
// 组别: Arm B
// ============================================================================

static G4long gMCP_Pulses = 0;
static G4long gMCP_Afterpulses = 0;
static G4double gTotalIonDelay = 0.0;

class T4H4Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* leadGlass = nist->FindOrBuildMaterial("G4_GLASS_LEAD");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* waferSolid = new G4Tubs("Wafer_Solid", 0.0 * mm, 25.0 * mm, 0.75 * mm, 0, 360*deg);
    auto* waferLog = new G4LogicalVolume(waferSolid, leadGlass, "Wafer_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), waferLog, "Wafer_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H4Physics : public G4VModularPhysicsList {
public:
  T4H4Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H4Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(1.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -2.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H4SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "Wafer_Phys") return;

    // Arm B / C: 建立微通道板 MCP 雪崩响应与离子反向加速轰击后脉冲 (afterpulse / delay)
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.05 * keV) {
      gMCP_Pulses++;
      G4double mcp_afterpulse_prob = 0.08;
      if (G4UniformRand() < mcp_afterpulse_prob) {
        G4double ion_delay_time = 250.0 * ns + G4UniformRand() * 50.0 * ns;
        gMCP_Afterpulses++;
        gTotalIonDelay += ion_delay_time;
      }
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H4Detector());
  runManager->SetUserInitialization(new T4H4Physics());
  runManager->SetUserAction(new T4H4Generator());
  runManager->SetUserAction(new T4H4SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H4] Done" << std::endl;

  delete runManager;
  return 0;
}
