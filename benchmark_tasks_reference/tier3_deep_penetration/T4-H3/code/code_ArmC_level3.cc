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
// 任务 T4-H3: 双相暗物质时间投影室信号时间响应
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalS1Photons = 0.0;
static G4double gTotalS2Photons = 0.0;
static G4long gDualPhaseEvents = 0;

class T4H3Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    // 双相氙 (LXe / GXe) TPC 探测介质
    auto* matLXe = nist->BuildMaterialWithNewDensity("LiquidXenon", "G4_lXe", 2.95 * g/cm3);
    auto* matGXe = nist->BuildMaterialWithNewDensity("GasXenon", "G4_Xe", 0.0058 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 40.0 * cm, 40.0 * cm, 40.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 液氙靶区 (LXe TPC)
    auto* tpcSolid = new G4Tubs("LXe_TPC_Solid", 0.0 * cm, 15.0 * cm, 10.0 * cm, 0, 360*deg);
    auto* tpcLog = new G4LogicalVolume(tpcSolid, matLXe, "LXe_TPC_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-2.0*cm), tpcLog, "LXeChamberPhys", worldLog, false, 1);

    // 气氙电致发光区 (GXe electroluminescence gap)
    auto* gasSolid = new G4Tubs("GXe_Gap_Solid", 0.0 * cm, 15.0 * cm, 1.0 * cm, 0, 360*deg);
    auto* gasLog = new G4LogicalVolume(gasSolid, matGXe, "GXe_Gap_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,9.0*cm), gasLog, "GXeGapPhys", worldLog, false, 2);

    return worldPhys;
  }
};

class T4H3Physics : public G4VModularPhysicsList {
public:
  T4H3Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H3Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(50.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -10.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H3SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;
    G4String name = pre->GetName();
    if (name != "LXeChamberPhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 1.0 * keV) {
      // 物理双相氙 (LXe/TPC): 瞬发初级闪烁 S1 与漂移电子抽取次级电致发光 S2
      G4double W_ph = 13.8 * eV;
      G4double S1_yield = edep / W_ph * 0.2; // S1 光子数
      gTotalS1Photons += S1_yield;

      // 电子漂移至气隙产出放大的 S2 信号
      G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
      G4double drift_dist = 8.0 * cm - pos.z();
      G4double v_drift = 2.0 * mm / microsecond; // 液氙电子漂移速度
      G4double t_drift = drift_dist / v_drift;

      G4double S2_gain = 50.0; // 气氙电致发光增益
      G4double S2_yield = (edep / W_ph * 0.8) * S2_gain; // S2 光子数
      gTotalS2Photons += S2_yield;
      gDualPhaseEvents++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H3Detector());
  runManager->SetUserInitialization(new T4H3Physics());
  runManager->SetUserAction(new T4H3Generator());
  runManager->SetUserAction(new T4H3SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H3] Xenon TPC Dual-Phase S1: " << gTotalS1Photons 
            << " S2: " << gTotalS2Photons << std::endl;

  delete runManager;
  std::_Exit(0);
}
