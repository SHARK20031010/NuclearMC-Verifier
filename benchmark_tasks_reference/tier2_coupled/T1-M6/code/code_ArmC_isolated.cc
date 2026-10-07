/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M6 目标几何空间出射面", "src": "U"},
  "F2":  {"v": "count", "src": "U"},
  "F3":  {"v": "other", "src": "A"},
  "F4":  {"v": "surface_avg", "src": "U"},
  "F5":  {"v": "steady", "src": "A"},
  "F6":  {"v": "per_source", "src": "U"},
  "F7":  {"v": "trend", "src": "A"},
  "F8":  {"v": "other_mc", "src": "A"},
  "F9":  {"v": "N/A", "src": "U"},
  "F10": {"v": "scalar", "src": "U"},
  "warnings": ["per_source_needs_strength"]
}
```
*/

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
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "QGSP_BIC_HP.hh"
#include "Randomize.hh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>

// ============================================================================
// Benchmark T1-M6: Fast Neutron Streaming through 2 mm Mechanical Slit Gap
// Between Two 20 cm Thick Stainless Steel Shielding Plates
// ============================================================================

// Geometric and beam parameters
static const G4double kPlateThickness = 20.0 * cm; // 20 cm thick along z
static const G4double kGapWidth       = 2.0 * mm;  // 2 mm mechanical installation gap
static const G4double kPlateWidth     = 20.0 * cm; // Width of each plate along x
static const G4double kPlateHeight    = 40.0 * cm; // Height of plates along y
static const G4double kBeamHalfX      = 2.0 * cm;  // Parallel beam half-width (4 cm total)
static const G4double kBeamHalfY      = 5.0 * cm;  // Parallel beam half-height (10 cm total)

// Transverse profile bins across [-20 mm, +20 mm] (40 bins, 1 mm each)
static const int kNumProfileBins = 40;

// Global tallies
static G4long gTotalPrimaries = 0;
static G4long gGapTransmitted = 0;
static G4long gSolidTransmitted = 0;
static G4long gProfileBins[kNumProfileBins] = {0};

// Detector Construction
class SlitGapDetectorConstruction : public G4VUserDetectorConstruction {
public:
  SlitGapDetectorConstruction() = default;
  ~SlitGapDetectorConstruction() override = default;

  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* airMat   = nist->FindOrBuildMaterial("G4_AIR");
    auto* steelMat = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

    // World volume
    auto* worldSolid = new G4Box("World", 1.0 * m, 1.0 * m, 1.0 * m);
    auto* worldLog   = new G4LogicalVolume(worldSolid, airMat, "World");
    auto* worldPV    = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // Two 20 cm thick stainless steel plates with a 2 mm gap
    auto* plateSolid = new G4Box("SteelPlate", kPlateWidth / 2.0, kPlateHeight / 2.0, kPlateThickness / 2.0);
    auto* plateLog   = new G4LogicalVolume(plateSolid, steelMat, "SteelPlate");

    // Left stainless steel plate
    G4double leftCenterX = -(kPlateWidth / 2.0 + kGapWidth / 2.0);
    new G4PVPlacement(nullptr, G4ThreeVector(leftCenterX, 0, 0), plateLog, "SteelPlateLeft", worldLog, false, 1);

    // Right stainless steel plate
    G4double rightCenterX = +(kPlateWidth / 2.0 + kGapWidth / 2.0);
    new G4PVPlacement(nullptr, G4ThreeVector(rightCenterX, 0, 0), plateLog, "SteelPlateRight", worldLog, false, 2);

    // Explicit 2 mm mechanical slit gap volume
    auto* slitSolid = new G4Box("SlitGap", kGapWidth / 2.0, kPlateHeight / 2.0, kPlateThickness / 2.0);
    auto* slitLog   = new G4LogicalVolume(slitSolid, airMat, "SlitGap");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), slitLog, "SlitGap", worldLog, false, 0);

    // Thin exit scoring detector immediately downstream of exit face (z = +10 cm)
    const G4double detThick = 0.2 * cm;
    auto* detSolid = new G4Box("ExitPlane", 30.0 * cm, 30.0 * cm, detThick / 2.0);
    auto* detLog   = new G4LogicalVolume(detSolid, airMat, "ExitPlane");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kPlateThickness / 2.0 + detThick / 2.0),
                      detLog, "ExitPlane", worldLog, false, 0);

    return worldPV;
  }
};

// Primary Generator: Broad Parallel Beam of 14 MeV Fast Neutrons
class SlitGapPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
  SlitGapPrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    auto* neutron = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    fGun->SetParticleDefinition(neutron);
    fGun->SetParticleEnergy(14.0 * MeV);
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }

  ~SlitGapPrimaryGeneratorAction() override {
    delete fGun;
  }

  void GeneratePrimaries(G4Event* anEvent) override {
    // Uniform parallel broad planar beam covering slit gap and adjacent steel plates
    G4double x = (2.0 * G4UniformRand() - 1.0) * kBeamHalfX;
    G4double y = (2.0 * G4UniformRand() - 1.0) * kBeamHalfY;
    G4double z = -15.0 * cm;
    fGun->SetParticlePosition(G4ThreeVector(x, y, z));
    fGun->GeneratePrimaryVertex(anEvent);
  }

private:
  G4ParticleGun* fGun;
};

// Stepping Action: Boundary Crossing Tally into ExitPlane
class SlitGapSteppingAction : public G4UserSteppingAction {
public:
  SlitGapSteppingAction() = default;
  ~SlitGapSteppingAction() override = default;

  void UserSteppingAction(const G4Step* step) override {
    auto* postPoint = step->GetPostStepPoint();
    if (!postPoint) return;

    // Detect forward boundary crossing into the ExitPlane scoring volume
    if (postPoint->GetStepStatus() == fGeomBoundary) {
      auto* postPV = postPoint->GetPhysicalVolume();
      if (postPV && postPV->GetName() == "ExitPlane") {
        auto* track = step->GetTrack();
        if (track->GetDefinition()->GetParticleName() == "neutron") {
          G4ThreeVector pos = postPoint->GetPosition();
          G4double x = pos.x();

          // 1. Gap center peak streaming (|x| <= gapWidth / 2 = 1.0 mm)
          if (std::abs(x) <= kGapWidth / 2.0) {
            gGapTransmitted++;
          }
          // 2. Surrounding solid shielding penetration (|x| in [5 mm, 20 mm])
          else if (std::abs(x) >= 5.0 * mm && std::abs(x) <= kBeamHalfX) {
            gSolidTransmitted++;
          }

          // 3. Transverse profile binning across [-20 mm, +20 mm]
          G4double x_mm = x / mm;
          int bin = static_cast<int>(std::floor((x_mm + 20.0) / 1.0));
          if (bin >= 0 && bin < kNumProfileBins) {
            gProfileBins[bin]++;
          }

          // Enforce strictly single crossing tally per transmitted particle
          track->SetTrackStatus(fStopAndKill);
        }
      }
    }
  }
};

// Run Action: Normalization to Incident Fluence and Ratio Evaluation
class SlitGapRunAction : public G4UserRunAction {
public:
  SlitGapRunAction() = default;
  ~SlitGapRunAction() override = default;

  void BeginOfRunAction(const G4Run*) override {
    gTotalPrimaries   = 0;
    gGapTransmitted   = 0;
    gSolidTransmitted = 0;
    for (int i = 0; i < kNumProfileBins; ++i) {
      gProfileBins[i] = 0;
    }
  }

  void EndOfRunAction(const G4Run* aRun) override {
    gTotalPrimaries = aRun->GetNumberOfEvent();
    if (gTotalPrimaries <= 0) return;

    G4double areaBeam  = (2.0 * kBeamHalfX) * (2.0 * kBeamHalfY);
    G4double areaGap   = kGapWidth * (2.0 * kBeamHalfY);
    G4double areaSolid = 2.0 * (kBeamHalfX - 5.0 * mm) * (2.0 * kBeamHalfY);

    // Incident parallel fluence
    G4double phi0 = static_cast<G4double>(gTotalPrimaries) / areaBeam;

    // Transmitted fluences
    G4double phiGap   = static_cast<G4double>(gGapTransmitted) / areaGap;
    G4double phiSolid = static_cast<G4double>(gSolidTransmitted) / areaSolid;

    G4double normGap   = phiGap / phi0;
    G4double normSolid = phiSolid / phi0;

    std::cout << "\n============================================================" << std::endl;
    std::cout << "  [T1-M6] 14 MeV Neutron Streaming & Shielding Penetration" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "  Total Incident Primaries (14 MeV n): " << gTotalPrimaries << std::endl;
    std::cout << "  Slit Gap Transmitted Counts:        " << gGapTransmitted << std::endl;
    std::cout << "  Solid Shield Transmitted Counts:    " << gSolidTransmitted << std::endl;
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Gap Center Peak Fluence / Phi0:     " << normGap << " (streaming peak)" << std::endl;
    std::cout << "  Solid Shield Fluence / Phi0:        " << normSolid << " (bulk penetration)" << std::endl;

    if (gSolidTransmitted > 0) {
      G4double ratio = phiGap / phiSolid;
      std::cout << "  Peak Streaming / Solid Shield Ratio:" << std::setprecision(2) << ratio << std::endl;
    } else {
      std::cout << "  Peak Streaming / Solid Shield Ratio: N/A (solid count = 0 in current statistics)" << std::endl;
    }
    std::cout << "============================================================\n" << std::endl;
  }
};

// ============================================================================
// Main Function
// ============================================================================
int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);

  // Detector construction
  runManager->SetUserInitialization(new SlitGapDetectorConstruction());

  // Complete physics list including HP elastic and inelastic neutron transport
  runManager->SetUserInitialization(new QGSP_BIC_HP());

  // User actions
  runManager->SetUserAction(new SlitGapPrimaryGeneratorAction());
  runManager->SetUserAction(new SlitGapRunAction());
  runManager->SetUserAction(new SlitGapSteppingAction());

  runManager->Initialize();

  int nEvents = (argc > 1) ? std::atoi(argv[1]) : 2000;
  runManager->BeamOn(nEvents);

  delete runManager;
  std::_Exit(0);
}
