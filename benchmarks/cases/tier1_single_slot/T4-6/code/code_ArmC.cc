// T4-6: 散裂源快中子飞行时间谱 (TOF) 测量 (老师傅插件物理修正版)
// 依据 PHYS-0050 修正：记录中子到达探测器的绝对飞行时间必须使用全局时间 GetGlobalTime()

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "QGSP_BIC_HP.hh"
#include <iostream>
#include <vector>
#include <numeric>
#include <iomanip>

static const G4double kFlightDist = 5.0 * m;
static std::vector<G4double> gTOFList;
static bool gHitInEvent = false;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* airMat = nist->FindOrBuildMaterial("G4_AIR");
    auto* scMat  = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");

    auto* worldBox = new G4Box("WorldBox", 2.0 * m, 2.0 * m, 6.0 * m);
    auto* worldLV  = new G4LogicalVolume(worldBox, airMat, "WorldLV");
    auto* worldPV  = new G4PVPlacement(nullptr, {}, worldLV, "WorldPV", nullptr, false, 0);

    // 探测器放置于中子飞行路径 5 米处 (z = 5.0 m)
    auto* detBox = new G4Box("DetBox", 10.0 * cm, 10.0 * cm, 5.0 * cm);
    auto* detLV  = new G4LogicalVolume(detBox, scMat, "DetLV");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kFlightDist), detLV, "DetPV", worldLV, false, 0);

    return worldPV;
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    auto* particle = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    fGun->SetParticleDefinition(particle);
    fGun->SetParticleEnergy(14.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~PrimaryGeneratorAction() override { delete fGun; }

  void GeneratePrimaries(G4Event* ev) override {
    fGun->GeneratePrimaryVertex(ev);
  }
};

class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gHitInEvent = false;
  }
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (gHitInEvent) return;
    auto* pv = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv || pv->GetName() != "DetPV") return;

    // PHYS-0050 修正核心：调用 GetGlobalTime() 获取从源发射到探测器的绝对飞行时间 (TOF)
    G4double tof = step->GetPreStepPoint()->GetGlobalTime();
    gTOFList.push_back(tof);
    gHitInEvent = true;
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100;

  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new DetectorConstruction());
  runManager->SetUserInitialization(new QGSP_BIC_HP());
  runManager->SetUserAction(new PrimaryGeneratorAction());
  runManager->SetUserAction(new EventAction());
  runManager->SetUserAction(new SteppingAction());

  runManager->Initialize();
  runManager->BeamOn(nEvents);

  std::cout << "\n================ T4-6 快中子飞行时间谱 (TOF) 测量结果 ================" << std::endl;
  std::cout << "发射粒子数: " << nEvents << " | 探测器击中数: " << gTOFList.size() << std::endl;

  if (!gTOFList.empty()) {
    G4double sum = std::accumulate(gTOFList.begin(), gTOFList.end(), 0.0);
    G4double avgTOF = sum / gTOFList.size();
    G4double beta = (kFlightDist / avgTOF) / c_light;
    G4double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
    G4double m_n = 939.565 * MeV;
    G4double E_kin = (gamma - 1.0) * m_n;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "飞行距离: " << kFlightDist / m << " m" << std::endl;
    std::cout << "平均飞行时间 TOF: " << avgTOF / ns << " ns" << std::endl;
    std::cout << "测量速度 beta (v/c): " << beta << std::endl;
    std::cout << "由 TOF 重建中子动能: " << E_kin / MeV << " MeV (标称 14.0 MeV)" << std::endl;
  }
  std::cout << "======================================================================\n" << std::endl;

  delete runManager;
  return 0;
}
