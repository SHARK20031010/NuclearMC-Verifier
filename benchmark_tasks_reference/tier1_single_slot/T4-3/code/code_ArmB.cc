// T4-3: 塑料闪烁体 5x5x5 cm 中 1 MeV 伽马 vs 1 MeV 中子响应对比 (Geant4 11.2.2, 单文件自带 main())
// 物理机制: 伽马通过轻子沉积能量，低 dE/dx 几无猝灭(~9500 ph/MeV)；中子通过反冲质子沉积，高 dE/dx 引发严重 Birks 猝灭(~1200 ph/MeV)。
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalPhoton.hh"
#include "G4PVPlacement.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysListFactory.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserSteppingAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"

static const G4int NEV = 50, NB = 20;   // 每组 50 事件 / 20 个 1 ns 时间道
namespace T43 {
struct Tally { G4int cur = 0, nsc[2] = {0, 0}, bin[2][NB] = {{0}}; G4double edep[2] = {0, 0}; };
thread_local Tally* tp = nullptr;
inline Tally* T() { if (!tp) tp = new Tally(); return tp; }
}

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* sc = new G4Material("PlasticScint", 1.032 * g / cm3, 2);           // 聚苯乙烯基底 C10H11
    sc->AddElement(nist->FindOrBuildElement("C"), 10); sc->AddElement(nist->FindOrBuildElement("H"), 11);
    sc->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);                 // 关键修正: 设置反冲质子 Birks 发光猝灭
    const G4int N = 2; G4double en[N] = {1.0 * eV, 6.0 * eV}, rind[N] = {1.58, 1.58}, comp[N] = {1.0, 1.0};
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", en, rind, N); mpt->AddProperty("SCINTILLATIONCOMPONENT1", en, comp, N);
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV); mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0 * ns);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    sc->SetMaterialPropertiesTable(mpt);
    auto* world = new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("world", 20*cm, 20*cm, 20*cm),
        nist->FindOrBuildMaterial("G4_AIR"), "world"), "world", nullptr, false, 0);
    new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("sc", 2.5*cm, 2.5*cm, 2.5*cm), sc, "sc"),
        "sc", world->GetLogicalVolume(), false, 0);
    return world;
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetPhysicalVolume()->GetName() != "sc") return; // 仅闪烁体内部计分
    auto* tr = step->GetTrack(); auto* t = T43::T();
    if (tr->GetParticleDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) {
      t->edep[t->cur] += step->GetTotalEnergyDeposit();
    } else if (tr->GetCurrentStepNumber() == 1) { // 仅首步计分，避免光子多次散射重复计数
      G4int b = (G4int)(tr->GetGlobalTime() / ns); t->nsc[t->cur]++;
      if (b >= 0 && b < NB) t->bin[t->cur][b]++;
      tr->SetTrackStatus(fStopAndKill);           // 记录产额与发射时刻后终止光子追踪，提升模拟效率
    }
  }
};

class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun gun;
public:
  Primary() : gun(1) {
    gun.SetParticleEnergy(1.0 * MeV);
    gun.SetParticlePosition({0, 0, -2.5 * cm});   // 修正: 粒子源位于闪烁体前表面，消除空气中虚假飞行时间延迟
    gun.SetParticleMomentumDirection({0, 0, 1}); SetType(0);
  }
  void SetType(G4int k) {
    T43::T()->cur = k; gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(k ? "neutron" : "gamma"));
  }
  void GeneratePrimaries(G4Event* ev) override { gun.GeneratePrimaryVertex(ev); }
};

class Action : public G4VUserActionInitialization {
public:
  void Build() const override { SetUserAction(new Primary); SetUserAction(new Stepping); }
};

int main() {
  auto* rm = new G4RunManager; rm->SetUserInitialization(new Detector);
  auto* fac = new G4PhysListFactory;
  auto* pl = (G4VModularPhysicsList*)fac->GetReferencePhysList("QGSP_BIC_HP");  // 高精度中子强子物理库
  pl->RegisterPhysics(new G4OpticalPhysics);                                   // 闪烁与光学物理过程
  rm->SetUserInitialization(pl); rm->SetUserInitialization(new Action); rm->Initialize();

  auto* gen = (Primary*)rm->GetUserPrimaryGeneratorAction(); auto* t = T43::T();
  G4double p2[2] = {0, 0}, mx[2] = {0, 0};
  for (G4int k = 0; k < 2; ++k) { gen->SetType(k);
    for (G4int e = 0; e < NEV; ++e) {
      G4double e0 = t->edep[k], n0 = t->nsc[k]; rm->BeamOn(1);
      G4double ed = t->edep[k] - e0, nn = t->nsc[k] - n0; p2[k] += nn * nn; if (ed > mx[k]) mx[k] = ed;
    }
  }

  const char* tag[2] = {"gamma 1MeV", "neutron 1MeV"};
  G4cout << "\n===== 塑料闪烁体 5x5x5 cm, 各 " << NEV << " 事件 =====" << G4endl;
  for (G4int k = 0; k < 2; ++k) {
    G4double av = t->nsc[k] / (G4double)NEV;
    G4double rel = (av > 0) ? std::sqrt(p2[k] / NEV - av * av) / av : 0.0;
    G4cout << tag[k] << ": <Edep>=" << t->edep[k]/NEV/MeV << " MeV, <Npe>=" << av
           << " 光子, 相对涨落=" << rel << ", 最大单事件沉积=" << mx[k]/MeV << " MeV" << G4endl;
  }
  G4cout << "\n闪烁光子发射时刻分布 (1 ns 道, 计数/事件):\n t(ns)\t" << tag[0] << "\t" << tag[1] << G4endl;
  for (G4int b = 0; b < NB; ++b)
    G4cout << " " << b + 0.5 << "\t" << t->bin[0][b]/(G4double)NEV << "\t" << t->bin[1][b]/(G4double)NEV << G4endl;

  delete rm;
  return 0;
}
