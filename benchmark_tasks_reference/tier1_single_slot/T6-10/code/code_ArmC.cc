// ============================================================================
// 蒙特卡洛任务 T6-10 - C组（认知护栏与老师傅插件针对性干预修正版）
// 
// 物理问题诊断与修正说明 (PHYS-0066):
// 原代码在 UserSteppingAction 中对所有次级粒子 (TrackID > 1) 的每一个步点都
// 执行了产生顶点累加，未做首步过滤。这导致次级强子沿径迹行进过程中的数百个
// 连续输运步点全被误当作"产生顶点"记录，使空间产生密度分布严重虚高两数量级。
//
// 修正方案：
// 增加 step->GetTrack()->GetCurrentStepNumber() == 1 条件判断，
// 严格限制仅在次级粒子诞生的第一步（首步）记录其产生顶点坐标与分布统计。
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
#include "Randomize.hh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>

// 全局统计量：产生顶点总数、平均产生深度与一维深度分布
static G4long gVertices = 0;
static G4double gTotalZ = 0.0;
static const int NBINS_Z = 10;
static G4long gZHistogram[NBINS_Z] = {0};
static const G4double TARGET_Z_MIN = -25.0 * cm;
static const G4double TARGET_Z_MAX =  25.0 * cm;

// 靶几何构建：铁靶 (Fe) 50 cm x 50 cm x 50 cm
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(
        new G4Box("Target_W", 25.0 * cm, 25.0 * cm, 25.0 * cm),
        nist->FindOrBuildMaterial("G4_Fe"),
        "Target_W");
    return new G4PVPlacement(nullptr, G4ThreeVector(), world, "Target_W", nullptr, false, 0);
  }
};

// 物理过程列表
class Phys : public G4VModularPhysicsList {
public:
  Phys() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
  }
};

// 初级粒子源：1 GeV 质子垂直轰击靶面
class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.0 * GeV);
    gun.SetParticlePosition(G4ThreeVector(0.0, 0.0, -20.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
    gun.GeneratePrimaryVertex(ev);
  }
};

// 步进动作：统计次级粒子产生顶点的空间分布
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    G4Track* track = s->GetTrack();
    
    // 依据 PHYS-0066 进行关键物理修正：
    // 1. track->GetTrackID() > 1 过滤初级入射粒子，仅关注产生的次级粒子
    // 2. track->GetCurrentStepNumber() == 1 严格限制仅在产生首步记录顶点位置
    //    彻底消除在后续输运步点中重复累加导致空间产生密度分布严重虚高两个数量级的缺陷
    if (track->GetTrackID() > 1 && track->GetCurrentStepNumber() == 1) {
      gVertices++;
      
      G4ThreeVector vtxPos = track->GetVertexPosition();
      G4double z = vtxPos.z();
      gTotalZ += z;
      
      // 统计沿深度 z 的产生密度分布箱
      G4double binWidth = (TARGET_Z_MAX - TARGET_Z_MIN) / NBINS_Z;
      int binIdx = static_cast<int>((z - TARGET_Z_MIN) / binWidth);
      if (binIdx >= 0 && binIdx < NBINS_Z) {
        gZHistogram[binIdx]++;
      }
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  
  rm->Initialize();
  int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100;
  rm->BeamOn(nEvents);
  
  std::cout << "T6-10 Vertex counts = " << gVertices << std::endl;
  if (gVertices > 0) {
    std::cout << "Average production depth <z> = " << (gTotalZ / gVertices) / cm << " cm" << std::endl;
  }
  std::cout << "# Spatial production density distribution along z (-25cm to 25cm):" << std::endl;
  for (int i = 0; i < NBINS_Z; ++i) {
    double zLow = (TARGET_Z_MIN + i * (TARGET_Z_MAX - TARGET_Z_MIN) / NBINS_Z) / cm;
    double zHigh = (TARGET_Z_MIN + (i + 1) * (TARGET_Z_MAX - TARGET_Z_MIN) / NBINS_Z) / cm;
    std::cout << "[" << std::setw(5) << zLow << ", " << std::setw(5) << zHigh << ") cm: "
              << gZHistogram[i] << " vertices" << std::endl;
  }
  
  delete rm;
  return 0;
}
