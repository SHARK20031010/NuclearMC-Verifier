// T6-3: 10 MeV electron beam into water -> depth-dose curve (Geant4 11.2.2, single file)
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include <vector>
#include <algorithm>
#include <cstdio>

static const G4int NBIN = 300;                       // 1 mm bins, 30 cm water
static std::vector<G4double> gEdep(NBIN, 0.0);

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto worldP = new G4PVPlacement(nullptr, G4ThreeVector(), 
        new G4LogicalVolume(new G4Box("World", 15*cm, 15*cm, 35*cm),
                            nist->FindOrBuildMaterial("G4_AIR"), "World"),
        "World", nullptr, false, 0);
    auto waterL = new G4LogicalVolume(new G4Box("Water", 10*cm, 10*cm, 15*cm),
                                      nist->FindOrBuildMaterial("G4_WATER"), "Water");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 15*cm), waterL, "Water",
                      worldP->GetLogicalVolume(), false, 0);   // water spans z = 0..30 cm
    return worldP;
  }
};

class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    fGun->SetParticleEnergy(10*MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -0.5*mm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};

class Scoring : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    G4double e = step->GetTotalEnergyDeposit();
    if (e <= 0) return;
    if (step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()
            ->GetLogicalVolume()->GetName() != "Water") return;
    G4double z = 0.5 * (step->GetPreStepPoint()->GetPosition().z() +
                        step->GetPostStepPoint()->GetPosition().z());
    G4int i = (G4int)(z / mm);
    if (i >= 0 && i < NBIN) gEdep[i] += e;
  }
};

class Report : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run*) override {
    G4double mx = *std::max_element(gEdep.begin(), gEdep.end());
    std::printf("# depth_mm  dose_rel\n");
    for (G4int i = 0; i < NBIN; ++i)
      std::printf("%8.1f  %.6f\n", (i + 0.5), mx > 0 ? gEdep[i] / mx : 0.0);
  }
};

int main() {
  auto runManager = new G4RunManager;
  runManager->SetUserInitialization(new Detector);
  auto physics = new G4VModularPhysicsList;
  physics->RegisterPhysics(new G4EmStandardPhysics_option4);
  runManager->SetUserInitialization(physics);
  runManager->SetUserAction(new Primary);
  runManager->SetUserAction(new Scoring);
  runManager->SetUserAction(new Report);
  runManager->Initialize();
  auto ui = G4UImanager::GetUIpointer();
  ui->ApplyCommand("/run/setCut 0.1 mm");
  ui->ApplyCommand("/run/beamOn 100");
  delete runManager;
  return 0;
}
