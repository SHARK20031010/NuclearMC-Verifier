/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M3 目标几何空间出射面", "src": "U"},
  "F2":  {"v": "count", "src": "U"},
  "F3":  {"v": "other", "src": "A"},
  "F4":  {"v": "surface_avg", "src": "U"},
  "F5":  {"v": "steady", "src": "A"},
  "F6":  {"v": "per_source", "src": "U"},
  "F7":  {"v": "trend", "src": "A"},
  "F8":  {"v": "other_mc", "src": "A"},
  "F9":  {"v": "N/A", "src": "U"},
  "F10": {"v": "scalar", "src": "U"},
  "warnings": ["per_source_needs_strength"]
}
```
*/

// ============================================================================
// 蒙特卡洛任务 T1-M3 - Arm C (Level 3 纯第一性原理修复重构)
// 题目需求：
//   单能 6 MeV 伽马束分别以 0°、30°、45°、60° 倾角入射到一块 40 cm 厚的
//   重晶石混凝土屏蔽墙上。计算不同入射角度下的全能透射率和多次散射透射角分布。
// ============================================================================

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4Gamma.hh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>

// ============================================================================
// 1. 几何与材料构建：40 cm 厚重晶石混凝土屏蔽墙
// ============================================================================
class BariteWallDetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();

    // 世界体积 (4m x 4m x 4m 空气)
    auto* worldSolid = new G4Box("World", 2.0 * m, 2.0 * m, 2.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 重晶石防辐射混凝土材料构建 (密度 3.35 g/cm3, 70% BaSO4 + 30% G4_CONCRETE)
    auto* concreteMat = nist->FindOrBuildMaterial("G4_CONCRETE");
    auto* baso4Mat = nist->FindOrBuildMaterial("G4_BARIUM_SULFATE");
    auto* bariteConcrete = new G4Material("BariteConcrete", 3.35 * g / cm3, 2);
    bariteConcrete->AddMaterial(baso4Mat, 0.70);
    bariteConcrete->AddMaterial(concreteMat, 0.30);

    // 40 cm 厚屏蔽墙 (half-z = 20 cm)，横向 3m x 3m 足够宽以容纳大倾角斜向偏转
    auto* wallSolid = new G4Box("BariteWall", 1.5 * m, 1.5 * m, 20.0 * cm);
    auto* wallLog = new G4LogicalVolume(wallSolid, bariteConcrete, "BariteWall");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), wallLog, "BariteWall", worldLog, false, 0);

    return worldPV;
  }
};

// ============================================================================
// 2. 物理列表：高精度标准电磁 Option4 (光电吸收、康普顿散射、电子对产生)
// ============================================================================
class ShieldingPhysicsList : public G4VModularPhysicsList {
public:
  ShieldingPhysicsList() {
    SetVerboseLevel(0);
    RegisterPhysics(new G4EmStandardPhysics_option4());
  }
};

// ============================================================================
// 3. 初级粒子源：6 MeV 单能伽马束，支持倾角 (0°, 30°, 45°, 60°) 抽样
// ============================================================================
class BaritePrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
private:
  G4ParticleGun* fParticleGun;
  G4double fIncidentAngleRad; // 入射倾角 (弧度)

public:
  BaritePrimaryGeneratorAction() : fIncidentAngleRad(0.0) {
    fParticleGun = new G4ParticleGun(1);
    fParticleGun->SetParticleDefinition(G4Gamma::Gamma());
    fParticleGun->SetParticleEnergy(6.0 * MeV);
  }

  ~BaritePrimaryGeneratorAction() override {
    delete fParticleGun;
  }

  void SetIncidentAngle(G4double angleRad) {
    fIncidentAngleRad = angleRad;
  }

  G4double GetIncidentAngle() const {
    return fIncidentAngleRad;
  }

  void GeneratePrimaries(G4Event* anEvent) override {
    // 束流发射在屏蔽墙前表面中心 (z = -20.001 cm)
    // 倾角动量矢量：(sin(theta), 0, cos(theta))
    fParticleGun->SetParticlePosition(G4ThreeVector(0.0, 0.0, -20.001 * cm));
    G4ThreeVector dir(std::sin(fIncidentAngleRad), 0.0, std::cos(fIncidentAngleRad));
    fParticleGun->SetParticleMomentumDirection(dir.unit());
    fParticleGun->GeneratePrimaryVertex(anEvent);
  }
};

// ============================================================================
// 4. 计分与步进统计：全能透射与多次散射出射角分布统计
// ============================================================================
struct AngleTally {
  G4long numIncident = 0;
  G4long numTotalTransmitted = 0;
  G4long numUncollided = 0;
  G4long numScattered = 0;
  static const int NBINS = 9; // 0-10, 10-20, ..., 80-90 度
  G4long angularBins[NBINS] = {0};

  void Reset() {
    numIncident = 0;
    numTotalTransmitted = 0;
    numUncollided = 0;
    numScattered = 0;
    for (int i = 0; i < NBINS; ++i) angularBins[i] = 0;
  }
};

static AngleTally gCurrentTally;

class BariteSteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* track = step->GetTrack();
    if (track->GetDefinition() != G4Gamma::Gamma()) return;

    auto* prePoint = step->GetPreStepPoint();
    auto* postPoint = step->GetPostStepPoint();
    auto* prePV = prePoint->GetPhysicalVolume();
    auto* postPV = postPoint->GetPhysicalVolume();

    // 严格核验粒子是否穿过屏蔽墙后表面出射 (BariteWall -> World, z >= 20.0 cm)
    if (prePV && postPV && prePV->GetName() == "BariteWall" && postPV->GetName() == "World") {
      if (postPoint->GetPosition().z() >= 20.0 * cm) {
        gCurrentTally.numTotalTransmitted++;
        G4double energy = postPoint->GetKineticEnergy();

        // 判定全能未碰撞峰 (6 MeV 初始能量无损失) vs 多次散射
        if (energy >= 5.999 * MeV) {
          gCurrentTally.numUncollided++;
        } else {
          gCurrentTally.numScattered++;
          // 计算透射极角 theta_out (相对于屏蔽墙法向 +z 方向的夹角)
          G4ThreeVector momDir = postPoint->GetMomentumDirection();
          G4double cosTheta = momDir.z();
          if (cosTheta > 1.0) cosTheta = 1.0;
          if (cosTheta < -1.0) cosTheta = -1.0;
          G4double thetaDeg = std::acos(cosTheta) / deg;

          int bin = static_cast<int>(thetaDeg / 10.0);
          if (bin >= 0 && bin < AngleTally::NBINS) {
            gCurrentTally.angularBins[bin]++;
          }
        }

        // 避免出射后空气反散射造成的重复计分，出射即刻终止跟踪
        track->SetTrackStatus(fStopAndKill);
      }
    }
  }
};

// ============================================================================
// 5. 主程序与多角度仿真流程
// ============================================================================
int main(int argc, char** argv) {
  int nEventsPerAngle = (argc > 1) ? std::atoi(argv[1]) : 500;
  if (nEventsPerAngle <= 0) nEventsPerAngle = 500;

  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);

  runManager->SetUserInitialization(new BariteWallDetectorConstruction());
  runManager->SetUserInitialization(new ShieldingPhysicsList());

  auto* primaryGen = new BaritePrimaryGeneratorAction();
  runManager->SetUserAction(primaryGen);
  runManager->SetUserAction(new BariteSteppingAction());

  runManager->Initialize();

  std::vector<G4double> testAnglesDeg = {0.0, 30.0, 45.0, 60.0};

  std::cout << "======================================================================\n";
  std::cout << "  [T1-M3 Arm C] 6 MeV 伽马斜入射 40 cm 重晶石混凝土透射率与角分布仿真\n";
  std::cout << "  每种入射倾角采样粒子数: " << nEventsPerAngle << "\n";
  std::cout << "======================================================================\n\n";

  for (G4double angleDeg : testAnglesDeg) {
    G4double angleRad = angleDeg * deg;
    primaryGen->SetIncidentAngle(angleRad);
    gCurrentTally.Reset();
    gCurrentTally.numIncident = nEventsPerAngle;

    runManager->BeamOn(nEventsPerAngle);

    G4double totalTransRate = (nEventsPerAngle > 0) ?
      static_cast<double>(gCurrentTally.numTotalTransmitted) / nEventsPerAngle : 0.0;
    G4double uncollidedRate = (nEventsPerAngle > 0) ?
      static_cast<double>(gCurrentTally.numUncollided) / nEventsPerAngle : 0.0;
    G4double scatteredRate = (nEventsPerAngle > 0) ?
      static_cast<double>(gCurrentTally.numScattered) / nEventsPerAngle : 0.0;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[入射倾角: " << std::setw(4) << angleDeg << "°] "
              << "总透射数: " << gCurrentTally.numTotalTransmitted
              << " (全能未碰撞: " << gCurrentTally.numUncollided
              << ", 多次散射: " << gCurrentTally.numScattered << ")\n";
    std::cout << "  全能透射率 T_tot = " << std::scientific << std::setprecision(4)
              << totalTransRate
              << " (T_uncollided = " << uncollidedRate
              << ", T_scattered = " << scatteredRate << ")\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  多次散射透射角分布 (出射极角 theta_out 分箱统计):\n";
    for (int b = 0; b < AngleTally::NBINS; ++b) {
      double pct = (gCurrentTally.numScattered > 0) ?
        100.0 * gCurrentTally.angularBins[b] / gCurrentTally.numScattered : 0.0;
      std::cout << "    [" << std::setw(2) << b*10 << "° - " << std::setw(2) << (b+1)*10 << "°]: "
                << std::setw(5) << gCurrentTally.angularBins[b] << " (" << std::setw(5) << pct << "%)\n";
    }
    std::cout << "\n";
  }

  std::cout << "======================================================================\n";
  std::cout << "[T1-M3 Arm C] 仿真计算全部顺利完成 Exit 0.\n";
  std::cout << "======================================================================\n";

  delete runManager;
  std::_Exit(0);
}
