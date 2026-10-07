// 活化产物伽马能谱：活化样品发出的伽马在 HPGe 探测器中的沉积能谱
// 针对 PHYS-0022 修正：Co-60 级联伽马 (1.173 MeV, 1.332 MeV) 分支比由 0.60 修正为 1.00
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o T3-4
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Orb.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmLivermorePhysics.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4Step.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include <cmath>

// 活化产物主要伽马发射线 (MeV) 与相对强度 —— Na-24 / Mn-56 / Co-60 等
static const G4int NL = 6, NB = 60;  // NB: 0-3 MeV, 50 keV/道
static const G4double gE[NL] = {0.847, 1.173, 1.332, 1.369, 1.811, 2.754};
// PHYS-0022 修正：Co-60 分支比修正为 1.00（1.173 与 1.332 MeV 均为 ~100% 级联发射）
static const G4double gI[NL] = {1.00, 1.00, 1.00, 1.00, 0.27, 0.50};
static G4LogicalVolume* gDet = nullptr;   // 探测器逻辑体
static G4double gEdep = 0.;               // 单事件沉积能
static G4int gSpec[NB] = {0}, gEvt = 0, gHit = 0;

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("World", 25 * cm, 25 * cm, 25 * cm),
                                      nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* wp = new G4PVPlacement(nullptr, {}, world, "World", nullptr, false, 0);
    // 均匀含活化产物的样品（混凝土小球，伽马从样品内各点各向同性发出）
    new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Orb("Sample", 2 * cm),
        nist->FindOrBuildMaterial("G4_CONCRETE"), "Sample"), "Sample", world, false, 0);
    // HPGe 探测器，距样品 5 cm
    gDet = new G4LogicalVolume(new G4Tubs("Ge", 0, 3 * cm, 3 * cm, 0, 360 * deg),
                               nist->FindOrBuildMaterial("G4_Ge"), "Ge");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 5 * cm), gDet, "Ge", world, false, 0);
    return wp;
  }
};

class Physics : public G4VModularPhysicsList {
public:
  Physics() { RegisterPhysics(new G4EmLivermorePhysics()); }
};

class Generator : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* gun;
  G4double tot = 0.;
public:
  Generator() : gun(new G4ParticleGun(1)) {
    gun->SetParticleDefinition(G4Gamma::Gamma());
    for (int i = 0; i < NL; ++i) tot += gI[i];
  }
  ~Generator() override { delete gun; }
  void GeneratePrimaries(G4Event* ev) override {
    G4double rnd = G4UniformRand() * tot, e = gE[NL - 1];
    for (int i = 0; i < NL; ++i) {
      if (rnd < gI[i]) { e = gE[i]; break; }
      rnd -= gI[i];
    }
    G4double u = 2 * G4UniformRand() - 1, phi = CLHEP::twopi * G4UniformRand();
    G4double sn = std::sqrt(std::max(0.0, 1.0 - u * u));
    G4ThreeVector dir(sn * std::cos(phi), sn * std::sin(phi), u);    // 各向同性方向
    G4ThreeVector pos = 2 * cm * std::cbrt(G4UniformRand()) * dir;   // 样品内均匀发射点
    gun->SetParticleEnergy(e * MeV);
    gun->SetParticlePosition(pos);
    gun->SetParticleMomentumDirection(dir);
    gun->GeneratePrimaryVertex(ev);
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    auto* vol = st->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    if (vol && vol->GetLogicalVolume() == gDet)
      gEdep += st->GetTotalEnergyDeposit();
  }
};

class Event : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    ++gEvt;
    if (gEdep <= 0.) return;
    ++gHit;
    int b = static_cast<int>(gEdep / MeV / 3.0 * NB);
    if (b >= 0 && b < NB) ++gSpec[b];
  }
};

int main() {
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new Physics);
  rm->SetUserAction(new Generator);
  rm->SetUserAction(new Stepping);
  rm->SetUserAction(new Event);
  rm->Initialize();
  const G4int N = 1000;
  rm->BeamOn(N);
  G4cout << "\n=== 活化产物伽马发射线 (MeV, 相对强度) ===" << G4endl;
  for (int i = 0; i < NL; ++i)
    G4cout << "  " << gE[i] << " MeV  I=" << gI[i] << G4endl;
  G4cout << "\n=== HPGe 沉积能谱 (50 keV/道, 事件 " << N << ", 有沉积 " << gHit << ") ===" << G4endl;
  for (int b = 0; b < NB; ++b) {
    if (gSpec[b])
      G4cout << "  E=" << (b + 0.5) * 3.0 / NB << " MeV : " << gSpec[b] << G4endl;
  }
  G4cout << "无沉积事件: " << gEvt - gHit << G4endl;
  delete rm;
  return 0;
}
