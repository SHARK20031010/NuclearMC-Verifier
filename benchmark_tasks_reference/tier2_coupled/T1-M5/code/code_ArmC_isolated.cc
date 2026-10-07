/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M5 目标几何空间出射面", "src": "U"},
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

// ==============================================================================
// 蒙特卡洛任务 T1-M5 (Tier 2 · Arm C 第一性原理修复重构)
// 题目需求：圆柱形放射性废液储罐（1cm不锈钢 + 20cm水 + 2cm铅屏蔽）内 137Cs 源
// 计算罐外侧表面沿轴向高度分布的剂量率/通量剖面 (Profile along axial height z)
// ==============================================================================

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Run.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>

// ------------------------------------------------------------------------------
// 全局物理计分量：沿轴向高度 z 分箱剖面与权重统计
// ------------------------------------------------------------------------------
static const G4int NUM_Z_BINS = 20;
static G4double gZProfile[NUM_Z_BINS] = {0.0};
static G4double gTotalPenetratingWeight = 0.0;
static G4VPhysicalVolume* gDetectorPV = nullptr;
static G4VPhysicalVolume* gLeadPV = nullptr;

// ------------------------------------------------------------------------------
// 几何构建：同心圆柱层状复合屏蔽结构
// 废液水体 (R=0~50cm) -> 1cm 不锈钢内胆 (R=50~51cm) -> 20cm 轻水屏蔽 (R=51~71cm)
// -> 2cm 铅屏蔽 (R=71~73cm) -> 罐外表面出射监测层 (R=73~74cm)
// ------------------------------------------------------------------------------
class DetConstructionT1M5 : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
    G4Material* water = nist->FindOrBuildMaterial("G4_WATER");
    G4Material* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");
    G4Material* lead = nist->FindOrBuildMaterial("G4_Pb");

    // 世界体积
    auto* worldSolid = new G4Box("World", 2.0*m, 2.0*m, 2.0*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "WorldPV", nullptr, false, 0);

    const G4double halfHeight = 100.0 * cm; // 总高度 200 cm
    const G4double r0 = 50.0 * cm;          // 放射性废液内半径
    const G4double r1 = 51.0 * cm;          // 不锈钢内胆外半径 (厚度 1 cm)
    const G4double r2 = 71.0 * cm;          // 轻水屏蔽外半径 (厚度 20 cm)
    const G4double r3 = 73.0 * cm;          // 铅屏蔽外半径 (厚度 2 cm)
    const G4double r4 = 74.0 * cm;          // 探测采样薄层外半径 (厚度 1 cm)

    // 1. 废液水体区 (水 + 137Cs)
    auto* liquidSolid = new G4Tubs("LiquidSolid", 0.0, r0, halfHeight, 0.0, 360.0*deg);
    auto* liquidLog = new G4LogicalVolume(liquidSolid, water, "LiquidLog");
    new G4PVPlacement(nullptr, {}, liquidLog, "WasteLiquidPV", worldLog, false, 0);

    // 2. 内胆 1 cm 不锈钢屏蔽层
    auto* steelSolid = new G4Tubs("SteelSolid", r0, r1, halfHeight, 0.0, 360.0*deg);
    auto* steelLog = new G4LogicalVolume(steelSolid, steel, "SteelLog");
    new G4PVPlacement(nullptr, {}, steelLog, "SteelLinerPV", worldLog, false, 0);

    // 3. 外包 20 cm 轻水屏蔽层
    auto* waterSolid = new G4Tubs("WaterSolid", r1, r2, halfHeight, 0.0, 360.0*deg);
    auto* waterLog = new G4LogicalVolume(waterSolid, water, "WaterLog");
    new G4PVPlacement(nullptr, {}, waterLog, "WaterShieldPV", worldLog, false, 0);

    // 4. 最外层 2 cm 铅屏蔽层
    auto* leadSolid = new G4Tubs("LeadSolid", r2, r3, halfHeight, 0.0, 360.0*deg);
    auto* leadLog = new G4LogicalVolume(leadSolid, lead, "LeadLog");
    gLeadPV = new G4PVPlacement(nullptr, {}, leadLog, "LeadShieldPV", worldLog, false, 0);

    // 5. 罐外侧表面通量监测层
    auto* detSolid = new G4Tubs("DetSolid", r3, r4, halfHeight, 0.0, 360.0*deg);
    auto* detLog = new G4LogicalVolume(detSolid, air, "DetLog");
    gDetectorPV = new G4PVPlacement(nullptr, {}, detLog, "ScoringDetectorPV", worldLog, false, 0);

    return worldPV;
  }
};

// ------------------------------------------------------------------------------
// 物理列表：高精度标准电磁 Option4 (光电吸收、康普顿散射、电子对生成)
// ------------------------------------------------------------------------------
class PhysicsListT1M5 : public G4VModularPhysicsList {
public:
  PhysicsListT1M5() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
  }
};

// ------------------------------------------------------------------------------
// 初级粒子发生器：均匀溶于废液中的 137Cs 放射源 (0.662 MeV 伽马射线)
// 柱坐标均匀体源抽样：相空间微元测度保持 r = R * sqrt(xi) 与 4pi 各向同性发射
// ------------------------------------------------------------------------------
class PrimaryGeneratorT1M5 : public G4VUserPrimaryGeneratorAction {
public:
  PrimaryGeneratorT1M5() : fGun(new G4ParticleGun(1)) {
    auto* particle = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
    fGun->SetParticleDefinition(particle);
    fGun->SetParticleEnergy(0.662 * MeV); // 137Cs 衰变 662 keV 主特征伽马射线
  }
  ~PrimaryGeneratorT1M5() override { delete fGun; }

  void GeneratePrimaries(G4Event* anEvent) override {
    const G4double R = 50.0 * cm;
    const G4double halfHeight = 100.0 * cm;

    // 极坐标面微元雅可比变换：半径抽样必须开根号保持面/体积测度守恒
    G4double r = R * std::sqrt(G4UniformRand());
    G4double phi = 2.0 * M_PI * G4UniformRand();
    G4double x = r * std::cos(phi);
    G4double y = r * std::sin(phi);
    G4double z = (2.0 * G4UniformRand() - 1.0) * halfHeight;

    fGun->SetParticlePosition(G4ThreeVector(x, y, z));
    // 4pi 立体角严格各向同性发射
    fGun->SetParticleMomentumDirection(G4RandomDirection());
    fGun->GeneratePrimaryVertex(anEvent);
  }

private:
  G4ParticleGun* fGun;
};

// ------------------------------------------------------------------------------
// 步进计分器：几何边界跨越 PostStepPoint 判定与轴向高度 z 剖面统计
// ------------------------------------------------------------------------------
class SteppingActionT1M5 : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* postPoint = step->GetPostStepPoint();
    auto* prePoint = step->GetPreStepPoint();

    // 几何边界跨越契约：必须在 PostStepPoint 上判定 fGeomBoundary
    if (postPoint->GetStepStatus() == fGeomBoundary) {
      auto* prePV = prePoint->GetPhysicalVolume();
      auto* postPV = postPoint->GetPhysicalVolume();

      // 从铅屏蔽体出射至罐外探测薄层
      if ((prePV == gLeadPV || (prePV && prePV->GetName() == "LeadShieldPV")) &&
          (postPV == gDetectorPV || (postPV && postPV->GetName() == "ScoringDetectorPV"))) {

        G4double z = postPoint->GetPosition().z();
        const G4double halfHeight = 100.0 * cm;
        const G4double totalHeight = 2.0 * halfHeight;
        const G4double binWidth = totalHeight / NUM_Z_BINS;

        G4int bin = static_cast<G4int>((z + halfHeight) / binWidth);
        if (bin >= 0 && bin < NUM_Z_BINS) {
          // 权重流守恒契约：深穿透与通量计分严格乘入粒子当前动态权重
          G4double weight = step->GetTrack()->GetWeight();
          gZProfile[bin] += weight;
          gTotalPenetratingWeight += weight;
        }

        // 记入出射通量后终止该粒子，防止多次反散射重复穿透出射面计分
        step->GetTrack()->SetTrackStatus(fStopAndKill);
      }
    }
  }
};

// ------------------------------------------------------------------------------
// 运行统计与剖面输出
// ------------------------------------------------------------------------------
class RunActionT1M5 : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* aRun) override {
    G4int nEvents = aRun->GetNumberOfEvent();
    if (nEvents <= 0) return;

    const G4double rOuter = 73.0 * cm;
    const G4double halfHeight = 100.0 * cm;
    const G4double totalHeight = 2.0 * halfHeight;
    const G4double binWidth = totalHeight / NUM_Z_BINS;
    const G4double binArea = 2.0 * M_PI * rOuter * binWidth / (cm * cm); // 侧面积 cm^2

    std::cout << "\n================ [T1-M5 轴向高度剂量率/通量剖面 profile] ================" << std::endl;
    std::cout << "总发射源粒子数: " << nEvents << std::endl;
    std::cout << "外表面穿透总权重: " << gTotalPenetratingWeight << std::endl;
    std::cout << "总出射透射率: " << (gTotalPenetratingWeight / nEvents) << std::endl;
    std::cout << "------------------------------------------------------------------------" << std::endl;
    std::cout << std::setw(6) << "Bin" 
              << std::setw(16) << "z_height (cm)" 
              << std::setw(18) << "Weighted Counts" 
              << std::setw(24) << "Fluence (cm^-2/src)" << std::endl;
    std::cout << "------------------------------------------------------------------------" << std::endl;

    for (G4int i = 0; i < NUM_Z_BINS; ++i) {
      G4double zCenter = (-halfHeight + (i + 0.5) * binWidth) / cm;
      G4double fluence = (gZProfile[i] / nEvents) / binArea;
      std::cout << std::setw(6) << i
                << std::setw(16) << std::fixed << std::setprecision(2) << zCenter
                << std::setw(18) << std::setprecision(2) << gZProfile[i]
                << std::setw(24) << std::scientific << std::setprecision(4) << fluence << std::endl;
    }
    std::cout << "========================================================================\n" << std::endl;
  }
};

// ------------------------------------------------------------------------------
// 主函数
// ------------------------------------------------------------------------------
int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;

  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);
  runManager->SetUserInitialization(new DetConstructionT1M5());
  runManager->SetUserInitialization(new PhysicsListT1M5());
  runManager->SetUserAction(new PrimaryGeneratorT1M5());
  runManager->SetUserAction(new SteppingActionT1M5());
  runManager->SetUserAction(new RunActionT1M5());

  runManager->Initialize();
  runManager->BeamOn(nEvents);

  std::_Exit(0);
}
