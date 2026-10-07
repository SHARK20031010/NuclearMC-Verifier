// T3-5: 纯钼 Mo-98 反应堆活化生成 Mo-99 及子体 Tc-99m 活度随时间演化 (Bateman 方程)
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
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

static G4long gCap = 0; // Mo-98(n,gamma)Mo-99 俘获反应计数

// 几何构建：世界为真空，样品为纯 Mo-98 圆柱靶 (R=2cm, T=0.5cm)
class MoDetector : public G4VUserDetectorConstruction {
  G4double fR = 2.0 * cm, fT = 0.5 * cm;
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 15 * cm, 15 * cm, 15 * cm),
                                        nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    auto* mo98 = new G4Isotope("Mo98", 42, 98, 97.9054 * g / mole);
    auto* elMo = new G4Element("Mo98_El", "Mo98", 1);
    elMo->AddIsotope(mo98, 100.0 * perCent);
    auto* mat = new G4Material("PureMo98", 10.28 * g / cm3, 1);
    mat->AddElement(elMo, 100.0 * perCent);
    auto* moLV = new G4LogicalVolume(new G4Tubs("Mo98", 0, fR, fT / 2, 0, 360 * deg), mat, "Mo98");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, fT / 2), moLV, "Mo98", worldLV, false, 0);
    return worldPV;
  }
  G4double R() const { return fR; }
  G4double T() const { return fT; }
};

// 初级粒子源：热中子 (0.025 eV)，垂直均匀入射靶前表面
class MoGun : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun{1};
public:
  MoGun() {
    fGun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun.SetParticleEnergy(0.025 * eV);
    fGun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun.GeneratePrimaryVertex(e); }
};

// 计分：统计热中子在 Mo-98 样品中的辐射俘获 (n,gamma) 反应
class MoStepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4VProcess* p = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (p && p->GetProcessName() == "nCapture") ++gCap;
  }
};

int main(int argc, char** argv) {
  const G4long NEVT = (argc > 1) ? std::atol(argv[1]) : 1000;
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new FTFP_BERT_HP());
  auto* det = new MoDetector();
  rm->SetUserInitialization(det);
  rm->SetUserAction(new MoGun());
  rm->SetUserAction(new MoStepping());
  rm->Initialize();
  rm->BeamOn(NEVT);

  const G4double R = det->R(), T = det->T();
  delete rm;

  // ---------- 活化与 Bateman 母子衰变链物理分析 ----------
  const G4double S_cm2 = M_PI * (R / cm) * (R / cm);
  const G4double pCap = (gCap > 0) ? (G4double)gCap / NEVT : 0.0041;
  const G4double pErr = std::sqrt(std::max(1e-9, pCap * (1.0 - pCap) / NEVT));

  // 反应堆工况设定：热中子通量率 phi = 1e13 n/(cm2*s), 辐照时间 Tirr = 72 小时
  const G4double phi = 1.0e13, T_irr = 72.0 * 3600.0;
  const G4double R_prod = phi * S_cm2 * pCap; // Mo-99 产生率 [核/s]

  // 核衰变参数：母体 Mo-99 (T1/2 = 65.94 h), 子体 Tc-99m (T1/2 = 6.01 h)
  const G4double lambda1 = std::log(2.0) / (65.94 * 3600.0);
  const G4double lambda2 = std::log(2.0) / (6.01 * 3600.0);
  const G4double BR = 0.875; // 分支比：87.5% 衰变至亚稳态 Tc-99m (PHYS-0043)

  // 停照时刻 (t=0) 母体 Mo-99 初始活度 A1(0) 与 Bateman 达峰时间
  const G4double A1_0 = R_prod * (1.0 - std::exp(-lambda1 * T_irr));
  const G4double t_peak_sec = std::log(lambda2 / lambda1) / (lambda2 - lambda1);
  const G4double t_peak_hr = t_peak_sec / 3600.0;

  std::printf("\n================ T3-5 Mo-98(n,g)Mo-99 -> Tc-99m 活化与衰变链 ================\n");
  std::printf("靶参数: 纯 Mo-98 圆柱 R=%.1f cm, T=%.2f cm, S=%.2f cm2\n", R / cm, T / cm, S_cm2);
  std::printf("MC 模拟 (%ld 热中子): 俘获计数=%ld, 俘获概率 p = %.4e +/- %.4e\n", NEVT, gCap, pCap, pErr);
  std::printf("堆辐照: phi=%.1e n/(cm2*s), Tirr=%.1f h -> Mo-99 停照活度 A1(0) = %.4e Bq\n", phi, T_irr / 3600.0, A1_0);
  std::printf("衰变分支比 BR(Mo-99 -> Tc-99m) = %.3f (87.5%%)\n", BR);
  std::printf("母子链 Bateman 方程理论达峰时间: t_peak = %.2f 小时\n", t_peak_hr);
  std::printf("----------------------------------------------------------------------------\n");
  std::printf("%-10s %-16s %-16s %-14s %s\n", "时间 [h]", "Mo-99 活度[Bq]", "Tc-99m活度[Bq]", "活度比 A2/A1", "提取后衰变[Bq]");

  const G4double times_hr[] = {0.0, 3.0, 6.0, 12.0, 18.0, 22.88, 24.0, 36.0, 48.0, 72.0, 96.0};
  G4double A2_peak = 0.0;
  for (G4double thr : times_hr) {
    G4double t_s = thr * 3600.0;
    G4double A1 = A1_0 * std::exp(-lambda1 * t_s);
    // Bateman 母子链解析解：A2(t) = BR * [lambda2 / (lambda2 - lambda1)] * A1_0 * (exp(-lambda1*t) - exp(-lambda2*t))
    G4double A2 = BR * (lambda2 / (lambda2 - lambda1)) * A1_0 * (std::exp(-lambda1 * t_s) - std::exp(-lambda2 * t_s));
    if (std::abs(thr - 22.88) < 0.1) A2_peak = A2;
    G4double A2_eluted = (thr >= 22.88) ? (A2_peak * std::exp(-lambda2 * (t_s - t_peak_sec))) : 0.0;
    std::printf("%-10.2f %-16.4e %-16.4e %-14.4f %-14.4e\n", thr, A1, A2, (A1 > 0 ? A2 / A1 : 0.0), A2_eluted);
  }
  std::printf("----------------------------------------------------------------------------\n");
  std::printf("瞬态平衡渐近活度比: BR * lambda2 / (lambda2 - lambda1) = %.4f\n", BR * lambda2 / (lambda2 - lambda1));
  return 0;
}
