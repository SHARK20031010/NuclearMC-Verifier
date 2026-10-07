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
// 任务 T4-H4: 多通道板纳秒超快脉冲时序分布与离子后脉冲
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4long gMCP_PrimaryPulses = 0;
static G4long gMCP_Afterpulses = 0; // ion afterpulse

class T4H4Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* leadGlass = nist->FindOrBuildMaterial("G4_GLASS_LEAD");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 20.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 微通道板 (MCP, Microchannel Plate)
    auto* waferSolid = new G4Tubs("MCP_Wafer_Solid", 0.0 * mm, 25.0 * mm, 0.75 * mm, 0, 360*deg);
    auto* waferLog = new G4LogicalVolume(waferSolid, leadGlass, "MCP_Wafer_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), waferLog, "MCP_Wafer_Phys", worldLog, false, 1);

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
    if (!pre || pre->GetName() != "MCP_Wafer_Phys") return;

    // 微通道板 (MCP) 初级电子雪崩与残余气体反冲正离子后脉冲 (ion afterpulse)
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.05 * keV) {
      gMCP_PrimaryPulses++;
      
      // 气体电离反向回轰残余气体阳离子 (ions) 触发延迟后脉冲 (delay afterpulse)
      G4double p_ion = 0.08; // 离子后脉冲概率
      if (G4UniformRand() < p_ion) {
        G4double delayTime_ns = 5.0 + G4UniformRand() * 20.0; // 纳秒延迟
        gMCP_Afterpulses++;
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

  std::cout << "[T4-H4] MCP Primaries: " << gMCP_PrimaryPulses 
            << " Ion Afterpulses: " << gMCP_Afterpulses << std::endl;

  delete runManager;
  std::_Exit(0);
}
