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
// 任务 T6-H3: 行星际空间太阳宇宙线质子地球磁鞘偏转与刚度截断模拟
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gDeflectedAngle = 0.0;
static G4long gCutoffAllowedCount = 0;

class T6H3EarthDipoleField : public G4MagneticField {
public:
  void GetFieldValue(const G4double Point[4], G4double* Bfield) const override {
    // 地球宏观偶极磁场 (Earth dipole B_field)
    G4double x = Point[0];
    G4double y = Point[1];
    G4double z = Point[2];
    G4double r = std::sqrt(x*x + y*y + z*z);
    if (r < 1.0 * m) r = 1.0 * m; // 避免奇点

    G4double M_dipole = 8.0e15 * tesla * m3; // 等效偶极磁矩
    G4double B0 = M_dipole / (r * r * r);
    // 偶极场矢量: B = (3(m.r)r - m)/r^3
    Bfield[0] = 3.0 * B0 * x * z / (r * r);
    Bfield[1] = 3.0 * B0 * y * z / (r * r);
    Bfield[2] = B0 * (3.0 * z * z / (r * r) - 1.0);
  }
};

class T6H3Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * m, 20.0 * m, 20.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");

    // 设置全局偶极磁场
    auto* dipoleMag = new T6H3EarthDipoleField();
    auto* fldMgr = new G4FieldManager(dipoleMag);
    fldMgr->CreateChordFinder(dipoleMag);
    worldLog->SetFieldManager(fldMgr, true);

    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);
    return worldPhys;
  }
};

class T6H3Physics : public G4VModularPhysicsList {
public:
  T6H3Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H3Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(2.0 * GeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -5.0 * m));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H3SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* post = aStep->GetPostStepPoint();
    if (!post) return;

    // 基于 Störmer 理论计算地球偶极地磁刚度截断 (rigidity cutoff)
    G4ThreeVector p_dir = post->GetMomentumDirection();
    gDeflectedAngle += p_dir.x();

    G4double magnetic_rigidity = aStep->GetTrack()->GetMomentum().mag() / (eplus * CLHEP::c_light); // 磁刚度
    G4double cutoff_rigidity = 1000.0 * megavolt; // Störmer 刚度截断 (rigidity cutoff) 阈值

    if (magnetic_rigidity > cutoff_rigidity) {
      gCutoffAllowedCount++; // 穿透地磁截断到达低轨
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H3Detector());
  runManager->SetUserInitialization(new T6H3Physics());
  runManager->SetUserAction(new T6H3Generator());
  runManager->SetUserAction(new T6H3SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H3] Earth Dipole Rigidity Cutoff Allowed: " << gCutoffAllowedCount << std::endl;

  delete runManager;
  std::_Exit(0);
}
