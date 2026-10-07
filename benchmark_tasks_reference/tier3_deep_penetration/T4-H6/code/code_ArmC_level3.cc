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
// 任务 T4-H6: 同轴 HPGe 探测器电极权场与脉冲波形
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalInducedCharge = 0.0;
static G4double gAvgRiseTime = 0.0;
static G4long gWaveformEvents = 0;

class T4H6Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matGe = nist->FindOrBuildMaterial("G4_Ge");

    auto* worldSolid = new G4Box("WorldBox", 30.0 * cm, 30.0 * cm, 30.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 同轴锗探测器圆柱体: 内径 r1 = 5 mm, 外径 r2 = 35 mm
    auto* coaxSolid = new G4Tubs("HPGeCyl_Solid", 5.0 * mm, 35.0 * mm, 30.0 * mm, 0, 360*deg);
    auto* coaxLog = new G4LogicalVolume(coaxSolid, matGe, "HPGeCyl_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), coaxLog, "HPGeCyl_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H6Physics : public G4VModularPhysicsList {
public:
  T4H6Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H6Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.332 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -10.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H6SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "HPGeCyl_Phys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 1.0 * keV) {
      // 基于 Ramo-Shockley 定理计算电极权场 (weighting potential) 与载流子漂移 (drift) 脉冲上升时间 (rise time)
      G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
      G4double r = std::hypot(pos.x(), pos.y());
      G4double r_in = 5.0 * mm;
      G4double r_out = 35.0 * mm;
      if (r < r_in) r = r_in;
      if (r > r_out) r = r_out;

      // 同轴电极权势: Ramo-Shockley weighting potential
      G4double weightingPotential = std::log(r / r_in) / std::log(r_out / r_in);

      // 电子/空穴向电极漂移动力学与脉冲上升沿时间 (rise time)
      G4double v_drift = 1.0e7 * cm / s; // 载流子饱和漂移速度
      G4double drift_dist = r_out - r;
      G4double pulse_rise_time = drift_dist / v_drift; // 上升时间

      G4double inducedCharge = (edep / (2.96 * eV)) * weightingPotential;
      gTotalInducedCharge += inducedCharge;
      gAvgRiseTime += pulse_rise_time;
      gWaveformEvents++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H6Detector());
  runManager->SetUserInitialization(new T4H6Physics());
  runManager->SetUserAction(new T4H6Generator());
  runManager->SetUserAction(new T4H6SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H6] Ramo-Shockley Pulse Waveform complete. Events: " 
            << gWaveformEvents << std::endl;

  delete runManager;
  std::_Exit(0);
}
