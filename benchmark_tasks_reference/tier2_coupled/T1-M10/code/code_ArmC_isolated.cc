/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M10 目标几何空间出射面", "src": "U"},
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
// ==============================================================================
// Benchmark Tier 2: T1-M10
// 宇宙线次级中子能谱垂直入射地表 2 米土壤层深穿透热化通量演化
// ==============================================================================

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Neutron.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "QGSP_BIC_HP.hh"
#include "Randomize.hh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

static const G4int NUM_DEPTH_BINS = 10;
static const G4double SOIL_DEPTH_TOTAL = 2.0 * m;
static const G4double BIN_THICKNESS = 0.2 * m;      // 20 cm per bin
static const G4double TRANSVERSE_SIZE = 4.0 * m;    // 4m x 4m transverse cross section

// Global accumulators for track-length scoring across depth bins
static G4double gTrackLenFast[NUM_DEPTH_BINS] = {0.0};
static G4double gTrackLenEpi[NUM_DEPTH_BINS] = {0.0};
static G4double gTrackLenThermal[NUM_DEPTH_BINS] = {0.0};
static G4double gTrackLenTotal[NUM_DEPTH_BINS] = {0.0};
static G4long   gBoundaryCrossingsFast[NUM_DEPTH_BINS] = {0};
static G4long   gBoundaryCrossingsThermal[NUM_DEPTH_BINS] = {0};

class Det10 : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();

    // 1. Air material for World
    auto* airMat = nist->FindOrBuildMaterial("G4_AIR");

    // 2. Realistic continental soil material (PNNL standard with moisture)
    auto* elH  = nist->FindOrBuildElement("H");
    auto* elC  = nist->FindOrBuildElement("C");
    auto* elO  = nist->FindOrBuildElement("O");
    auto* elAl = nist->FindOrBuildElement("Al");
    auto* elSi = nist->FindOrBuildElement("Si");
    auto* elK  = nist->FindOrBuildElement("K");
    auto* elCa = nist->FindOrBuildElement("Ca");
    auto* elFe = nist->FindOrBuildElement("Fe");

    auto* soilMat = new G4Material("Soil", 1.52 * g/cm3, 8);
    soilMat->AddElement(elH,  0.021);
    soilMat->AddElement(elC,  0.016);
    soilMat->AddElement(elO,  0.577);
    soilMat->AddElement(elAl, 0.050);
    soilMat->AddElement(elSi, 0.271);
    soilMat->AddElement(elK,  0.013);
    soilMat->AddElement(elCa, 0.041);
    soilMat->AddElement(elFe, 0.011);

    // 3. World volume (6m x 6m x 6m)
    auto* worldSolid = new G4Box("WorldSolid", 3.0*m, 3.0*m, 3.0*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, airMat, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 4. Soil envelope volume (4m x 4m x 2m thick)
    // Placed from z = 0 to z = 2m in world coordinates (center at z = 1.0m)
    auto* soilSolid = new G4Box("SoilEnvelopeSolid", TRANSVERSE_SIZE/2.0, TRANSVERSE_SIZE/2.0, SOIL_DEPTH_TOTAL/2.0);
    auto* soilLog = new G4LogicalVolume(soilSolid, soilMat, "SoilEnvelopeLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, SOIL_DEPTH_TOTAL/2.0), soilLog, "SoilEnvelopePhys", worldLog, false, 0);

    // 5. Ten soil layers (each 20 cm thick along z) for depth binning
    auto* layerSolid = new G4Box("SoilLayerSolid", TRANSVERSE_SIZE/2.0, TRANSVERSE_SIZE/2.0, BIN_THICKNESS/2.0);
    auto* layerLog = new G4LogicalVolume(layerSolid, soilMat, "SoilLayerLog");

    for (G4int zBin = 0; zBin < NUM_DEPTH_BINS; ++zBin) {
      G4double zCenterLocal = -SOIL_DEPTH_TOTAL/2.0 + (zBin + 0.5) * BIN_THICKNESS;
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zCenterLocal), layerLog, "SoilLayerPhys", soilLog, false, zBin);
    }

    return worldPhys;
  }
};

class Prim10 : public G4VUserPrimaryGeneratorAction {
private:
  G4ParticleGun fGun;

public:
  Prim10() : fGun(1) {
    fGun.SetParticleDefinition(G4Neutron::NeutronDefinition());
    // Vertically incident beam along +z direction
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }

  void GeneratePrimaries(G4Event* anEvent) override {
    // Secondary cosmic ray neutron energy sampling across 9 orders of magnitude
    G4double eKin = SampleCosmicNeutronEnergy();
    fGun.SetParticleEnergy(eKin);

    // Vertically incident on top surface of soil (z = 0)
    fGun.SetParticlePosition(G4ThreeVector(0, 0, -0.1*mm));
    fGun.GeneratePrimaryVertex(anEvent);
  }

private:
  G4double SampleCosmicNeutronEnergy() {
    // Cosmic ray secondary neutron spectrum at sea level:
    // 1) Evaporation peak (~1-2 MeV, from nuclear evaporation): ~45%
    // 2) Cascade/spallation peak (~100 MeV, from high-energy nucleon reactions): ~40%
    // 3) Epithermal slowing-down region (1/E spectrum, 1 eV - 100 keV): ~15%
    G4double xi = G4UniformRand();
    if (xi < 0.45) {
      // Evaporation spectrum: f(E) ~ E * exp(-E / T), T = 1.3 MeV
      G4double u1 = std::max(1e-12, G4UniformRand());
      G4double u2 = std::max(1e-12, G4UniformRand());
      G4double E = -1.3 * MeV * std::log(u1 * u2);
      return std::max(0.01 * MeV, std::min(20.0 * MeV, E));
    } else if (xi < 0.85) {
      // High-energy cascade peak around 100 MeV (log-normal distribution)
      G4double mean_ln = std::log(100.0);
      G4double sigma_ln = 0.8;
      G4double r1 = std::max(1e-12, G4UniformRand());
      G4double r2 = G4UniformRand();
      G4double z0 = std::sqrt(-2.0 * std::log(r1)) * std::cos(2.0 * M_PI * r2);
      G4double E_MeV = std::exp(mean_ln + sigma_ln * z0);
      return std::max(10.0 * MeV, std::min(1000.0 * MeV, E_MeV * MeV));
    } else {
      // 1/E epithermal slowing-down spectrum between 1 eV and 100 keV
      G4double Emin = 1.0 * eV;
      G4double Emax = 100.0 * keV;
      G4double u = G4UniformRand();
      return Emin * std::pow(Emax / Emin, u);
    }
  }
};

class Step10 : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetTrack()->GetDefinition() != G4Neutron::NeutronDefinition()) return;

    auto* prePoint = step->GetPreStepPoint();
    const auto* touchable = prePoint->GetTouchable();
    if (!touchable) return;
    auto* volume = touchable->GetVolume();
    if (!volume || volume->GetName() != "SoilLayerPhys") return;

    G4int zBin = touchable->GetCopyNumber();
    if (zBin < 0 || zBin >= NUM_DEPTH_BINS) return;

    G4double stepLen = step->GetStepLength();
    if (stepLen <= 0.0) return;

    G4double eKin = prePoint->GetKineticEnergy();

    gTrackLenTotal[zBin] += stepLen;
    if (eKin > 100.0 * keV) {
      gTrackLenFast[zBin] += stepLen;
    } else if (eKin < 0.5 * eV) {
      gTrackLenThermal[zBin] += stepLen;
    } else {
      gTrackLenEpi[zBin] += stepLen;
    }

    // Boundary crossing tally to next layer
    auto* postPoint = step->GetPostStepPoint();
    auto* postVol = postPoint->GetPhysicalVolume();
    if (postVol != volume) {
      if (eKin > 100.0 * keV) {
        gBoundaryCrossingsFast[zBin]++;
      } else if (eKin < 0.5 * eV) {
        gBoundaryCrossingsThermal[zBin]++;
      }
    }
  }
};

class Run10 : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    for (G4int i = 0; i < NUM_DEPTH_BINS; ++i) {
      gTrackLenFast[i] = 0.0;
      gTrackLenEpi[i] = 0.0;
      gTrackLenThermal[i] = 0.0;
      gTrackLenTotal[i] = 0.0;
      gBoundaryCrossingsFast[i] = 0;
      gBoundaryCrossingsThermal[i] = 0;
    }
  }

  void EndOfRunAction(const G4Run* run) override {
    G4int nEvents = run->GetNumberOfEvent();
    if (nEvents <= 0) return;

    G4double binVolumeCm3 = (TRANSVERSE_SIZE * TRANSVERSE_SIZE * BIN_THICKNESS) / cm3;

    std::cout << "\n========================================================================================\n";
    std::cout << " [T1-M10] Cosmic Ray Secondary Neutron Soil Penetration & Thermalization Profile\n";
    std::cout << " Total primaries: " << nEvents << " | Soil Depth: 2.0 m (10 bins x 20 cm)\n";
    std::cout << "========================================================================================\n";
    std::cout << std::setw(14) << "Depth Bin [cm]"
              << std::setw(18) << "Fast Fluence"
              << std::setw(18) << "Epi Fluence"
              << std::setw(18) << "Thermal Fluence"
              << std::setw(18) << "Total Fluence\n";
    std::cout << std::setw(14) << ""
              << std::setw(18) << "[cm^-2/prim]"
              << std::setw(18) << "[cm^-2/prim]"
              << std::setw(18) << "[cm^-2/prim]"
              << std::setw(18) << "[cm^-2/prim]\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (G4int i = 0; i < NUM_DEPTH_BINS; ++i) {
      G4double depthMin = i * 20.0;
      G4double depthMax = (i + 1) * 20.0;
      std::string depthLabel = std::to_string((int)depthMin) + " - " + std::to_string((int)depthMax);

      G4double phiFast = (gTrackLenFast[i] / cm) / (binVolumeCm3 * nEvents);
      G4double phiEpi  = (gTrackLenEpi[i] / cm) / (binVolumeCm3 * nEvents);
      G4double phiTh   = (gTrackLenThermal[i] / cm) / (binVolumeCm3 * nEvents);
      G4double phiTot  = (gTrackLenTotal[i] / cm) / (binVolumeCm3 * nEvents);

      std::cout << std::setw(14) << depthLabel
                << std::scientific << std::setprecision(4)
                << std::setw(18) << phiFast
                << std::setw(18) << phiEpi
                << std::setw(18) << phiTh
                << std::setw(18) << phiTot << "\n";
    }
    std::cout << "========================================================================================\n\n";
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);

  runManager->SetUserInitialization(new Det10());
  runManager->SetUserInitialization(new QGSP_BIC_HP());

  runManager->SetUserAction(new Prim10());
  runManager->SetUserAction(new Run10());
  runManager->SetUserAction(new Step10());

  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 10;
  runManager->BeamOn(nEvents);

  delete runManager;
  std::_Exit(0);
}
