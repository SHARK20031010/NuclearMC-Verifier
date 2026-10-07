// Co-59 纯钴样品在中子场中辐照生成 Co-60，停照后活度随时间的衰变变化
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o T3-1
// 运行: ./T3-1 [nEvents] (默认 1000)
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4VProcess.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "FTFP_BERT_HP.hh"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static G4long gCap = 0;              // Co-59(n,gamma)Co-60 反应计数

// ---------- 几何: 纯钴圆柱样品, 热中子从前表面沿 +z 垂直入射 ----------
class CoDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* lvW = new G4LogicalVolume(new G4Box("World", 15 * cm, 15 * cm, 15 * cm),
                                    nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto* pvW = new G4PVPlacement(nullptr, {}, lvW, "World", nullptr, false, 0);
    fR = 2.5 * cm; fT = 1.0 * cm;                       // 半径 2.5 cm, 厚 1.0 cm
    auto* lvCo = new G4LogicalVolume(new G4Tubs("Co", 0, fR, fT / 2, 0, 360 * deg),
                                     nist->FindOrBuildMaterial("G4_Co"), "Co");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, fT / 2), lvCo, "Co", lvW, false, 0);
    return pvW;
  }
  G4double R() const { return fR; }
  G4double T() const { return fT; }
private:
  G4double fR = 0, fT = 0;
};

class CoGun : public G4VUserPrimaryGeneratorAction {
public:
  CoGun() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(0.025 * eV);                // 0.025 eV 热中子
    fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));  // 样品前表面中心
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~CoGun() override { delete fGun; }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
private:
  G4ParticleGun* fGun;
};

// 统计 Co-59(n,gamma)Co-60 的俘获次数
class CoStepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    const G4VProcess* p = s->GetPostStepPoint()->GetProcessDefinedStep();
    if (p && p->GetProcessName() == "nCapture") ++gCap;
  }
};

int main(int argc, char** argv) {
  const G4long NEVT = (argc > 1) ? std::atol(argv[1]) : 1000;
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new FTFP_BERT_HP);
  auto* det = new CoDetector;
  rm->SetUserInitialization(det);
  rm->SetUserAction(new CoGun);
  rm->SetUserAction(new CoStepping);
  rm->Initialize();
  rm->BeamOn(NEVT);

  const G4double R = det->R(), T = det->T();
  delete rm;

  // ---------- 活化与衰变物理计算 ----------
  const G4double S_cm2 = M_PI * (R / cm) * (R / cm);           // 样品截面 (cm2)
  const G4double V_cm3 = S_cm2 * (T / cm);                     // 样品体积 (cm3)
  const G4double pCap  = (G4double)gCap / NEVT;                // 单个入射中子俘获概率
  const G4double pErr  = std::sqrt(pCap * (1.0 - pCap) / NEVT);// 统计不确定度 (标准误差)
  const G4double phi   = 1.0e8;                                // 中子注量率 (n/cm2/s)
  // 有效反应率: R_tot = phi * S * pCap [1/s]; 比反应率: Rp = R_tot / V [1/cm3/s]
  const G4double R_tot = phi * S_cm2 * pCap;                   // 总产生率 (/s)
  const G4double Rp    = R_tot / V_cm3;                        // 比产生率 (/cm3/s) = phi * pCap / (T/cm)
  const G4double lam   = std::log(2.0) / (5.2714 * 365.25 * 86400.0); // Co-60 衰变常数 (/s)
  const G4double Tirr  = 30.0 * 86400.0;                       // 辐照时间: 30 天 (s)
  // 饱和活度公式: A(t) = R * (1 - exp(-lambda * t)), 活度已含 lambda (A = lambda * N), 严禁除以 lambda!
  const G4double satFactor = 1.0 - std::exp(-lam * Tirr);      // 饱和活化因子
  const G4double A0_tot = R_tot * satFactor;                   // 停照时刻总活度 (Bq)
  const G4double A0_vol = Rp * satFactor;                      // 停照时刻比活度 (Bq/cm3)

  std::printf("\n================ Co-59(n,g)Co-60 活化与衰变特征 ================\n");
  std::printf("样品参数: 圆柱纯钴 R=%.2f cm, T=%.2f cm -> 截面 S=%.3f cm2, 体积 V=%.3f cm3\n",
              R / cm, T / cm, S_cm2, V_cm3);
  std::printf("MC 统计 (%ld 热中子): 俘获数=%ld -> 俘获概率 p = %.4f +/- %.4f\n",
              NEVT, gCap, pCap, pErr);
  std::printf("辐照工况: phi=%.1e n/(cm2*s), 辐照时间 Tirr=%.1f 天 (饱和因子 1-e^{-lt} = %.4e)\n",
              phi, Tirr / 86400.0, satFactor);
  std::printf("停照活度: 总活度 A0 = %.4e Bq (%.3f MBq), 比活度 A0_vol = %.4e Bq/cm3\n",
              A0_tot, A0_tot * 1e-6, A0_vol);
  std::printf("----------------------------------------------------------------\n");
  std::printf("%-12s %-12s %-18s %s\n", "冷却时间", "衰变因子 A/A0", "总活度 [Bq]", "比活度 [Bq/cm3]");
  const G4double tc[] = {0, 1, 7, 30, 90, 180, 365, 730, 1825, 3650};
  for (G4double d : tc) {
    G4double f = std::exp(-lam * d * 86400.0);
    std::printf("%-10.0f d  %-12.4f %-18.4e %.4e\n", d, f, A0_tot * f, A0_vol * f);
  }
  std::printf("----------------------------------------------------------------\n");
  std::printf("半衰期 T1/2 = 5.2714 a; 停照后纯指数衰变 A(t) = A0 * exp(-lambda * t)\n");
  return 0;
}
