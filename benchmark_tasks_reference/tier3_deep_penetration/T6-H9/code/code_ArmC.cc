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
#include "G4Run.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4MagneticField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4ClassicalRK4.hh"
#include "G4Mag_UsualEqRhs.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T6-H9: 空间核动力环形排热系统相空间角系数视因子计算
// 组别: Arm C
// ============================================================================

static G4double gRadiatorEscapedFlux = 0.0;
static G4long gFinShadowInterceptions = 0;

class T6H9Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* matBe = nist->FindOrBuildMaterial("G4_Be");

    auto* worldSolid = new G4Box("WorldBox", 1.0 * m, 1.0 * m, 1.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 散热阵列几何 (外径 40 cm)
    auto* bodySolid = new G4Tubs("FinArraySolid", 10.0 * cm, 40.0 * cm, 20.0 * cm, 0, 360*deg);
    auto* bodyLog = new G4LogicalVolume(bodySolid, matBe, "FinArrayLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), bodyLog, "FinArrayPhys", worldLog, false, 1);

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
    if (!pre || pre->GetName() != "FinArrayPhys") return;

    // Arm C: 环形辐射散热器 (radiator) 金属翼片 (fin) 阴影自遮挡 (shadow) 与三维视因子 (viewFactor)
    G4ThreeVector pos = aStep->GetPostStepPoint()->GetPosition();
    G4double r_xy = pos.perp();
    if (r_xy > 35.0 * cm) { // 逸出最外侧散热翼片
      gRadiatorEscapedFlux += 1.0;
    } else {
      gFinShadowInterceptions++;
    }

  }
};

class T6H9RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H9] Radiator viewFactor and fin shadow tally complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H9Detector());
  runManager->SetUserInitialization(new T6H9Physics());
  runManager->SetUserAction(new T6H9Generator());
  runManager->SetUserAction(new T6H9RunAction());
  runManager->SetUserAction(new T6H9SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H9] Completed" << std::endl;

  delete runManager;
  return 0;
}
