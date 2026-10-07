#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "FTFP_BERT.hh"
#include <iostream>
#include <iomanip>

static G4long gVertices = 0;
static G4double gSumZ = 0.0;
static G4long gCountProton = 0;
static G4long gCountNeutron = 0;
static G4long gCountMeson = 0;
static const int NBINS = 10;
static G4long gZDist[NBINS] = {0};

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* tungsten = nist->FindOrBuildMaterial("G4_W");

    auto* solidWorld = new G4Box("World", 50*cm, 50*cm, 50*cm);
    auto* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
    auto* physWorld = new G4PVPlacement(nullptr, {}, logicWorld, "World", nullptr, false, 0);

    auto* solidTarget = new G4Box("Target", 10*cm, 10*cm, 20*cm);
    fTargetLV = new G4LogicalVolume(solidTarget, tungsten, "Target");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), fTargetLV, "Target", logicWorld, false, 0);

    return physWorld;
  }
  static G4LogicalVolume* GetTargetLV() { return fTargetLV; }
private:
  static inline G4LogicalVolume* fTargetLV = nullptr;
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    auto* particle = G4ParticleTable::GetParticleTable()->FindParticle("proton");
    fGun->SetParticleDefinition(particle);
    fGun->SetParticleEnergy(1.0 * GeV);
    fGun->SetParticlePosition(G4ThreeVector(0.0, 0.0, -25.0 * cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
  }
  ~PrimaryGeneratorAction() override { delete fGun; }
  void GeneratePrimaries(G4Event* event) override {
    fGun->GeneratePrimaryVertex(event);
  }
private:
  G4ParticleGun* fGun;
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* track = step->GetTrack();
    // 护栏修正：仅在次级粒子产生首步记录顶点坐标，避免在后续连续步点重复累加
    if (track->GetTrackID() > 1 && track->GetCurrentStepNumber() == 1) {
      auto* preVol = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
      if (!preVol || preVol->GetLogicalVolume() != DetectorConstruction::GetTargetLV()) return;

      auto* def = track->GetParticleDefinition();
      const G4String& name = def->GetParticleName();
      const G4String& type = def->GetParticleType();

      bool isTargetHadron = false;
      if (name == "proton") { gCountProton++; isTargetHadron = true; }
      else if (name == "neutron") { gCountNeutron++; isTargetHadron = true; }
      else if (type == "meson") { gCountMeson++; isTargetHadron = true; }

      if (isTargetHadron) {
        gVertices++;
        G4ThreeVector vtx = track->GetVertexPosition();
        G4double depth = vtx.z() - (-20.0 * cm);
        if (depth >= 0.0 && depth < 40.0 * cm) {
          gSumZ += depth;
          int bin = static_cast<int>(depth / (4.0 * cm));
          if (bin >= 0 && bin < NBINS) gZDist[bin]++;
        }
      }
    }
  }
};

int main(int argc, char** argv) {
  int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100;

  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new DetectorConstruction());
  runManager->SetUserInitialization(new FTFP_BERT());
  runManager->SetUserAction(new PrimaryGeneratorAction());
  runManager->SetUserAction(new SteppingAction());
  runManager->Initialize();

  runManager->BeamOn(nEvents);

  std::cout << "\n================ T6-10 Measurement Results ================" << std::endl;
  std::cout << "T6-10 Vertex counts = " << gVertices << std::endl;
  std::cout << "Protons: " << gCountProton << ", Neutrons: " << gCountNeutron
            << ", Mesons: " << gCountMeson << std::endl;
  if (gVertices > 0) {
    std::cout << "Mean vertex depth = " << (gSumZ / gVertices) / cm << " cm" << std::endl;
  }
  std::cout << "Longitudinal Production Density Distribution (dN/dz per primary):" << std::endl;
  for (int i = 0; i < NBINS; ++i) {
    double zStart = i * 4.0;
    double zEnd = (i + 1) * 4.0;
    double density = static_cast<double>(gZDist[i]) / (nEvents * 4.0);
    std::cout << "  [" << std::setw(2) << zStart << " - " << std::setw(2) << zEnd << " cm]: "
              << gZDist[i] << " (density = " << density << " /cm/event)" << std::endl;
  }
  std::cout << "===========================================================" << std::endl;

  delete runManager;
  return 0;
}
