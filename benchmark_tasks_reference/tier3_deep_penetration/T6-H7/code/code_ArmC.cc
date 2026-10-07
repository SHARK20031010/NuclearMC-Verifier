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
// 任务 T6-H7: 医用旋转机架圆周运动源中心原点空间动量矢量对准
// 组别: Arm C
// ============================================================================

static G4double gPhantomEdep = 0.0;
static G4long gHitEvents = 0;

class T6H7Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 1.5 * m, 1.5 * m, 1.5 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 中心水幻体 (等中心原点 0,0,0)
    auto* boxSolid = new G4Box("PhantomSolid", 10.0 * cm, 10.0 * cm, 10.0 * cm);
    auto* boxLog = new G4LogicalVolume(boxSolid, water, "PhantomLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), boxLog, "PhantomPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T6H7Physics : public G4VModularPhysicsList {
public:
  T6H7Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H7Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.0 * MeV);

    // 源在半径 R=100 cm 的圆周上旋转抽样
    G4double phi = G4UniformRand() * 360.0 * deg;
    G4double R = 100.0 * cm;
    G4ThreeVector pos(R * std::cos(phi), R * std::sin(phi), 0.0);
    gun.SetParticlePosition(pos);

    // Arm C: 严格执行旋转源动量方向始终对准几何中心靶区原点: dir = (-pos).unit()
    G4ThreeVector dir = (-pos).unit();
    gun.SetParticleMomentumDirection(dir);

    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H7SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "PhantomPhys") return;
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.0) {
      gPhantomEdep += edep;
      gHitEvents++;
    }
  }
};

class T6H7RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H7] Gantry radiation simulation complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H7Detector());
  runManager->SetUserInitialization(new T6H7Physics());
  runManager->SetUserAction(new T6H7Generator());
  runManager->SetUserAction(new T6H7RunAction());
  runManager->SetUserAction(new T6H7SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H7] Completed" << std::endl;

  delete runManager;
  return 0;
}
