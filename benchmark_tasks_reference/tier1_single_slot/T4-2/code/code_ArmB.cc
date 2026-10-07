// NaI(Tl) 探测器测 662 keV 伽马（Cs-137）能谱 —— 单文件 Geant4 程序
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
// 运行: ./code [nEvents] (默认 100000) -> 导出 nai_spectrum.csv
#include "G4RunManagerFactory.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4UserEventAction.hh"
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "Randomize.hh"
#include <cstdio>
#include <cmath>
#include <cstdlib>

static const G4int kNbins = 280;          // 2.5 keV/道，0 – 700 keV
static const G4double kEmax = 700. * keV;
static G4LogicalVolume* gNaILV = nullptr; // NaI 晶体逻辑体
static G4double gEdep = 0.;               // 单事件晶体总沉积能量
static G4int gSpec[kNbins] = {0};         // 沉积能谱计数
static G4int gHits = 0;                   // 晶体有效响应事件数

// 几何构建：3"×3" 圆柱 NaI(Tl) 晶体，中心位于 z = +8 cm
class MyDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 20. * cm, 20. * cm, 20. * cm),
                                        nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    gNaILV = new G4LogicalVolume(new G4Tubs("NaI", 0., 3.81 * cm, 3.81 * cm, 0., twopi),
                                 nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "NaI");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 8. * cm), gNaILV, "NaI", worldLV, false, 0);
    return worldPV;
  }
};

// 物理列表：高精度电磁 Opt4 模型（光电效应、康普顿散射等）
class Physics : public G4VModularPhysicsList {
public:
  Physics() { SetVerboseLevel(0); RegisterPhysics(new G4EmStandardPhysics_option4()); }
};

// 初级粒子源：(0, 0, -8 cm) 点源向 4pi 立体角各向同性发射 662 keV 伽马
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticleEnergy(662. * keV);
    fGun->SetParticlePosition(G4ThreeVector(0., 0., -8. * cm));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    // 4pi 空间各向同性立体角抽样 (cosTheta in [-1, 1], phi in [0, 2pi])
    G4double cosTheta = 2.0 * G4UniformRand() - 1.0;
    G4double phi = twopi * G4UniformRand();
    G4double sinTheta = std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));
    fGun->SetParticleMomentumDirection(
        G4ThreeVector(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta));
    fGun->GeneratePrimaryVertex(ev);
  }
};

// 步进动作：累加 NaI 晶体内各步的能量沉积
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() == gNaILV)
      gEdep += step->GetTotalEnergyDeposit();
  }
};

// 事件动作：记录晶体内总沉积能量，施加阈值保护避免零道噪声
class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (gEdep > 0.) { // 阈值保护：剔除未作用/穿透事件产生的 0 道伪峰
      ++gHits;
      G4int ib = static_cast<G4int>(gEdep / kEmax * kNbins);
      if (ib >= 0 && ib < kNbins) gSpec[ib]++;
    }
  }
};

// 运行动作：输出统计结果并导出 CSV 能谱文件
class RunAction : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* run) override {
    G4int n = run->GetNumberOfEvent();
    std::printf("\n=== NaI(Tl) 662 keV 伽马能谱 ===\n发射: %d | 响应: %d | 效率: %.4f%%\n",
                n, gHits, n ? 100.0 * gHits / n : 0.0);
    FILE* f = std::fopen("nai_spectrum.csv", "w");
    if (!f) return;
    std::fprintf(f, "energy_keV,counts\n");
    for (G4int i = 0; i < kNbins; ++i)
      std::fprintf(f, "%.2f,%d\n", (i + 0.5) * kEmax / kNbins / keV, gSpec[i]);
    std::fclose(f);
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100000;
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
  rm->SetUserInitialization(new MyDetector());
  rm->SetUserInitialization(new Physics());
  rm->SetUserAction(new Primary()); rm->SetUserAction(new Stepping());
  rm->SetUserAction(new EventAction()); rm->SetUserAction(new RunAction());
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
