// T3-3: 辐照样品冷却后剩余剂量率计算 (Na-24 放射性核素点源)
// 针对 PHYS-0022 缺陷修正：补全 Na-24 衰变伴随的 2.754 MeV 主伽马线 (与 1.369 MeV 级联发射)
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4EmStandardPhysics.hh"
#include "G4EmParameters.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const G4double kR = 10.0 * cm, kT = 0.5 * cm; // 测量距离 10 cm, 壳半厚 0.5 cm
static const G4double kA0 = 1.0e9;                   // 停照初活度 1 GBq
static const G4double kT12 = 14.997 * 3600.0;        // Na-24 半衰期 (14.997 h)
static G4double gK = 0.0;                            // 径迹比释动能积分 [MeV*cm^3/g]

// 空气质能吸收系数 mu_en/rho [cm^2/g] (NIST: 1.37 MeV: 0.0266, 2.75 MeV: 0.0217)
static inline G4double GetMuEnAir(G4double eMeV) {
  return (eMeV >= 2.0) ? 0.0217 : ((eMeV >= 1.0) ? 0.0266 : 0.0280);
}

// 灵敏探测器: 径迹长度空气比释动能 (Air Kerma) 计分器
class Sens : public G4VSensitiveDetector {
public:
  Sens() : G4VSensitiveDetector("shell") {}
  G4bool ProcessHits(G4Step* st, G4TouchableHistory*) override {
    if (st->GetTrack()->GetDefinition() == G4Gamma::Gamma()) {
      G4double e = st->GetTrack()->GetKineticEnergy() / MeV;
      gK += (st->GetStepLength() / cm) * e * GetMuEnAir(e);
    }
    return true;
  }
};

// 探测器几何: 世界与 10 cm 处球形空气薄壳 (9.5~10.5 cm)
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    auto* lw = new G4LogicalVolume(new G4Box("w", 30*cm, 30*cm, 30*cm), air, "world");
    auto* ld = new G4LogicalVolume(new G4Sphere("sD", kR-kT, kR+kT, 0, 360*deg, 0, 180*deg), air, "doseShell");
    new G4PVPlacement(nullptr, {}, ld, "pD", lw, false, 0);
    auto* sd = new Sens;
    G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    ld->SetSensitiveDetector(sd);
    return new G4PVPlacement(nullptr, {}, lw, "physWorld", nullptr, false, 0);
  }
};

// 初级粒子发生器: Na-24 衰变伴随发射两条级联伽马射线 (点源各向同性发射)
class Prim : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Prim() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticlePosition(G4ThreeVector());
  }
  ~Prim() override { delete fGun; }

  void GeneratePrimaries(G4Event* ev) override {
    auto emitGamma = [&](G4double energy) {
      G4double cz = 2.0 * G4UniformRand() - 1.0, ph = 2.0 * M_PI * G4UniformRand();
      G4double rho = std::sqrt(std::max(0.0, 1.0 - cz * cz));
      fGun->SetParticleEnergy(energy);
      fGun->SetParticleMomentumDirection(G4ThreeVector(rho * std::cos(ph), rho * std::sin(ph), cz));
      fGun->GeneratePrimaryVertex(ev);
    };
    // 针对 PHYS-0022 缺陷修正：补全 1.369 MeV (100%) 与 2.754 MeV (99.9%) 级联跃迁
    emitGamma(1.369 * MeV);
    if (G4UniformRand() < 0.999) emitGamma(2.754 * MeV); // 贡献 62.1% 的比释动能率
  }
};

// 运行动作: 统计并输出不同冷却时间下的剩余活度及空气比释动能率
class RunAct : public G4UserRunAction {
  G4double fTc = 0.0;
public:
  void SetTcool(G4double t) { fTc = t; }
  void BeginOfRunAction(const G4Run*) override { gK = 0.0; }
  void EndOfRunAction(const G4Run* run) override {
    G4double vol = (4.0/3.0) * M_PI * (std::pow((kR+kT)/cm, 3) - std::pow((kR-kT)/cm, 3));
    G4double lam = std::log(2.0) / kT12, A = kA0 * std::exp(-lam * fTc);
    G4double kerma = gK / vol * 1.602176634e-10 / run->GetNumberOfEvent(), rate = kerma * A;
    std::printf("t_cool=%10.4e s (%7.3f d)  A(t)=%9.3e Bq  D_dot=%9.3e Gy/s = %9.3f uGy/h\n",
                fTc, fTc / 86400.0, A, rate, rate * 3600.0 / 1e-6);
  }
};

int main(int argc, char** argv) {
  G4int nev = (argc > 1) ? std::atoi(argv[1]) : 20000;
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  auto* pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics());
  pl->SetVerboseLevel(0);
  G4EmParameters::Instance()->SetVerbose(0);
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  auto* ra = new RunAct;
  rm->SetUserAction(ra);
  rm->Initialize();
  std::printf("--- T3-3 Na-24 剩余剂量率: A0=%.3g Bq, T1/2=%.3f h, 级联(1.369+2.754 MeV), R=%.1f cm ---\n",
              kA0, kT12 / 3600.0, kR / cm);
  const G4double tc[5] = {0.0, 3600.0, 6.0 * 3600.0, 86400.0, 7.0 * 86400.0};
  for (int i = 0; i < 5; ++i) { ra->SetTcool(tc[i]); rm->BeamOn(nev); }
  delete rm;
  return 0;
}
