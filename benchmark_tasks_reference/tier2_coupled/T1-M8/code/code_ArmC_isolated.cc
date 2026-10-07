/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M8 目标几何空间出射面", "src": "U"},
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
// Benchmark Tier 2: T1-M8 - Stray Neutron Attenuation in Ceiling S-Duct
//
// Physical Scenario:
//   230 MeV protons impinge on a water phantom target in a proton therapy vault.
//   Inelastic nuclear interactions produce stray secondary neutrons.
//   Neutrons stream into a ventilation duct embedded in the concrete ceiling.
//   The duct features an S-shape with two 90-degree bends to eliminate line-of-sight.
//   Neutrons undergo albedo scattering off the concrete walls.
//   We calculate the exit neutron fluence and attenuation factor relative to
//   the target source strength.
// ============================================================================

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4UnionSolid.hh"
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
#include "QGSP_BIC_HP.hh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <cmath>
#include <cstdlib>
#include <string>

// Global scoring observables
static G4long   gPrimaryProtons      = 0;
static G4double gTargetNeutronYield  = 0.0;
static G4double gDuctInletCount      = 0.0;
static G4double gExitNeutronCount    = 0.0;
static G4double gExitFluence         = 0.0;

// Geometry parameters
static const G4double kExitArea = 30.0 * cm * 30.0 * cm; // 900 cm^2

// Event-level track deduplication container
static std::set<G4int> gScoredExitTracks;

// ============================================================================
// Detector Construction: Treatment Vault, Water Target, Ceiling & S-Duct
// ============================================================================
class DetectorConstruction8 : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air      = nist->FindOrBuildMaterial("G4_AIR");
    auto* water    = nist->FindOrBuildMaterial("G4_WATER");
    auto* concrete = nist->FindOrBuildMaterial("G4_CONCRETE");

    // 1. World Volume: 6 m x 6 m x 6 m air box
    auto* worldSolid = new G4Box("World", 3.0*m, 3.0*m, 3.0*m);
    auto* worldLog   = new G4LogicalVolume(worldSolid, air, "World");
    auto* worldPV    = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 2. Water Target: 30 cm x 30 cm x 40 cm (stopping 230 MeV protons, range ~33 cm)
    // Centered at (0, 0, 20 cm) so proton enters at z = 0
    auto* targetSolid = new G4Box("WaterTarget", 15.0*cm, 15.0*cm, 20.0*cm);
    auto* targetLog   = new G4LogicalVolume(targetSolid, water, "WaterTarget");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 20.0*cm), targetLog, "WaterTarget", worldLog, false, 0);

    // 3. Concrete Ceiling: 4 m x 1 m x 4 m slab
    // Placed at y = 2.0 m, spans y in [1.5 m, 2.5 m]
    auto* ceilingSolid = new G4Box("ConcreteCeiling", 2.0*m, 0.5*m, 2.0*m);
    auto* ceilingLog   = new G4LogicalVolume(ceilingSolid, concrete, "ConcreteCeiling");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 2.0*m, 0), ceilingLog, "ConcreteCeiling", worldLog, false, 0);

    // 4. S-shaped Ventilation Duct (曲折通风管道 / 两次弯管迷宫结构)
    // Duct cross section: 30 cm x 30 cm (half: 15 cm x 15 cm)
    // Leg 1 (Vertical inlet): local y in [-0.5 m, -0.1 m], z = -0.3 m
    auto* leg1Solid = new G4Box("DuctLeg1", 15.0*cm, 20.0*cm, 15.0*cm);
    // Leg 2 (Horizontal intermediate): local y in [-0.15 m, 0.15 m], z in [-0.4 m, 0.4 m]
    auto* leg2Solid = new G4Box("DuctLeg2", 15.0*cm, 15.0*cm, 40.0*cm);
    // Leg 3 (Vertical outlet): local y in [0.1 m, 0.5 m], z = +0.3 m
    auto* leg3Solid = new G4Box("DuctLeg3", 15.0*cm, 20.0*cm, 15.0*cm);

    // Union of Leg 2 with Leg 1 at z = -30 cm (Elbow 1 / 第一弯管)
    auto* duct12Solid = new G4UnionSolid("Duct12", leg2Solid, leg1Solid, nullptr,
                                         G4ThreeVector(0, -30.0*cm, -30.0*cm));
    // Union with Leg 3 at z = +30 cm (Elbow 2 / 第二弯管)
    auto* sDuctSolid  = new G4UnionSolid("SDuctSolid", duct12Solid, leg3Solid, nullptr,
                                         G4ThreeVector(0, 30.0*cm, 30.0*cm));

    // Place S-Duct inside Concrete Ceiling as air cavity
    auto* sDuctLog = new G4LogicalVolume(sDuctSolid, air, "SDuct");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), sDuctLog, "SDuct", ceilingLog, false, 0);

    // 5. Duct Exit Scoring Volume (排风口出射面计分体)
    // Placed at the top exit of Leg 3 (y = 2.505 m, z = 30 cm in World)
    auto* exitSolid = new G4Box("DuctExit", 15.0*cm, 0.5*cm, 15.0*cm);
    auto* exitLog   = new G4LogicalVolume(exitSolid, air, "DuctExit");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 2.505*m, 30.0*cm), exitLog, "DuctExit", worldLog, false, 0);

    return worldPV;
  }
};

// ============================================================================
// Primary Generator Action: 230 MeV Proton Beam on Water Target
// ============================================================================
class PrimaryGeneratorAction8 : public G4VUserPrimaryGeneratorAction {
public:
  PrimaryGeneratorAction8() : fGun(1) {
    auto* proton = G4ParticleTable::GetParticleTable()->FindParticle("proton");
    fGun.SetParticleDefinition(proton);
    fGun.SetParticleEnergy(230.0 * MeV);
    fGun.SetParticlePosition(G4ThreeVector(0, 0, 0));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1.0));
  }

  void GeneratePrimaries(G4Event* ev) override {
    gPrimaryProtons++;
    fGun.GeneratePrimaryVertex(ev);
  }

private:
  G4ParticleGun fGun;
};

// ============================================================================
// Event Action: Clear track de-duplication cache per event
// ============================================================================
class EventAction8 : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    gScoredExitTracks.clear();
  }
};

// ============================================================================
// Stepping Action: Lifecycle-isolated Secondary and Boundary Crossing Scoring
// ============================================================================
class SteppingAction8 : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* track = step->GetTrack();
    const auto* pDef = track->GetParticleDefinition();
    const G4String pName = pDef->GetParticleName();

    // 1. Target Stray Neutron Production Lifecycle Filter:
    // Only tally at birth step (GetCurrentStepNumber() == 1) for secondaries in WaterTarget
    if (track->GetCurrentStepNumber() == 1 && track->GetParentID() > 0 && pName == "neutron") {
      auto* prePV = step->GetPreStepPoint()->GetPhysicalVolume();
      if (prePV && prePV->GetName() == "WaterTarget") {
        gTargetNeutronYield += track->GetWeight();
      }
    }

    // 2. Duct Entrance Inflow Check (entering SDuct from World/Air)
    if (pName == "neutron" && step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) {
      auto* postPV = step->GetPostStepPoint()->GetPhysicalVolume();
      auto* prePV  = step->GetPreStepPoint()->GetPhysicalVolume();
      if (prePV && prePV->GetName() == "World" && postPV && postPV->GetName() == "SDuct") {
        gDuctInletCount += track->GetWeight();
      }
    }

    // 3. Duct Exit Outflow Scoring (entering DuctExit scoring plane from SDuct)
    if (pName == "neutron" && step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) {
      auto* postPV = step->GetPostStepPoint()->GetPhysicalVolume();
      if (postPV && postPV->GetName() == "DuctExit") {
        G4int trackID = track->GetTrackID();
        if (gScoredExitTracks.find(trackID) == gScoredExitTracks.end()) {
          gScoredExitTracks.insert(trackID);
          G4double w = track->GetWeight();
          gExitNeutronCount += w;
          gExitFluence      += w / (kExitArea / cm2); // cm^-2
          // Absorb after scoring to prevent double counting
          track->SetTrackStatus(fStopAndKill);
        }
      }
    }
  }
};

// ============================================================================
// Main Simulation Entry
// ============================================================================
int main(int argc, char** argv) {
  int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  if (nEvents <= 0) nEvents = 50;

  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);

  // Detector construction
  runManager->SetUserInitialization(new DetectorConstruction8());

  // High-precision physics list with BIC and HP for proton/neutron interactions
  runManager->SetUserInitialization(new QGSP_BIC_HP());

  // User actions
  runManager->SetUserAction(new PrimaryGeneratorAction8());
  runManager->SetUserAction(new EventAction8());
  runManager->SetUserAction(new SteppingAction8());

  // Initialize and run
  runManager->Initialize();
  runManager->BeamOn(nEvents);

  // Physical results calculation
  G4double exitAreaCm2 = kExitArea / cm2;
  G4double attenRelativeToTarget = (gTargetNeutronYield > 0.0)
                                     ? (gExitNeutronCount / gTargetNeutronYield)
                                     : 0.0;
  G4double attenFluencePerTarget = (gTargetNeutronYield > 0.0)
                                     ? (gExitFluence / gTargetNeutronYield)
                                     : 0.0;
  G4double exitFluencePerProton  = (gPrimaryProtons > 0)
                                     ? (gExitFluence / gPrimaryProtons)
                                     : 0.0;

  std::cout << "\n================ [T1-M8 Simulation Summary] ================\n";
  std::cout << "Primary Protons (230 MeV):      " << gPrimaryProtons << "\n";
  std::cout << "Water Target Neutron Yield:     " << gTargetNeutronYield << "\n";
  std::cout << "Neutrons Entering Duct Inlet:   " << gDuctInletCount << "\n";
  std::cout << "Neutrons Exiting Duct (2 Bends):" << gExitNeutronCount << "\n";
  std::cout << "Duct Exit Surface Area:         " << exitAreaCm2 << " cm^2\n";
  std::cout << "Duct Exit Neutron Fluence:      " << gExitFluence << " cm^-2\n";
  std::cout << "Attenuation (Exit / Target S0): " << std::scientific << std::setprecision(4)
            << attenRelativeToTarget << "\n";
  std::cout << "Fluence Atten (Fluence / S0):   " << std::scientific << std::setprecision(4)
            << attenFluencePerTarget << " cm^-2\n";
  std::cout << "Exit Fluence Per Source Proton: " << std::scientific << std::setprecision(4)
            << exitFluencePerProton << " cm^-2\n";
  std::cout << "============================================================\n" << std::endl;

  delete runManager;
  std::_Exit(0);
}
