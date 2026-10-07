// T4-4 (Arm C): 死层厚度对探测效率的影响 (插件指导修正版)
// 依据 PHYS-0026 修正：死层为无电场不灵敏区，剥离死层沉积能量，仅当灵敏晶体内有能量沉积 (g_eCry > 0) 时计入探测事件。
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4Gamma.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "QGSP_BIC_HP.hh"
#include <cstdio>
#include <cstdlib>
#include <vector>

static const G4double kR = 2 * cm, kL = 3 * cm, kE = 1 * MeV, kGap = 2 * cm;
static const G4double kSrcZ = -8 * cm;
static const std::vector<G4double> kDead = {0., 0.1 * mm, 1. * mm, 3. * mm, 10. * mm};
static const G4int kN = (G4int)kDead.size();
static G4int g_cur = 0;
static G4double g_eCry = 0., g_eDead = 0.;
static std::vector<G4long> g_any(kN, 0), g_full(kN, 0);

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4double ed = step->GetTotalEnergyDeposit();
    if (ed <= 0.) return;
    const G4String n = step->GetPreStepPoint()->GetPhysicalVolume()->GetName();
    if (n == "crystal") g_eCry += ed;
    else if (n == "dead") g_eDead += ed;
  }
};

class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    g_eCry = 0.;
    g_eDead = 0.;
  }
  void EndOfEventAction(const G4Event*) override {
    if (g_eCry > 0.) g_any[g_cur]++;
    if (g_eCry > 0.95 * kE && g_eDead < 0.01 * kE) g_full[g_cur]++;
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::GammaDefinition());
    fGun->SetParticleEnergy(kE);
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~PrimaryGeneratorAction() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    g_eCry = g_eDead = 0.;
    fGun->SetParticlePosition(G4ThreeVector((g_cur - (kN - 1) / 2.) * kGap, 0, kSrcZ));
    fGun->GeneratePrimaryVertex(ev);
  }
};

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4NistManager* nist = G4NistManager::Instance();
    auto* ge = nist->FindOrBuildMaterial("G4_Ge");
    auto* vac = nist->FindOrBuildMaterial("G4_Galactic");
    auto* lvW = new G4LogicalVolume(new G4Box("world", 40 * cm, 20 * cm, 20 * cm), vac, "world");
    auto* pvW = new G4PVPlacement(nullptr, {}, lvW, "world", nullptr, false, 0);
    for (G4int i = 0; i < kN; ++i) {
      G4double d = kDead[i], cryT = kL - d, x = (i - (kN - 1) / 2.) * kGap, z0 = -kL / 2.;
      auto* crystal = new G4LogicalVolume(new G4Box("sc", kR, kR, cryT / 2.), ge, "crystal");
      new G4PVPlacement(nullptr, G4ThreeVector(x, 0, z0 + d + cryT / 2.), crystal, "crystal",
                        lvW, false, i);
      if (d > 0.) {
        auto* dl = new G4LogicalVolume(new G4Box("sd", kR, kR, d / 2.), ge, "dead");
        new G4PVPlacement(nullptr, G4ThreeVector(x, 0, z0 + d / 2.), dl, "dead", lvW, false, i);
      }
    }
    return pvW;
  }
};

int main(int argc, char** argv) {
  G4long n = (argc > 1) ? std::atol(argv[1]) : 100;
  auto* run = new G4RunManager;
  run->SetUserInitialization(new DetectorConstruction);
  run->SetUserInitialization(new QGSP_BIC_HP);
  run->SetUserAction(new PrimaryGeneratorAction);
  run->SetUserAction(new SteppingAction);
  run->SetUserAction(new EventAction);
  run->Initialize();
  for (g_cur = 0; g_cur < kN; ++g_cur) run->BeamOn(n);
  std::printf("# Ge %gx%gx%g cm, %g MeV 伽马准直正入射, 源距 %g cm, 每点 %ld 事件\n",
              2 * kR / cm, 2 * kR / cm, kL / cm, kE / MeV, -kSrcZ / cm, n);
  std::printf("# %-10s %-14s %-16s %-12s\n", "死层(mm)", "探测效率(%)", "全能峰效率(%)", "峰/全谱比");
  for (G4int i = 0; i < kN; ++i) {
    G4double a = 100. * g_any[i] / n, f = 100. * g_full[i] / n;
    std::printf("  %-10.2f %-14.2f %-16.2f %-12.3f\n", kDead[i] / mm, a, f,
                g_any[i] ? (G4double)g_full[i] / g_any[i] : 0.);
  }
  delete run;
  return 0;
}
