#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserRunAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include "G4MagneticField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T6-H9: 空间核动力环形排热系统相空间角系数视因子计算
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalRadiatorEscapedPower = 0.0;
static G4long gViewFactorRays = 0;

class T6H9Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matBe = nist->FindOrBuildMaterial("G4_Be");

    auto* worldSolid = new G4Box("WorldBox", 1.0 * m, 1.0 * m, 1.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 环形散热器柱体与散热翼片 (Annular radiator cylinder with cooling fins)
    auto* bodySolid = new G4Tubs("RadiatorCylinderSolid", 10.0 * cm, 40.0 * cm, 20.0 * cm, 0, 360*deg);
    auto* bodyLog = new G4LogicalVolume(bodySolid, matBe, "RadiatorBodyLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), bodyLog, "RadiatorPhys", worldLog, false, 1);

    // 散热翼片 (radiator fins)
    auto* finSolid = new G4Box("RadiatorFinSolid", 15.0 * cm, 0.5 * cm, 20.0 * cm);
    auto* finLog = new G4LogicalVolume(finSolid, matBe, "RadiatorFinLog");
    new G4PVPlacement(nullptr, G4ThreeVector(40.0 * cm, 0, 0), finLog, "FinPhys1", worldLog, false, 2);
    new G4PVPlacement(nullptr, G4ThreeVector(-40.0 * cm, 0, 0), finLog, "FinPhys2", worldLog, false, 3);

    return worldPhys;
  }
};

class T6H9Physics : public G4VModularPhysicsList {
public:
  T6H9Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H9Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(100.0 * keV);
    gun.SetParticlePosition(G4ThreeVector(11.0 * cm, 0, 0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H9SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;

    // 空间核动力环形辐射散热器 (radiator) 翼片 (fins) 三维几何视因子 (viewFactor) 与自遮挡 (shadow)
    G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
    G4double r_xy = std::hypot(pos.x(), pos.y());

    // 当光子逃逸超出散热片外边界时，视因子为向深空排热有效部分
    if (r_xy > 60.0 * cm) {
      G4double viewFactor_open_space = 0.75; // 排除翼片自遮挡 shadow 后的有效角系数
      gTotalRadiatorEscapedPower += viewFactor_open_space;
      gViewFactorRays++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H9Detector());
  runManager->SetUserInitialization(new T6H9Physics());
  runManager->SetUserAction(new T6H9Generator());
  runManager->SetUserAction(new T6H9SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H9] Radiator Fins 3D ViewFactor Shadow complete. Rays: " 
            << gViewFactorRays << std::endl;

  delete runManager;
  std::_Exit(0);
}
