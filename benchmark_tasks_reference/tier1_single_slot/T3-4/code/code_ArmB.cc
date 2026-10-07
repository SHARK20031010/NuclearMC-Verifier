// 活化产物伽马能谱：活化混凝土样品发射伽马在 HPGe 探测器中的沉积能谱
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
#include "G4RandomDirection.hh"
#include <cmath>
#include <iomanip>

// 典型活化产物特征伽马发射线 (MeV) 与相对强度 (Na-24, Co-60, Mn-56 等)
// 修正：Co-60 (1.173, 1.332 MeV) 与 Na-24 (1.369, 2.754 MeV) 级联跃迁分支比均为 ~1.00
static const G4int NL = 6, NB = 60;
static const G4double gE[NL] = {0.847, 1.173, 1.332, 1.369, 1.811, 2.754};
static const G4double gI[NL] = {1.00,  1.00,  1.00,  1.00,  0.27,  1.00};
static G4LogicalVolume* gDetLV = nullptr;
static G4double gEdep = 0.;
static G4int gSpec[NB] = {0}, gHit = 0;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    // 世界体：空气盒子 (50 cm x 50 cm x 50 cm)
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 25*cm, 25*cm, 25*cm),
                                        nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    // 活化样品：半径 2 cm 混凝土小球，表面位于 z = 2 cm
    auto* sampleLV = new G4LogicalVolume(new G4Orb("Sample", 2*cm),
                                         nist->FindOrBuildMaterial("G4_CONCRETE"), "Sample");
    new G4PVPlacement(nullptr, {}, sampleLV, "Sample", worldLV, false, 0);
    // HPGe 探测器：半径 3 cm, 半高 3 cm；前表面位于 z = 7 cm (距样品表面 5 cm)，中心在 z = 10 cm
    gDetLV = new G4LogicalVolume(new G4Tubs("HPGe", 0, 3*cm, 3*cm, 0, 360*deg),
                                 nist->FindOrBuildMaterial("G4_Ge"), "HPGe");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 10*cm), gDetLV, "HPGe", worldLV, false, 0);
    return worldPV;
  }
};

class PhysicsList : public G4VModularPhysicsList {
public:
  PhysicsList() { RegisterPhysics(new G4EmLivermorePhysics(0)); }
};

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* gun;
  G4double totI = 0.;
public:
  PrimaryGenerator() : gun(new G4ParticleGun(1)) {
    gun->SetParticleDefinition(G4Gamma::Gamma());
    for (int i = 0; i < NL; ++i) totI += gI[i];
  }
  ~PrimaryGenerator() override { delete gun; }
  void GeneratePrimaries(G4Event* event) override {
    // 离散特征能谱轮盘赌 CDF 抽样
    G4double rnd = G4UniformRand() * totI, eGamma = gE[NL - 1];
    for (int i = 0; i < NL; ++i) {
      if (rnd < gI[i]) { eGamma = gE[i]; break; }
      rnd -= gI[i];
    }
    // 样品内空间均匀发射点 (r = R * u^(1/3)) 与各向同性独立发射动量方向
    G4ThreeVector pos = (2.0 * cm * std::cbrt(G4UniformRand())) * G4RandomDirection();
    G4ThreeVector dir = G4RandomDirection();
    gun->SetParticleEnergy(eGamma * MeV); gun->SetParticlePosition(pos);
    gun->SetParticleMomentumDirection(dir); gun->GeneratePrimaryVertex(event);
  }
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* vol = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    if (vol && vol->GetLogicalVolume() == gDetLV)
      gEdep += step->GetTotalEnergyDeposit();
  }
};

class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (gEdep <= 0.) return;
    ++gHit;
    int bin = int((gEdep / MeV) / 3.0 * NB);
    if (bin >= 0 && bin < NB) ++gSpec[bin];
  }
};

int main() {
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new DetectorConstruction);
  rm->SetUserInitialization(new PhysicsList);
  rm->SetUserAction(new PrimaryGenerator);
  rm->SetUserAction(new SteppingAction);
  rm->SetUserAction(new EventAction);
  rm->Initialize();

  const G4int N = 50000;
  rm->BeamOn(N);

  const G4double dE = 3.0 / NB; // 50 keV / bin
  G4cout << "\n================ 活化产物伽马能谱模拟结果 ================" << G4endl;
  G4cout << "发射粒子数: " << N << " | 响应事件: " << gHit
         << " | 探测效率: " << std::fixed << std::setprecision(4)
         << (double)gHit / N * 100 << "% (+/- " << std::sqrt(gHit) / N * 100 << "%)\n" << G4endl;
  G4cout << "道号\t能量(MeV)\t计数\t统计误差\t归一化能谱(1/MeV)" << G4endl;
  for (int b = 0; b < NB; ++b) {
    if (gSpec[b] > 0) {
      G4double eCenter = (b + 0.5) * dE, err = std::sqrt(gSpec[b]), diffYield = (double)gSpec[b] / (N * dE);
      G4cout << b << "\t" << std::setprecision(3) << eCenter << "\t\t"
             << gSpec[b] << "\t+/-" << std::setprecision(1) << err
             << "\t\t" << std::setprecision(4) << diffYield << G4endl;
    }
  }
  G4cout << "==========================================================" << G4endl;
  delete rm;
  return 0;
}
