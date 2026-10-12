// T4-3: 塑料闪烁体 5x5x5 cm 中 1 MeV 伽马 vs 1 MeV 中子 (Geant4 11.2.2, 单文件自带 main())
// 伽马=Compton/光电子 -> 低沉积、单脉冲、0.5 ns 快峰; 中子=质子反冲+2.2 MeV 俘获伽马 -> 高沉积、宽延迟谱(~4.5 ns 峰+长尾)
// 输出: 每类 50 事件的 <Edep>/<Npe>/相对涨落/最大沉积 + 1 ns 道闪烁光子发射时刻分布
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
static const G4int NEV = 50, NB = 20;   // 事件数 / 时间道数(1 ns)
namespace T43 {
struct Tally { G4int cur = 0, nsc[2] = {0, 0}, bin[2][NB] = {{0}}; G4double edep[2] = {0, 0}; };
thread_local Tally* tp = nullptr;
inline Tally* T() { if (!tp) tp = new Tally(); return tp; }
}
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* sc = new G4Material("PlasticScint", 1.032 * g / cm3, 2);           // 聚苯乙烯 C10H11
    sc->AddElement(nist->FindOrBuildElement("C"), 10); sc->AddElement(nist->FindOrBuildElement("H"), 11);
    const G4int N = 2; G4double en[N] = {1.0 * eV, 6.0 * eV}, rind[N] = {1.58, 1.58}, comp[N] = {1.0, 1.0};
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", en, rind, N); mpt->AddProperty("SCINTILLATIONCOMPONENT1", en, comp, N);  // 缺它则零光子
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV); mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0 * ns);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    sc->SetMaterialPropertiesTable(mpt);
    auto* world = new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("world", 20*cm, 20*cm, 20*cm),
        nist->FindOrBuildMaterial("G4_AIR"), "world"), "world", nullptr, false, 0);
    new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("sc", 2.5*cm, 2.5*cm, 2.5*cm), sc, "sc"), "sc",
        world->GetLogicalVolume(), false, 0);
    return world;
  }
};
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* tr = step->GetTrack(); auto* t = T43::T();
    if (tr->GetParticleDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) t->edep[t->cur] += step->GetTotalEnergyDeposit();
    else if (tr->GetCreatorProcess() && tr->GetCreatorProcess()->GetProcessName() == "Scintillation") {
      G4int b = (G4int)(tr->GetGlobalTime() / ns); t->nsc[t->cur]++;
      if (b >= 0 && b < NB) t->bin[t->cur][b]++;
    }
  }
};
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun gun;
public:
  Primary() : gun(1) {
    gun.SetParticleEnergy(1.0 * MeV); gun.SetParticlePosition({0, 0, -5 * cm}); gun.SetParticleMomentumDirection({0, 0, 1}); SetType(0);
  }
  void SetType(G4int k) {   // 0: 1 MeV 伽马, 1: 1 MeV 中子 (同能量, 只换粒子种类)
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
  auto* fac = new G4PhysListFactory; auto* pl = (G4VModularPhysicsList*)fac->GetReferencePhysList("QGSP_BIC_HP");  // 含中子 HP
  pl->RegisterPhysics(new G4OpticalPhysics);                               // 闪烁发光 + 光学光子输运
  rm->SetUserInitialization(pl); rm->SetUserInitialization(new Action); rm->Initialize();
  auto* gen = (Primary*)rm->GetUserPrimaryGeneratorAction(); auto* t = T43::T();
  G4double p2[2] = {0, 0}, mx[2] = {0, 0};                                 // 光子数平方和 / 最大单事件沉积
  for (G4int k = 0; k < 2; ++k) { gen->SetType(k);
    for (G4int e = 0; e < NEV; ++e) {
      G4double e0 = t->edep[k], n0 = t->nsc[k]; rm->BeamOn(1);
      G4double ed = t->edep[k] - e0, nn = t->nsc[k] - n0; p2[k] += nn * nn; if (ed > mx[k]) mx[k] = ed;
    }
  }
  const char* tag[2] = {"gamma 1MeV", "neutron 1MeV"};
  G4cout << "\n===== 塑料闪烁体 5x5x5 cm, 各 " << NEV << " 事件 =====" << G4endl;
  for (G4int k = 0; k < 2; ++k) { G4double av = t->nsc[k] / (G4double)NEV;
    G4cout << tag[k] << ": <Edep>=" << t->edep[k]/NEV/MeV << " MeV, <Npe>=" << av << " 光子, 相对涨落="
           << std::sqrt(p2[k]/NEV - av*av)/av << ", 最大单事件沉积=" << mx[k]/MeV << " MeV" << G4endl; }
  G4cout << "\n闪烁光子发射时刻分布 (1 ns 道, 计数/事件):\n t(ns)\t" << tag[0] << "\t" << tag[1] << G4endl;
  for (G4int b = 0; b < NB; ++b)
    G4cout << " " << b + 0.5 << "\t" << t->bin[0][b]/(G4double)NEV << "\t" << t->bin[1][b]/(G4double)NEV << G4endl;
  return 0;
}
