/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M1 目标几何空间出射面", "src": "U"},
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
#include "G4Step.hh"
#include "G4Track.hh"
#include "QGSP_BIC_HP.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iomanip>

// Global tallies for maze leakage observables
static const int NUM_BINS = 10;
static G4long gPrimaryCount = 0;
static G4long gGammaExitCount = 0;
static G4long gNeutronExitCount = 0;
static G4double gGammaExitFluence = 0.0;
static G4double gNeutronExitFluence = 0.0;
static G4double gGammaSpectrum[NUM_BINS] = {0.0};
static G4double gNeutronSpectrum[NUM_BINS] = {0.0};
static const G4double gExitArea = 1.0 * m * 1.0 * m; // 1m x 1m exit aperture

// Detector Construction: Concrete shield wall with a 2-bend labyrinth maze channel
class MazeDetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* airMat = nist->FindOrBuildMaterial("G4_AIR");
    auto* concreteMat = nist->FindOrBuildMaterial("G4_CONCRETE");

    // World Volume (Air)
    auto* worldSolid = new G4Box("World", 4.0 * m, 3.0 * m, 4.0 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, airMat, "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // Thick Concrete Shield Wall
    // Center at (0, 0, 0), half-dimensions: x=2.5m, y=1.5m, z=2.5m (total 5m x 3m x 5m)
    auto* shieldSolid = new G4Box("ConcreteMazeWall", 2.5 * m, 1.5 * m, 2.5 * m);
    auto* shieldLog = new G4LogicalVolume(shieldSolid, concreteMat, "ConcreteMazeWall");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), shieldLog, "ConcreteMazeWall", worldLog, false, 0);

    // 2-Bend Maze Duct (1m x 1m cross-section, two 90-degree corners):
    // Leg 1 (entrance duct along +Z): x in [-1.5m, -0.5m], y in [-0.5m, 0.5m], z in [-2.5m, -0.5m]
    auto* leg1Solid = new G4Box("MazeLeg1", 0.5 * m, 0.5 * m, 1.0 * m);
    auto* leg1Log = new G4LogicalVolume(leg1Solid, airMat, "MazeLeg1");
    new G4PVPlacement(nullptr, G4ThreeVector(-1.0 * m, 0.0, -1.5 * m), leg1Log, "MazeLeg1", shieldLog, false, 0);

    // Leg 2 (transverse corridor along +X): x in [-1.5m, 1.5m], y in [-0.5m, 0.5m], z in [-0.5m, 0.5m]
    // Connects Bend 1 at x = -1.0m to Bend 2 at x = +1.0m
    auto* leg2Solid = new G4Box("MazeLeg2", 1.5 * m, 0.5 * m, 0.5 * m);
    auto* leg2Log = new G4LogicalVolume(leg2Solid, airMat, "MazeLeg2");
    new G4PVPlacement(nullptr, G4ThreeVector(0.0, 0.0, 0.0), leg2Log, "MazeLeg2", shieldLog, false, 0);

    // Leg 3 (exit duct along +Z): x in [0.5m, 1.5m], y in [-0.5m, 0.5m], z in [0.5m, 2.5m]
    auto* leg3Solid = new G4Box("MazeLeg3", 0.5 * m, 0.5 * m, 1.0 * m);
    auto* leg3Log = new G4LogicalVolume(leg3Solid, airMat, "MazeLeg3");
    new G4PVPlacement(nullptr, G4ThreeVector(1.0 * m, 0.0, 1.5 * m), leg3Log, "MazeLeg3", shieldLog, false, 0);

    // Exit Boundary Scoring Slice: 1m x 1m surface at the exit plane (z = 2.505m)
    auto* exitSolid = new G4Box("ExitDetector", 0.5 * m, 0.5 * m, 0.005 * m);
    auto* exitLog = new G4LogicalVolume(exitSolid, airMat, "ExitDetector");
    new G4PVPlacement(nullptr, G4ThreeVector(1.0 * m, 0.0, 2.505 * m), exitLog, "ExitDetector", worldLog, false, 0);

    return worldPV;
  }
};

// Primary Generator: 2 MeV Gamma and U-235 Fission Fast Neutron Mixed Field
class MazePrimaryGenerator : public G4VUserPrimaryGeneratorAction {
private:
  G4ParticleGun* fGun;

  // Watt fission spectrum sampling for U-235 fast neutrons: f(E) ~ exp(-E/a) * sinh(sqrt(b*E))
  // a = 0.988 MeV, b = 2.249 MeV^-1, mean energy ~ 2.0 MeV
  G4double SampleWattEnergy() {
    const G4double a = 0.988 * MeV;
    const G4double b = 2.249 / MeV;
    while (true) {
      G4double E = G4UniformRand() * 15.0 * MeV;
      if (E <= 0.0) continue;
      G4double f = std::exp(-E / a) * std::sinh(std::sqrt(b * E));
      if (G4UniformRand() * 0.85 < f) {
        return E;
      }
    }
  }

public:
  MazePrimaryGenerator() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0)); // Injected forward into Leg 1
  }

  ~MazePrimaryGenerator() override {
    delete fGun;
  }

  void GeneratePrimaries(G4Event* event) override {
    gPrimaryCount++;
    // Uniform planar sampling over 1m x 1m entrance cross section
    G4double x = -1.0 * m + (G4UniformRand() - 0.5) * 1.0 * m;
    G4double y = (G4UniformRand() - 0.5) * 1.0 * m;
    G4double z = -2.5 * m;
    fGun->SetParticlePosition(G4ThreeVector(x, y, z));

    // 50% 2.0 MeV Gamma, 50% Fission Fast Neutron
    if (G4UniformRand() < 0.5) {
      fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
      fGun->SetParticleEnergy(2.0 * MeV);
    } else {
      fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
      fGun->SetParticleEnergy(SampleWattEnergy());
    }

    fGun->GeneratePrimaryVertex(event);
  }
};

// Stepping Action: Unbiased surface crossing tally and spectrum score at exit
class MazeSteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* prePV = step->GetPreStepPoint()->GetPhysicalVolume();
    auto* postPV = step->GetPostStepPoint()->GetPhysicalVolume();

    // Boundary crossing: entering ExitDetector from upstream
    if (postPV && postPV->GetName() == "ExitDetector" && prePV != postPV) {
      auto* track = step->GetTrack();
      G4String pName = track->GetParticleDefinition()->GetParticleName();
      G4double kinE = step->GetPreStepPoint()->GetKineticEnergy();
      G4ThreeVector dir = step->GetPreStepPoint()->GetMomentumDirection();
      G4double cosTheta = std::abs(dir.z());
      if (cosTheta < 0.05) cosTheta = 0.05; // grazing angle divergence cutoff

      if (pName == "gamma") {
        gGammaExitCount++;
        gGammaExitFluence += 1.0 / (cosTheta * (gExitArea / cm2));
        int bin = static_cast<int>(kinE / (0.25 * MeV));
        if (bin >= 0 && bin < NUM_BINS) {
          gGammaSpectrum[bin] += 1.0;
        } else if (bin >= NUM_BINS) {
          gGammaSpectrum[NUM_BINS - 1] += 1.0;
        }
      } else if (pName == "neutron") {
        gNeutronExitCount++;
        gNeutronExitFluence += 1.0 / (cosTheta * (gExitArea / cm2));
        int bin = static_cast<int>(kinE / (1.0 * MeV));
        if (bin >= 0 && bin < NUM_BINS) {
          gNeutronSpectrum[bin] += 1.0;
        } else if (bin >= NUM_BINS) {
          gNeutronSpectrum[NUM_BINS - 1] += 1.0;
        }
      }

      // Stop and kill track at scoring plane to prevent boundary multi-crossing distortion
      track->SetTrackStatus(fStopAndKill);
    }
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;

  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);

  runManager->SetUserInitialization(new MazeDetectorConstruction());
  runManager->SetUserInitialization(new QGSP_BIC_HP()); // Complete EM + HP Neutron physics
  runManager->SetUserAction(new MazePrimaryGenerator());
  runManager->SetUserAction(new MazeSteppingAction());

  runManager->Initialize();
  runManager->BeamOn(nEvents);

  // Compute normalized flux and fluence per source particle
  G4double norm = (gPrimaryCount > 0) ? (1.0 / gPrimaryCount) : 1.0;
  G4double exitAreaCm2 = gExitArea / cm2;
  G4double gamma_flux = (gGammaExitCount * norm) / exitAreaCm2;
  G4double neutron_flux = (gNeutronExitCount * norm) / exitAreaCm2;
  G4double gamma_fluence = gGammaExitFluence * norm;
  G4double neutron_fluence = gNeutronExitFluence * norm;

  std::cout << "\n================ [T1-M1 Maze Shielding Benchmark Summary] ================\n";
  std::cout << "Total Primaries: " << gPrimaryCount << "\n";
  std::cout << "Exit Aperture Area: " << exitAreaCm2 << " cm^2\n";
  std::cout << "-------------------------------------------------------------------------\n";
  std::cout << "Gamma Exit Count: " << gGammaExitCount << "\n";
  std::cout << "Gamma Exit Flux (per source, cm^-2): " << gamma_flux << "\n";
  std::cout << "Gamma Exit Fluence (per source, cm^-2): " << gamma_fluence << "\n";
  std::cout << "Gamma Energy Spectrum (0 - 2.5 MeV, 0.25 MeV/bin):\n  ";
  for (int i = 0; i < NUM_BINS; ++i) {
    std::cout << "[" << i * 0.25 << "-" << (i + 1) * 0.25 << " MeV]: " << gGammaSpectrum[i] * norm << "  ";
  }
  std::cout << "\n-------------------------------------------------------------------------\n";
  std::cout << "Neutron Exit Count: " << gNeutronExitCount << "\n";
  std::cout << "Neutron Exit Flux (per source, cm^-2): " << neutron_flux << "\n";
  std::cout << "Neutron Exit Fluence (per source, cm^-2): " << neutron_fluence << "\n";
  std::cout << "Neutron Energy Spectrum (0 - 10.0 MeV, 1.0 MeV/bin):\n  ";
  for (int i = 0; i < NUM_BINS; ++i) {
    std::cout << "[" << i * 1.0 << "-" << (i + 1) * 1.0 << " MeV]: " << gNeutronSpectrum[i] * norm << "  ";
  }
  std::cout << "\n=========================================================================\n" << std::endl;

  delete runManager;
  std::_Exit(0);
}
