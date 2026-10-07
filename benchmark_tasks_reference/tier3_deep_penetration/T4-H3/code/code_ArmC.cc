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
// 任务 T4-H3: 双相暗物质时间投影室信号时间响应
// 组别: Arm C
// ============================================================================

static G4double gS1_Signal = 0.0;
static G4double gS2_Signal = 0.0;
static G4long gTPC_Events = 0;

class T4H3Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matGas = nist->BuildMaterialWithNewDensity("Medium_Mat", "G4_Ar", 1.78 * mg/cm3);

    auto* worldSolid = new G4Box("WorldBox", 40.0 * cm, 40.0 * cm, 40.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* tpcSolid = new G4Tubs("Chamber_Solid", 0.0 * cm, 15.0 * cm, 12.0 * cm, 0, 360*deg);
    auto* tpcLog = new G4LogicalVolume(tpcSolid, matGas, "Chamber_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tpcLog, "TPC_LXePhys", worldLog, false, 1);

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
    if (!pre || pre->GetName() != "TPC_LXePhys") return;

    // Arm C: 双相氙 (LXe / GXe) 时间投影室 TPC: 解耦产生瞬发 S1 闪烁与漂移 S2 电致发光信号
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 1.0 * keV) {
      G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
      G4double z_drift = 10.0 * cm - pos.z();
      G4double v_drift = 2.0 * mm / microsecond;
      G4double driftTime = z_drift / v_drift;
      G4double photons_S1 = edep / (13.7 * eV) * 0.2;
      gS1_Signal += photons_S1;
      G4double electrons = edep / (13.7 * eV) * 0.8;
      G4double photons_S2 = electrons * 30.0;
      gS2_Signal += photons_S2;
      gTPC_Events++;
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

  std::cout << "[T4-H3] Run Completed" << std::endl;

  delete runManager;
  return 0;
}
