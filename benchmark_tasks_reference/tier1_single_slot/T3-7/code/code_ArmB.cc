// T3-7: 50 MeV 电子轰击重金属钨靶诱发光核反应中子产额模拟
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "FTFP_BERT.hh"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

// 全局统计量：总中子数、光核中子数、电核中子数及动能总和
static G4long gTotalNeutrons = 0;
static G4long gPhotoNeutrons = 0;
static G4long gElectroNeutrons = 0;
static G4double gTotalNeutronEk = 0.0;

// 几何构建：高真空世界中放置厚钨靶 (4cm x 4cm x 2cm, 约 5.7 辐射长度)
class TargetDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldMat = nist->FindOrBuildMaterial("G4_Galactic");
    auto* targetMat = nist->FindOrBuildMaterial("G4_W");

    auto* worldSolid = new G4Box("World", 30.0 * cm, 30.0 * cm, 30.0 * cm);
    auto* worldLV = new G4LogicalVolume(worldSolid, worldMat, "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);

    // 钨靶尺寸：半宽 2cm, 半厚 1cm (即厚度 2cm)
    auto* targetSolid = new G4Box("Target_W", 2.0 * cm, 2.0 * cm, 1.0 * cm);
    auto* targetLV = new G4LogicalVolume(targetSolid, targetMat, "Target_W");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), targetLV, "Target_W", worldLV, false, 0);

    return worldPV;
  }
};

// 物理列表：继承 FTFP_BERT 以激活 G4EmExtraPhysics 中的光核 (PhotoNuclear) 与电核反应
class PhotoNuclearPhysicsList : public FTFP_BERT {
public:
  PhotoNuclearPhysicsList() : FTFP_BERT(0) {}
};

// 初级粒子源：50 MeV 电子束沿 +Z 轴垂直入射靶前表面 (z = -5 cm -> z = -1 cm)
class ElectronPrimaryGenerator : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun{1};
public:
  ElectronPrimaryGenerator() {
    auto* particle = G4ParticleTable::GetParticleTable()->FindParticle("e-");
    fGun.SetParticleDefinition(particle);
    fGun.SetParticleEnergy(50.0 * MeV);
    fGun.SetParticlePosition(G4ThreeVector(0.0, 0.0, -5.0 * cm));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
  }
  void GeneratePrimaries(G4Event* event) override {
    fGun.GeneratePrimaryVertex(event);
  }
};

// 计分步进动作：在次级中子产生的首步统计反应通道与初始动能
class NeutronSteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4Track* track = step->GetTrack();
    if (track->GetParticleDefinition()->GetParticleName() == "neutron" &&
        track->GetCurrentStepNumber() == 1) {
      gTotalNeutrons++;
      gTotalNeutronEk += track->GetKineticEnergy();

      const G4VProcess* creator = track->GetCreatorProcess();
      if (creator) {
        const G4String& procName = creator->GetProcessName();
        if (procName == "photonNuclear") {
          gPhotoNeutrons++;
        } else if (procName == "electronNuclear") {
          gElectroNeutrons++;
        }
      }
    }
  }
};

int main(int argc, char** argv) {
  const G4long nEvents = (argc > 1) ? std::atol(argv[1]) : 200;

  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);
  runManager->SetUserInitialization(new TargetDetector());
  runManager->SetUserInitialization(new PhotoNuclearPhysicsList());
  runManager->SetUserAction(new ElectronPrimaryGenerator());
  runManager->SetUserAction(new NeutronSteppingAction());

  runManager->Initialize();
  runManager->BeamOn(nEvents);

  // ---------- 结果统计与物理分析 ----------
  const G4double yield = static_cast<G4double>(gTotalNeutrons) / nEvents;
  const G4double yieldErr = std::sqrt(std::max(1L, gTotalNeutrons)) / nEvents;
  const G4double avgEk = (gTotalNeutrons > 0) ? (gTotalNeutronEk / gTotalNeutrons / MeV) : 0.0;

  std::cout << "\n================ T3-7 50 MeV 电子光核中子产额模拟 ================\n";
  std::cout << "入射初级电子数 : " << nEvents << " (50 MeV e-)\n";
  std::cout << "产生总中子数   : " << gTotalNeutrons << "\n";
  std::cout << "  - 光核反应中子 (photonNuclear)   : " << gPhotoNeutrons << "\n";
  std::cout << "  - 电核反应中子 (electronNuclear) : " << gElectroNeutrons << "\n";
  std::cout << "每入射电子中子产额 Y_n : " << std::fixed << std::setprecision(4)
            << yield << " +/- " << yieldErr << " n/e-\n";
  std::cout << "次级中子平均动能       : " << std::setprecision(2) << avgEk << " MeV\n";
  std::cout << "-------------------------------------------------------------------\n";
  std::cout << "T3-7 Photoneutrons = " << gTotalNeutrons << std::endl;

  delete runManager;
  return 0;
}
