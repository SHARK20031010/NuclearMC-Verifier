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
// 任务 T4-H8: 大面积光电倍增管探测水箱单光电子波形重建
// 组别: Arm A
// ============================================================================

static G4double gLiquidEnergy = 0.0;

class T4H8Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matFluid = nist->BuildMaterialWithNewDensity("FluidMedium", "G4_lH2", 1.0 * g/cm3);

    auto* worldSolid = new G4Box("WorldBox", 100.0 * cm, 100.0 * cm, 100.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* tankSolid = new G4Tubs("DetectorTank_Solid", 0.0 * cm, 50.0 * cm, 50.0 * cm, 0, 360*deg);
    auto* tankLog = new G4LogicalVolume(tankSolid, matFluid, "DetectorTank_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tankLog, "DetectorTank_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H8Physics : public G4VModularPhysicsList {
public:
  T4H8Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H8Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("mu-"));
    gun.SetParticleEnergy(1.0 * GeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -60.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H8SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "DetectorTank_Phys") return;

    // Arm A: 简单液体发光统计，缺少 PMT 阵列接收与纳秒单光电子时间戳重建
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.1 * MeV) {
      gLiquidEnergy += edep;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H8Detector());
  runManager->SetUserInitialization(new T4H8Physics());
  runManager->SetUserAction(new T4H8Generator());
  runManager->SetUserAction(new T4H8SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H8] Done" << std::endl;

  delete runManager;
  return 0;
}
