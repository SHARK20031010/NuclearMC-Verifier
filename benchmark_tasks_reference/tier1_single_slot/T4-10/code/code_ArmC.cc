// ============================================================================
// 蒙特卡洛任务 T4-10: 高能电子穿过水体产生的契伦科夫辐射光子数与发射角锥模拟
// 针对 PHYS-0054 缺陷修正: 为水介质添加 G4MaterialPropertiesTable 并注册折射率 RINDEX
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
// 运行: ./code [事件数] (默认 100)
// ============================================================================
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4OpticalPhysics.hh"
#include "G4EmParameters.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Run.hh"
#include "G4OpticalPhoton.hh"
#include "G4MaterialPropertiesTable.hh"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>

// 全局统计量：契伦科夫光子数与发射角分布
static long gCherenkovPhotons = 0;
static double gSumAngleDeg = 0.0;
static double gSumCosTheta = 0.0;

class DetConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    // [PHYS-0054 修复]: 为水添加光学属性表，注册折射率 RINDEX
    // 在可见至近紫外波段 (1.5 eV - 6.0 eV), 水的折射率约为 1.333
    auto* mpt = new G4MaterialPropertiesTable();
    std::vector<G4double> photonE = {1.5 * eV, 6.0 * eV};
    std::vector<G4double> rIndex  = {1.333, 1.333};
    mpt->AddProperty("RINDEX", photonE, rIndex);
    water->SetMaterialPropertiesTable(mpt);

    auto* solidWorld = new G4Box("World", 50.0 * cm, 50.0 * cm, 50.0 * cm);
    auto* logicWorld = new G4LogicalVolume(solidWorld, water, "World");
    return new G4PVPlacement(nullptr, {}, logicWorld, "World", nullptr, false, 0);
  }
};

class PhysicsList : public G4VModularPhysicsList {
public:
  PhysicsList() {
    SetVerboseLevel(0);
    G4EmParameters::Instance()->SetVerbose(0);
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4OpticalPhysics());
  }
};

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGenerator() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    fGun->SetParticleEnergy(10.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0.0, 0.0, -20.0 * cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
  }
  ~PrimaryGenerator() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetTrack()->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
      step->GetTrack()->SetTrackStatus(fStopAndKill); // 统计后及时终止光子，节省时间
      return;
    }
    const auto* secondaries = step->GetSecondaryInCurrentStep();
    if (!secondaries) return;

    G4ThreeVector eDir = step->GetPreStepPoint()->GetMomentumDirection();
    for (const auto* sec : *secondaries) {
      if (sec->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
        const auto* proc = sec->GetCreatorProcess();
        if (proc && proc->GetProcessName() == "Cerenkov") {
          G4ThreeVector photDir = sec->GetMomentumDirection();
          double cosTheta = std::clamp(eDir.dot(photDir), -1.0, 1.0);
          double thetaDeg = std::acos(cosTheta) * (180.0 / CLHEP::pi);
          gCherenkovPhotons++;
          gSumCosTheta += cosTheta;
          gSumAngleDeg += thetaDeg;
        }
      }
    }
  }
};

class RunAction : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* run) override {
    long nEvents = run->GetNumberOfEvent();
    double avgPhotons = (nEvents > 0) ? (double)gCherenkovPhotons / nEvents : 0.0;
    double avgAngle = (gCherenkovPhotons > 0) ? gSumAngleDeg / gCherenkovPhotons : 0.0;
    double avgCos   = (gCherenkovPhotons > 0) ? gSumCosTheta / gCherenkovPhotons : 0.0;
    double theoDeg  = std::acos(1.0 / 1.333) * (180.0 / CLHEP::pi);

    std::cout << "\n================= T4-10 契伦科夫辐射模拟结果 =================" << std::endl;
    std::cout << "初级电子能量: 10 MeV, 模拟事件数: " << nEvents << std::endl;
    std::cout << "契伦科夫光子总产生数: " << gCherenkovPhotons << std::endl;
    std::cout << "平均单电子产生光子数: " << std::fixed << std::setprecision(1) << avgPhotons << std::endl;
    std::cout << "出射发射角锥 (契伦科夫角):" << std::endl;
    std::cout << "  - 模拟平均 cos(theta): " << std::setprecision(4) << avgCos << std::endl;
    std::cout << "  - 模拟平均开角 theta : " << std::setprecision(2) << avgAngle << " deg" << std::endl;
    std::cout << "  - 理论极大值 (beta->1) : " << theoDeg << " deg (cos=" << 1.0 / 1.333 << ")" << std::endl;
    std::cout << "============================================================\n" << std::endl;
  }
};

int main(int argc, char** argv) {
  int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100;
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new DetConstruction());
  rm->SetUserInitialization(new PhysicsList());
  rm->SetUserAction(new PrimaryGenerator());
  rm->SetUserAction(new SteppingAction());
  rm->SetUserAction(new RunAction());
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
