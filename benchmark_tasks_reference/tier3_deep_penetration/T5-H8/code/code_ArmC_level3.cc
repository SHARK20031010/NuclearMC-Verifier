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
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T5-H8: 质子治疗靶区微剂量比能与生物剂量增益映射
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gTotalPhysicalDose = 0.0;
static G4double gTotalBiologicalDose = 0.0;
static G4double gAvgRBE = 1.0;
static G4long gStepTallies = 0;

class T5H8Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 20.0 * cm, 20.0 * cm, 40.0 * cm);
    auto* worldLog = new G4LogicalVolume(worldSolid, water, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    return worldPhys;
  }
};

class T5H8Physics : public G4VModularPhysicsList {
public:
  T5H8Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H8Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(150.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -10.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H8SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre) return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    G4double stepLen = aStep->GetStepLength();
    if (edep > 10.0 * keV && stepLen > 0.0) {
      gTotalPhysicalDose += edep;

      // 质子治疗深度微剂量线能谱与剂量平均线能 y_D
      G4double lineal_energy_y = edep / (1.0 * um); // keV / um
      G4double y_D = lineal_energy_y * 1.2; // 谱线展开剂量平均线能

      // 临床相对生物效应 (RBE) 空间映射模型: Wilkens / McNamara 临床模型
      // RBE = 1.0 + c * y_D
      G4double c_bio = 0.04;
      G4double local_RBE = 1.0 + c_bio * (y_D / (keV/um));
      if (local_RBE > 2.5) local_RBE = 2.5;

      G4double biologicalDose = edep * local_RBE; // 生物有效剂量
      gTotalBiologicalDose += biologicalDose;
      gStepTallies++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H8Detector());
  runManager->SetUserInitialization(new T5H8Physics());
  runManager->SetUserAction(new T5H8Generator());
  runManager->SetUserAction(new T5H8SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H8] Proton Therapy RBE Biological Dose: " 
            << gTotalBiologicalDose / MeV << " MeV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
