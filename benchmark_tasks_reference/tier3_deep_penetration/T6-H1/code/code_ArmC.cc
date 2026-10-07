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
// 任务 T6-H1: 高能强子厚散裂靶产生顶点生命周期追踪
// 组别: Arm C
// ============================================================================

static G4long gSpallationBirthVertices = 0;
static G4double gTotalBirthEnergy = 0.0;

class T6H1Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* matW = nist->FindOrBuildMaterial("G4_W");

    auto* worldSolid = new G4Box("WorldBox", 1.0 * m, 1.0 * m, 1.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, air, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 50 cm 钨靶
    auto* tgtSolid = new G4Tubs("SpallTgtSolid", 0.0 * cm, 5.0 * cm, 25.0 * cm, 0, 360*deg);
    auto* tgtLog = new G4LogicalVolume(tgtSolid, matW, "SpallTgtLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tgtLog, "SpallTgtPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T6H1Physics : public G4VModularPhysicsList {
public:
  T6H1Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T6H1Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1.2 * GeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -30.0 * cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T6H1SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "SpallTgtPhys") return;

    // Arm C: 严格过滤产生顶点生命周期首步 track->GetCurrentStepNumber() == 1，消除后续弹性/非弹性散射步点重复累加
    G4Track* track = aStep->GetTrack();
    if (track->GetCurrentStepNumber() == 1 && track->GetTrackID() > 1) {
      G4String pName = track->GetDefinition()->GetParticleName();
      if (pName == "neutron" || pName == "proton") {
        gSpallationBirthVertices++;
        gTotalBirthEnergy += track->GetKineticEnergy();
      }
    }

  }
};

class T6H1RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T6-H1] Spallation tally finished." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T6H1Detector());
  runManager->SetUserInitialization(new T6H1Physics());
  runManager->SetUserAction(new T6H1Generator());
  runManager->SetUserAction(new T6H1RunAction());
  runManager->SetUserAction(new T6H1SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T6-H1] Completed" << std::endl;

  delete runManager;
  return 0;
}
