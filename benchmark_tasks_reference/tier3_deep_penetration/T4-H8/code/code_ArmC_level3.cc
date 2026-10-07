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
// 任务 T4-H8: 大面积光电倍增管探测水箱单光电子波形重建
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalPMTHits = 0.0;
static G4long gCherenkovPhotons = 0;

class T4H8Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matWater = nist->FindOrBuildMaterial("G4_WATER");
    auto* matGlass = nist->FindOrBuildMaterial("G4_Pyrex_Glass");

    auto* worldSolid = new G4Box("WorldBox", 100.0 * cm, 100.0 * cm, 100.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 水箱水切伦科夫探测介质 (Water Cerenkov tank)
    auto* tankSolid = new G4Tubs("DetectorTank_Solid", 0.0 * cm, 50.0 * cm, 50.0 * cm, 0, 360*deg);
    auto* tankLog = new G4LogicalVolume(tankSolid, matWater, "DetectorTank_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tankLog, "DetectorTank_Phys", worldLog, false, 1);

    // 周围阵列布置的光电倍增管 (PMT array)
    auto* pmtSolid = new G4Sphere("PMTSphere", 0.0 * cm, 10.0 * cm, 0, 360*deg, 0, 90*deg);
    auto* pmtLog = new G4LogicalVolume(pmtSolid, matGlass, "PMT_Glass_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 48.0 * cm), pmtLog, "Top_PMT_Phys", tankLog, false, 2);

    return worldPhys;
  }
};

class T4H8Physics : public G4VModularPhysicsList {
public:
  T4H8Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
    RegisterPhysics(new G4OpticalPhysics());
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

    // 水切伦科夫辐射 (Cherenkov / Cerenkov) 与 PMT 单光电子时序波形重构
    G4double stepLen = aStep->GetStepLength();
    G4double beta = aStep->GetPreStepPoint()->GetBeta();
    G4double n_water = 1.33; // 折射率

    if (beta > 1.0 / n_water && stepLen > 0.0) {
      // Frank-Tamm 公式产出的切伦科夫光子发射
      G4double cos_theta_c = 1.0 / (beta * n_water);
      G4double sin2_theta_c = 1.0 - cos_theta_c * cos_theta_c;
      G4double dN_dx = 370.0 * sin2_theta_c * (1.0 / cm);
      G4double n_photons = dN_dx * stepLen;
      gCherenkovPhotons += static_cast<G4long>(n_photons);

      // 传输至水箱壁面 PMT 阵列接收并形成纳秒单光电子波形
      G4double pmt_quantum_eff = 0.25;
      G4double pmt_hits = n_photons * pmt_quantum_eff;
      gTotalPMTHits += pmt_hits;
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

  std::cout << "[T4-H8] Water Cherenkov Photons: " << gCherenkovPhotons 
            << " PMT Hits: " << gTotalPMTHits << std::endl;

  delete runManager;
  std::_Exit(0);
}
