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
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T4-H5: 中子伽马双探测有机闪烁体脉冲形状甄别
// 组别: Arm C
// ============================================================================

static G4double gFastLight = 0.0;
static G4double gTailLight = 0.0;
static G4long gPSD_Counts = 0;

class T4H5Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matScint = nist->BuildMaterialWithNewDensity("PlasticScint_Mat", "G4_POLYSTYRENE", 1.03 * g/cm3);
    matScint->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);

    auto* worldSolid = new G4Box("WorldBox", 30.0 * cm, 30.0 * cm, 30.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    auto* ejSolid = new G4Tubs("Scint_Solid", 0.0 * cm, 2.5 * cm, 2.5 * cm, 0, 360*deg);
    auto* ejLog = new G4LogicalVolume(ejSolid, matScint, "Scint_Log");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), ejLog, "Scint_Phys", worldLog, false, 1);

    return worldPhys;
  }
};

class T4H5Physics : public G4VModularPhysicsList {
public:
  T4H5Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T4H5Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    if (G4UniformRand() < 0.5) {
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
      gun.SetParticleEnergy(1.5 * MeV);
    } else {
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
      gun.SetParticleEnergy(500.0 * keV);
    }
    gun.SetParticlePosition(G4ThreeVector(0, 0, -5.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T4H5SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "Scint_Phys") return;

    // Arm C: 实现 Birks 效应与快/慢成分发光时域积分比 PSD 甄别
    G4double edep = aStep->GetTotalEnergyDeposit();
    G4double stepLength = aStep->GetStepLength();
    if (edep > 1.0 * keV) {
      G4double kB = 0.126 * mm / MeV;
      G4double dE_dx = edep / (stepLength + 1e-6 * mm);
      G4double quenched_edep = edep / (1.0 + kB * dE_dx);

      G4String pName = aStep->GetTrack()->GetDefinition()->GetParticleName();
      G4double fast_frac = (pName == "proton") ? 0.65 : 0.90;
      G4double slow_frac = 1.0 - fast_frac;

      G4double q_fast = quenched_edep * fast_frac;
      G4double q_tail = quenched_edep * slow_frac;
      G4double psd_ratio = q_tail / (q_fast + q_tail);

      gFastLight += q_fast;
      gTailLight += q_tail;
      gPSD_Counts++;
    }

  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T4H5Detector());
  runManager->SetUserInitialization(new T4H5Physics());
  runManager->SetUserAction(new T4H5Generator());
  runManager->SetUserAction(new T4H5SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T4-H5] Done" << std::endl;

  delete runManager;
  return 0;
}
