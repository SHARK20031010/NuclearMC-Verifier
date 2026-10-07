// T6-1 (Arm B): 20 MeV e- on W -> bremsstrahlung spectrum (自查修正版)
// 自查复核：初生韧致辐射光子在产生步进 (Step 1) 中可能已发生相互作用损失能量，
// 必须在 PreStepPoint 读取初生动能 (step->GetPreStepPoint()->GetKineticEnergy() 或 GetVertexKineticEnergy())。
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4PhysListFactory.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Gamma.hh"
#include "G4Track.hh"
#include "G4Step.hh"
#include "G4Run.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4ios.hh"
#include <cstdlib>

static const G4int NBIN = 100;
static const G4double EMAX = 20.0;
static G4double gHist[NBIN] = {0};

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4NistManager* nist = G4NistManager::Instance();
    G4LogicalVolume* worldL = new G4LogicalVolume(
        new G4Box("World", 10*cm, 10*cm, 10*cm), nist->FindOrBuildMaterial("G4_AIR"), "World");
    G4VPhysicalVolume* worldP = new G4PVPlacement(0, G4ThreeVector(), worldL, "World", 0, false, 0);
    G4LogicalVolume* tgtL = new G4LogicalVolume(
        new G4Box("Target", 1*cm, 1*cm, 0.1*cm), nist->FindOrBuildMaterial("G4_W"), "Target");
    new G4PVPlacement(0, G4ThreeVector(0, 0, 0.1*cm), tgtL, "Target", worldL, false, 0);
    return worldP;
  }
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    G4Track* tr = step->GetTrack();
    if (tr->GetDefinition() != G4Gamma::GammaDefinition()) return;
    if (tr->GetCurrentStepNumber() != 1) return;
    // 在粒子初生点 PreStepPoint 读取真实产生动能
    G4double e0 = step->GetPreStepPoint()->GetKineticEnergy();
    G4int i = (G4int)(e0 / (EMAX*MeV) * NBIN);
    if (i >= 0 && i < NBIN) gHist[i] += 1.0;
  }
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    for (G4int i = 0; i < NBIN; i++) gHist[i] = 0.;
  }
  void EndOfRunAction(const G4Run* run) override {
    G4double n = (G4double)run->GetNumberOfEvent();
    if (n <= 0) n = 1;
    G4double tot = 0;
    for (G4int i = 0; i < NBIN; i++) tot += gHist[i];
    G4cout << "# T6-1 (Arm B) 20 MeV e- on W: bremsstrahlung spectrum" << G4endl;
    G4cout << "# events = " << run->GetNumberOfEvent() << " photons/event = " << tot/n << G4endl;
    for (G4int i = 0; i < NBIN; i++)
      G4cout << i*EMAX/NBIN << " " << (i+1)*EMAX/NBIN << " " << gHist[i] << " " << gHist[i]/n << G4endl;
  }
};

class PrimaryGenerator : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGenerator() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    fGun->SetParticleEnergy(20*MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -1*cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~PrimaryGenerator() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};

class ActionInit : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new PrimaryGenerator);
    SetUserAction(new RunAction);
    SetUserAction(new SteppingAction);
  }
};

int main(int argc, char** argv) {
  G4RunManager* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  G4PhysListFactory factory;
  rm->SetUserInitialization(factory.GetReferencePhysList("G4EmStandard_opt4"));
  rm->SetUserInitialization(new ActionInit);
  rm->Initialize();
  G4UImanager::GetUIpointer()->ApplyCommand("/run/verbose 0");
  rm->BeamOn((argc > 1) ? std::atoi(argv[1]) : 100);
  delete rm;
  return 0;
}
