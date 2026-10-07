// T3-3: 活化样品冷却后的剩余剂量率 (Na-24点源, 离源10 cm处空气吸收剂量率)
// 修正重点: 补全Na-24主能级跃迁2.754 MeV与1.369 MeV级联伽马射线(分支比约100%),
// 并采用能量对应的空气质量能量吸收系数(mu_en/rho), 消除原代码低估2.59倍的缺陷。
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

static const G4double kR = 10.0 * cm, kT = 0.5 * cm; // 计分球壳半径与半厚度
static const G4double kA0 = 1.0e9;                   // 停照即刻活度 [Bq] (1 GBq)
static const G4double kT12 = 14.997 * 3600.0;        // Na-24 半衰期 [s]
static G4double gK = 0.0;                            // 径迹长度 Kerma 积分 [MeV*cm^3/g]

// NIST 干燥空气质量能量吸收系数 (mu_en/rho) [cm^2/g]
// 1.369 MeV -> 0.0261 cm^2/g, 2.754 MeV -> 0.0213 cm^2/g
inline G4double GetAirMuEnRho(G4double eMeV) {
  return (eMeV > 2.0) ? 0.0213 : 0.0261;
}

// 灵敏探测器：利用光子径迹长度估计空气碰撞比释动能 (Air Kerma)
class Sens : public G4VSensitiveDetector {
public:
  Sens() : G4VSensitiveDetector("shellSD") {}
  G4bool ProcessHits(G4Step* st, G4TouchableHistory*) override {
    if (st->GetTrack()->GetDefinition() != G4Gamma::Gamma()) return false;
    G4double e = st->GetPreStepPoint()->GetKineticEnergy() / MeV;
    G4double stepL = st->GetStepLength() / cm;
    gK += stepL * e * GetAirMuEnRho(e);
    return true;
  }
};

// 几何构建：空气世界体 + 10 cm 处球壳探测层
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    auto* lw = new G4LogicalVolume(new G4Box("World", 30 * cm, 30 * cm, 30 * cm), air, "World");
    auto* ld = new G4LogicalVolume(new G4Sphere("sD", kR - kT, kR + kT, 0, 360 * deg, 0, 180 * deg), air, "doseShell");
    new G4PVPlacement(nullptr, {}, ld, "pD", lw, false, 0);
    auto* sd = new Sens();
    G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    ld->SetSensitiveDetector(sd);
    return new G4PVPlacement(nullptr, {}, lw, "physWorld", nullptr, false, 0);
  }
};

// 初级粒子源：Na-24 衰变级联发射 1.369 MeV 与 2.754 MeV 两条主伽马射线 (4pi 各向同性)
class Prim : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Prim() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticlePosition(G4ThreeVector());
  }
  ~Prim() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    static const G4double energies[2] = {1.369 * MeV, 2.754 * MeV};
    for (int i = 0; i < 2; ++i) {
      G4double cz = 2.0 * G4UniformRand() - 1.0;
      G4double rho = std::sqrt(std::max(0.0, 1.0 - cz * cz));
      G4double ph = 2.0 * M_PI * G4UniformRand();
      fGun->SetParticleEnergy(energies[i]);
      fGun->SetParticleMomentumDirection(G4ThreeVector(rho * std::cos(ph), rho * std::sin(ph), cz));
      fGun->GeneratePrimaryVertex(ev);
    }
  }
};

// 运行动作：统计单次衰变剂量贡献，并根据放射性衰变定律外推多冷却时间的残余剂量率
class RunAct : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override { gK = 0.0; }
  void EndOfRunAction(const G4Run* run) override {
    G4double vol = (4.0 / 3.0) * M_PI * (std::pow((kR + kT) / cm, 3) - std::pow((kR - kT) / cm, 3)); // cm^3
    G4double kermaPerDecay = (gK / vol) * 1.602176634e-10 / run->GetNumberOfEvent(); // Gy per decay
    G4double lam = std::log(2.0) / kT12;
    std::printf("\n--- T3-3 residual dose rate: A0=%.3g Bq, Na-24(1.369+2.754 MeV), R=%.1f+-%.1f cm ---\n",
                kA0, kR / cm, kT / cm);
    std::printf("Air Kerma per decay = %.4e Gy/decay (Theory: 1.203e-14 Gy/decay)\n", kermaPerDecay);
    const G4double tc[5] = {0.0, 3600.0, 6.0 * 3600.0, 86400.0, 7.0 * 86400.0};
    for (int i = 0; i < 5; ++i) {
      G4double t = tc[i];
      G4double A = kA0 * std::exp(-lam * t);
      G4double rate = kermaPerDecay * A; // Gy/s
      std::printf("t_cool=%10.4e s (%9.4f d)  A(t)=%9.3e Bq  D_dot=%9.3e Gy/s = %9.3f uGy/h\n",
                  t, t / 86400.0, A, rate, rate * 3600.0 / 1e-6);
    }
  }
};

int main(int argc, char** argv) {
  G4int nev = (argc > 1) ? std::atoi(argv[1]) : 20000;
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Detector());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics());
  pl->SetVerboseLevel(0);
  G4EmParameters::Instance()->SetVerbose(0);
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new RunAct());
  rm->Initialize();
  rm->BeamOn(nev);
  delete rm;
  return 0;
}
