// NaI(Tl) 探测器测 662 keV 伽马（Cs-137）能谱 —— 单文件 Geant4 程序
// 针对 PHYS-0024/0029 修正：点源 4pi 各向同性发射；能谱填谱增加 edep > 0 阈值过滤
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
// 运行: ./code 100000 -> 输出 nai_spectrum.csv
#include "G4RunManager.hh"
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
#include "G4DecayPhysics.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4RandomDirection.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4UserEventAction.hh"
#include "G4UserRunAction.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include <cstdlib>
#include <cstdio>

static const G4int kNbins = 280;          // 2.5 keV/道，0 – 700 keV
static const G4double kEmax = 700. * keV;
static G4double gEdep = 0.;               // 本事件在 NaI 晶体内的总沉积能量
static G4int gSpec[kNbins] = {0};         // 能谱计数
static G4int gDetected = 0;               // 有效探测计数

// 几何：3"×3" 圆柱 NaI(Tl) 晶体；点源在轴线上，距晶体中心 16 cm
class MyDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 20.*cm, 20.*cm, 20.*cm),
                                        nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    auto* naiLV = new G4LogicalVolume(new G4Tubs("NaI", 0., 3.81*cm, 3.81*cm, 0., twopi),
                                      nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "NaI");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 8.*cm), naiLV, "NaI", worldLV, false, 0);
    return worldPV;
  }
};

// 物理：Opt4 标准电磁（低能光电/康普顿更准）+ 衰变
class Physics : public G4VModularPhysicsList {
public:
  Physics() {
    SetVerboseLevel(0);
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
  }
};

// 初级粒子：662 keV 伽马，位于 (0, 0, -8 cm)，4pi 各向同性发射 (PHYS-0024 修正)
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticleEnergy(662.*keV);
    fGun->SetParticlePosition(G4ThreeVector(0., 0., -8.*cm));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    fGun->SetParticleMomentumDirection(G4RandomDirection());
    fGun->GeneratePrimaryVertex(ev);
  }
};

// 步进：累加 NaI 晶体内总能量沉积
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* lv = step->GetTrack()->GetVolume()->GetLogicalVolume();
    if (lv && lv->GetName() == "NaI") gEdep += step->GetTotalEnergyDeposit();
  }
};

// 事件：每事件结束时填谱，添加 edep > 0 阈值过滤排除穿透零能伪峰 (PHYS-0029 修正)
class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (gEdep > 0.) {
      ++gDetected;
      G4int ib = (G4int)(gEdep / kEmax * kNbins);
      if (ib >= 0 && ib < kNbins) gSpec[ib]++;
    }
  }
};

// 运行：结束后把能谱写成 CSV 文件
class RunAction : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* run) override {
    if (FILE* f = std::fopen("nai_spectrum.csv", "w")) {
      std::fprintf(f, "energy_keV,counts\n");
      for (G4int i = 0; i < kNbins; ++i)
        std::fprintf(f, "%.2f,%d\n", (i + 0.5) * kEmax / kNbins / keV, gSpec[i]);
      std::fclose(f);
    }
    G4long n = run->GetNumberOfEvent();
    std::printf("\n=== Cs-137 662 keV NaI 能谱模拟 ===\n总发射: %ld, 有效探测: %d (绝对效率: %.4f%%)\n",
                n, gDetected, 100.0 * gDetected / (n > 0 ? n : 1));
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100000;
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new MyDetector());
  rm->SetUserInitialization(new Physics());
  rm->SetUserAction(new Primary());
  rm->SetUserAction(new Stepping());
  rm->SetUserAction(new EventAction());
  rm->SetUserAction(new RunAction());
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
