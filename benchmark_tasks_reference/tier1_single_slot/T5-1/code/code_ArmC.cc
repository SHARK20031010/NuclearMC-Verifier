// T5-1: TEPC 组织等效正比计数器微剂量线能谱模拟
// 针对 PHYS-0027 缺陷修正：依据 ICRU 微剂量学严格定义，线能 y = epsilon / l_bar
// 球形灵敏体积平均弦长为 l_bar = 4V/S = 4r/3 = (2/3)d，而非直径 d
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4NistManager.hh"
#include "G4Orb.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "QGSP_BIC_HP.hh"
#include <cmath>

static const int    kNBin = 40;            // 0-200 keV/um, 5 keV/um per bin
static const double kBinW = 5.;            // keV/um
static const double kRadius = 0.5 * um;    // 1 um 直径微球，半径 r = 0.5 um
// ICRU 定义球形灵敏体积平均弦长: l_bar = 4V/S = 4r/3 = (2/3)d
static const double kMeanChord = (4.0 / 3.0) * kRadius;
static double fSpectrum[kNBin];
static long   fNhit = 0;
static double fSumY = 0., fEdep = 0.;

// 几何构建：微米级组织等效球形灵敏体积 (TEPC site) 放置于组织等效球内
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4Material* te = G4NistManager::Instance()->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
    auto* lWorld = new G4LogicalVolume(new G4Orb("World", 50. * um), te, "World");
    auto* pWorld = new G4PVPlacement(nullptr, {}, lWorld, "World", nullptr, false, 0);
    auto* lCav = new G4LogicalVolume(new G4Orb("Cavity", kRadius), te, "Cavity");
    new G4PVPlacement(nullptr, {}, lCav, "Cavity", lWorld, false, 0);
    return pWorld;
  }
};

// 步进动作：统计每个事件在灵敏腔 (Cavity) 中的总沉积能量
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4VPhysicalVolume* pv = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv || pv->GetName() != "Cavity") return;
    G4double e = step->GetTotalEnergyDeposit();
    if (e > 0.) fEdep += e;
  }
};

// 初级粒子：快中子在壁中弹性散射产生的反冲质子 (0..En 均匀谱，各向同性)
class Primary : public G4VUserPrimaryGeneratorAction {
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    double th = std::acos(2. * G4UniformRand() - 1.);
    double ph = CLHEP::twopi * G4UniformRand();
    fGun->SetParticleEnergy(G4UniformRand() * 1. * MeV + 1. * eV);
    fGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
    fGun->SetParticleMomentumDirection(
        G4ThreeVector(std::sin(th) * std::cos(ph), std::sin(th) * std::sin(ph), std::cos(th)));
    fGun->GeneratePrimaryVertex(ev);
  }
private:
  G4ParticleGun* fGun;
};

// 事件动作：事件结束时根据平均弦长计算线能 y = edep / l_bar 并填谱
class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { fEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (fEdep > 0.) {
      // 针对 PHYS-0027 缺陷修正：分母必须使用 ICRU 平均弦长 4r/3，而非直径 d
      double y = (fEdep / keV) / (kMeanChord / um);
      fNhit++;
      fSumY += y;
      int ib = static_cast<int>(y / kBinW);
      if (ib >= kNBin) ib = kNBin - 1;
      fSpectrum[ib] += 1.;
    }
  }
};

// 运行动作：初始化能谱并在运行结束时输出线能谱统计结果
class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    fNhit = 0;
    fSumY = 0.;
    for (int i = 0; i < kNBin; ++i) fSpectrum[i] = 0.;
  }
  void EndOfRunAction(const G4Run*) override {
    G4cout << "\n# TEPC lineal-energy spectrum: 1 MeV neutron field, 1 um TE site\n"
           << "# Mean chord length l_bar = 4r/3 = " << kMeanChord / um << " um\n"
           << "# y_mid[keV/um]   counts\n";
    for (int i = 0; i < kNBin; ++i)
      G4cout << (i + 0.5) * kBinW << "   " << fSpectrum[i] << G4endl;
    G4cout << "# hit events = " << fNhit
           << "   mean y = " << (fNhit ? fSumY / fNhit : 0.) << " keV/um" << G4endl;
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100;
  G4Random::setTheSeed(20240);
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new RunAction);
  rm->SetUserAction(new EventAction);
  rm->SetUserAction(new Stepping);
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
