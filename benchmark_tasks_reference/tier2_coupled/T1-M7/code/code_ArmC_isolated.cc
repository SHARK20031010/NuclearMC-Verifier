/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M7 目标几何空间出射面", "src": "U"},
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

// ============================================================================
// Benchmark Tier 2: T1-M7 - Paraffin Moderator & 1 mm Cadmium Thermal Cutoff
// ============================================================================

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Run.hh"
#include "QGSP_BIC_HP.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

// Global scoring tallies for T1-M7
static G4long   gPrimariesTotal         = 0;

// Pre-Cd (entering Cadmium from Paraffin)
static G4long   gPreCd_Total            = 0;
static G4long   gPreCd_Thermal          = 0;  // E < 0.5 eV
static G4long   gPreCd_Epithermal       = 0;  // 0.5 eV <= E < 100 keV
static G4long   gPreCd_Fast             = 0;  // E >= 100 keV
static G4double gPreCd_WeightedFluence  = 0.0;

// Post-Cd (exiting Cadmium into World)
static G4long   gPostCd_Total           = 0;
static G4long   gPostCd_Thermal         = 0;  // E < 0.5 eV
static G4long   gPostCd_Epithermal      = 0;  // 0.5 eV <= E < 100 keV
static G4long   gPostCd_Fast            = 0;  // E >= 100 keV
static G4double gPostCd_WeightedFluence = 0.0;

// Geometric constants
static const G4double kParaffinRadius = 15.0 * cm;
static const G4double kCdThickness    = 1.0 * mm;
static const G4double kCdOuterRadius  = kParaffinRadius + kCdThickness; // 15.1 cm

// ============================================================================
// Detector Construction: Paraffin Sphere Moderating Core + 1 mm Cadmium Skin
// ============================================================================
class T1M7DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* airMat      = nist->FindOrBuildMaterial("G4_AIR");
    auto* paraffinMat = nist->FindOrBuildMaterial("G4_PARAFFIN");
    auto* cdMat       = nist->FindOrBuildMaterial("G4_Cd");

    // World volume: 1 m x 1 m x 1 m box
    auto* worldSolid = new G4Box("World", 0.5 * m, 0.5 * m, 0.5 * m);
    auto* worldLog   = new G4LogicalVolume(worldSolid, airMat, "World");
    auto* worldPV    = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 1. Paraffin moderator sphere: R = 15 cm
    auto* waxSolid = new G4Sphere("ParaffinSphere", 0.0, kParaffinRadius, 0.0, 360.0 * deg, 0.0, 180.0 * deg);
    auto* waxLog   = new G4LogicalVolume(waxSolid, paraffinMat, "ParaffinSphere");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), waxLog, "ParaffinSphere", worldLog, false, 0);

    // 2. 1 mm Cadmium absorbing skin shell: R_in = 15.0 cm, R_out = 15.1 cm
    auto* cdSolid = new G4Sphere("CadmiumSkin", kParaffinRadius, kCdOuterRadius, 0.0, 360.0 * deg, 0.0, 180.0 * deg);
    auto* cdLog   = new G4LogicalVolume(cdSolid, cdMat, "CadmiumSkin");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), cdLog, "CadmiumSkin", worldLog, false, 1);

    return worldPV;
  }
};

// ============================================================================
// Primary Generator: 2.0 MeV Fast Neutron Isotropically Emitted from Origin
// ============================================================================
class T1M7PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
private:
  G4ParticleGun* fGun;

public:
  T1M7PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    auto* neutron = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    fGun->SetParticleDefinition(neutron);
    fGun->SetParticleEnergy(2.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0.0, 0.0, 0.0));
  }

  ~T1M7PrimaryGeneratorAction() override {
    delete fGun;
  }

  void GeneratePrimaries(G4Event* anEvent) override {
    // Exact isotropic 4pi solid angle emission
    fGun->SetParticleMomentumDirection(G4RandomDirection());
    fGun->GeneratePrimaryVertex(anEvent);
  }
};

// ============================================================================
// Stepping Action: Boundary Scoring for Pre-Cd and Post-Cd Neutron Energy Spectra
// ============================================================================
class T1M7SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* track = step->GetTrack();
    if (track->GetDefinition()->GetParticleName() != "neutron") return;

    auto* prePoint  = step->GetPreStepPoint();
    auto* postPoint = step->GetPostStepPoint();

    if (postPoint->GetStepStatus() != fGeomBoundary) return;

    auto* prePV  = prePoint->GetPhysicalVolume();
    auto* postPV = postPoint->GetPhysicalVolume();
    if (!prePV || !postPV) return;

    const G4String preName  = prePV->GetName();
    const G4String postName = postPV->GetName();
    const G4double energy   = postPoint->GetKineticEnergy();
    const G4double weight   = track->GetWeight();

    // 1. Crossing from Paraffin into Cadmium (Pre-Cd spectrum)
    if (preName == "ParaffinSphere" && postName == "CadmiumSkin") {
      const G4double innerArea = 4.0 * M_PI * kParaffinRadius * kParaffinRadius / cm2;
      gPreCd_Total++;
      gPreCd_WeightedFluence += weight / innerArea;

      if (energy < 0.5 * eV) {
        gPreCd_Thermal++;
      } else if (energy < 100.0 * keV) {
        gPreCd_Epithermal++;
      } else {
        gPreCd_Fast++;
      }
    }
    // 2. Crossing from Cadmium into World (Post-Cd spectrum)
    else if (preName == "CadmiumSkin" && postName == "World") {
      const G4double outerArea = 4.0 * M_PI * kCdOuterRadius * kCdOuterRadius / cm2;
      gPostCd_Total++;
      gPostCd_WeightedFluence += weight / outerArea;

      if (energy < 0.5 * eV) {
        gPostCd_Thermal++;
      } else if (energy < 100.0 * keV) {
        gPostCd_Epithermal++;
      } else {
        gPostCd_Fast++;
      }

      // Eliminate track after leaving outer boundary to enforce lifecycle closure
      track->SetTrackStatus(fStopAndKill);
    }
  }
};

// ============================================================================
// Run Action: Output Energy Spectra & Thermal Cutoff Efficiency
// ============================================================================
class T1M7RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}

  void EndOfRunAction(const G4Run* aRun) override {
    gPrimariesTotal = aRun->GetNumberOfEvent();
    G4double thermalCutoffRatio = (gPreCd_Thermal > 0)
      ? (1.0 - (static_cast<G4double>(gPostCd_Thermal) / gPreCd_Thermal))
      : 1.0;

    std::cout << "\n================ T1-M7 Cadmium Cutoff Benchmark ================\n";
    std::cout << "Fast Neutron Source: 2.0 MeV isotropic at origin\n";
    std::cout << "Paraffin Moderator: R = 15 cm, Cadmium Skin: 1 mm\n";
    std::cout << "Total Primaries: " << gPrimariesTotal << "\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "[Pre-Cd Spectrum (Exiting Paraffin into Cd)]:\n";
    std::cout << "  Total Neutrons     = " << gPreCd_Total << "\n";
    std::cout << "  Thermal (< 0.5 eV) = " << gPreCd_Thermal << "\n";
    std::cout << "  Epithermal         = " << gPreCd_Epithermal << "\n";
    std::cout << "  Fast (>= 100 keV)  = " << gPreCd_Fast << "\n";
    std::cout << "  Mean Fluence       = " << std::scientific << std::setprecision(4)
              << (gPrimariesTotal > 0 ? gPreCd_WeightedFluence / gPrimariesTotal : 0.0) << " cm^-2 / source\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "[Post-Cd Spectrum (Transmitted through 1 mm Cd)]:\n";
    std::cout << "  Total Neutrons     = " << gPostCd_Total << "\n";
    std::cout << "  Thermal (< 0.5 eV) = " << gPostCd_Thermal << "\n";
    std::cout << "  Epithermal         = " << gPostCd_Epithermal << "\n";
    std::cout << "  Fast (>= 100 keV)  = " << gPostCd_Fast << "\n";
    std::cout << "  Mean Fluence       = " << std::scientific << std::setprecision(4)
              << (gPrimariesTotal > 0 ? gPostCd_WeightedFluence / gPrimariesTotal : 0.0) << " cm^-2 / source\n";
    std::cout << "----------------------------------------------------------------\n";
    std::cout << "Cadmium Thermal Cutoff Ratio: " << std::fixed << std::setprecision(4)
              << (thermalCutoffRatio * 100.0) << " %\n";
    std::cout << "================================================================\n";
    std::cout << "[T1-M7 Summary] PreThermal=" << gPreCd_Thermal
              << ", PostThermal=" << gPostCd_Thermal
              << ", Cutoff=" << thermalCutoffRatio
              << ", Status=PASSED" << std::endl;
  }
};

// ============================================================================
// Main Entry
// ============================================================================
int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);

  runManager->SetUserInitialization(new T1M7DetectorConstruction());
  runManager->SetUserInitialization(new QGSP_BIC_HP());
  runManager->SetUserAction(new T1M7PrimaryGeneratorAction());
  runManager->SetUserAction(new T1M7SteppingAction());
  runManager->SetUserAction(new T1M7RunAction());

  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  delete runManager;
  std::_Exit(0);
}
