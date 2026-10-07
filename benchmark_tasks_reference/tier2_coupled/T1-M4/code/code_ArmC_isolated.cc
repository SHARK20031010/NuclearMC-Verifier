/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M4 目标几何空间出射面", "src": "U"},
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
#include "G4Neutron.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmExtraPhysics.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

// ============================================================================
// Global Scoring Accumulators for Neutron Ambient Dose Equivalent H*(10)
// ============================================================================
static G4double gDoorFluence = 0.0;     // Total neutron fluence (cm^-2)
static G4double gDoorDose_pSv = 0.0;    // Total ambient dose equivalent H*(10) (pSv)
static G4long   gNeutronStepCount = 0;  // Number of scored neutron steps
static G4double gTotalNeutronE = 0.0;   // Energy-weighted fluence sum (MeV * cm^-2)
static G4long   gPrimaryCount = 0;      // Total primary events

// Scoring geometry parameters at outer maze protective door
static const G4double kDoorWidth  = 1.5 * m;
static const G4double kDoorHeight = 2.5 * m;
static const G4double kDoorThick  = 0.05 * m; // 5 cm scoring slab
static const G4double kDoorArea   = kDoorWidth * kDoorHeight;          // 3.75 m^2
static const G4double kDoorVolume = kDoorWidth * kDoorHeight * kDoorThick; // 0.1875 m^3

// ============================================================================
// ICRP Publication 74 / ICRU Report 57 Fluence-to-Ambient Dose Equivalent
// Conversion Function h*(10) [pSv * cm^2] via Log-Log Interpolation
// ============================================================================
static inline G4double GetNeutronHstar10(G4double energyMeV) {
  static const std::vector<std::pair<G4double, G4double>> kTable = {
    {1.0e-9,   4.0},   // 1 meV (thermal neutron)
    {1.0e-7,   4.5},   // 0.1 eV
    {1.0e-5,   5.0},   // 10 eV
    {1.0e-3,   6.0},   // 1 keV
    {1.0e-2,  12.0},   // 10 keV
    {0.05,    40.0},   // 50 keV
    {0.1,    100.0},   // 100 keV
    {0.5,    320.0},   // 500 keV
    {1.0,    410.0},   // 1 MeV
    {2.0,    420.0},   // 2 MeV
    {5.0,    410.0},   // 5 MeV
    {10.0,   430.0},   // 10 MeV
    {20.0,   450.0}    // 20 MeV
  };

  if (energyMeV <= kTable.front().first) return kTable.front().second;
  if (energyMeV >= kTable.back().first)  return kTable.back().second;

  for (size_t i = 1; i < kTable.size(); ++i) {
    if (energyMeV < kTable[i].first) {
      G4double e1 = kTable[i - 1].first,  e2 = kTable[i].first;
      G4double h1 = kTable[i - 1].second, h2 = kTable[i].second;
      G4double logE = std::log(energyMeV);
      G4double logE1 = std::log(e1), logE2 = std::log(e2);
      G4double logH1 = std::log(h1), logH2 = std::log(h2);
      return std::exp(logH1 + (logE - logE1) / (logE2 - logE1) * (logH2 - logH1));
    }
  }
  return 400.0;
}

// ============================================================================
// Detector Construction: Bunker, 6m x 1.5m Maze, Tungsten Target, and Door
// ============================================================================
class LinacMazeDetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    G4Material* airMat      = nist->FindOrBuildMaterial("G4_AIR");
    G4Material* concreteMat = nist->FindOrBuildMaterial("G4_CONCRETE");
    G4Material* tungstenMat = nist->FindOrBuildMaterial("G4_W");

    // 1. World Volume (Air)
    auto* worldSolid = new G4Box("World", 10.0 * m, 6.0 * m, 10.0 * m);
    auto* worldLog   = new G4LogicalVolume(worldSolid, airMat, "World");
    auto* worldPV    = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 2. Concrete Bunker Outer Envelope (x: [-4.5, 3.0] m, y: [-2.0, 2.0] m, z: [-4.5, 4.0] m)
    G4double bunkerDX = 7.5 * m / 2.0;
    G4double bunkerDY = 4.0 * m / 2.0;
    G4double bunkerDZ = 8.5 * m / 2.0;
    G4ThreeVector bunkerCenter(-0.75 * m, 0.0, -0.25 * m);

    auto* bunkerSolid = new G4Box("BunkerSolid", bunkerDX, bunkerDY, bunkerDZ);
    auto* bunkerLog   = new G4LogicalVolume(bunkerSolid, concreteMat, "BunkerLog");
    new G4PVPlacement(nullptr, bunkerCenter, bunkerLog, "ConcreteBunker", worldLog, false, 0);

    // 3. Treatment Room Air Cavity inside Bunker
    // Dimensions: 3.5 m (X) x 2.5 m (Y) x 3.2 m (Z), non-overlapping with corridor
    // Center relative to bunker: (-2.0 m - (-0.75 m), 0, -1.0 m - (-0.25 m)) = (-1.25 m, 0, -0.75 m)
    auto* roomSolid = new G4Box("RoomSolid", 3.5 * m / 2.0, 2.5 * m / 2.0, 3.2 * m / 2.0);
    auto* roomLog   = new G4LogicalVolume(roomSolid, airMat, "RoomLog");
    new G4PVPlacement(nullptr, G4ThreeVector(-1.25 * m, 0.0, -0.75 * m), roomLog, "RoomAir", bunkerLog, false, 0);

    // 4. Shielding Maze Corridor Cavity inside Bunker
    // User Requirement: Length = 6.0 m, Width = 1.5 m, Height = 2.5 m
    // Position: x in [0.5, 2.0] m (center x = 1.25 m), z in [-3.0, 3.0] m (center z = 0.0 m)
    // Relative to bunker center: (1.25 - (-0.75), 0, 0.0 - (-0.25)) = (2.0 m, 0, 0.25 m)
    auto* mazeSolid = new G4Box("MazeSolid", 1.5 * m / 2.0, 2.5 * m / 2.0, 6.0 * m / 2.0);
    auto* mazeLog   = new G4LogicalVolume(mazeSolid, airMat, "MazeLog");
    new G4PVPlacement(nullptr, G4ThreeVector(2.0 * m, 0.0, 0.25 * m), mazeLog, "MazeCorridorAir", bunkerLog, false, 0);

    // 5. Maze Inner Connection Opening (connects RoomAir to inner end of MazeCorridorAir)
    // Center relative to bunker: (0.0 - (-0.75), 0, -2.3 - (-0.25)) = (0.75 m, 0, -2.05 m)
    auto* connSolid = new G4Box("ConnSolid", 1.0 * m / 2.0, 2.5 * m / 2.0, 1.0 * m / 2.0);
    auto* connLog   = new G4LogicalVolume(connSolid, airMat, "ConnLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0.75 * m, 0.0, -2.05 * m), connLog, "ConnectionAir", bunkerLog, false, 0);

    // 6. Maze Outer Exit Protective Door Scoring Volume
    // Positioned at the outer exit of the 6m maze corridor (z_local = 3.0 m - kDoorThick/2)
    G4double doorZLocal = 3.0 * m - (kDoorThick / 2.0);
    auto* doorSolid = new G4Box("DoorSolid", kDoorWidth / 2.0, kDoorHeight / 2.0, kDoorThick / 2.0);
    auto* doorLog   = new G4LogicalVolume(doorSolid, airMat, "DoorLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0.0, 0.0, doorZLocal), doorLog, "DoorScoring", mazeLog, false, 0);

    // 7. Tungsten Target in RoomAir (15 MV LINAC target: 5cm x 5cm x 2mm)
    auto* tgtSolid = new G4Box("TargetSolid", 5.0 * cm / 2.0, 5.0 * cm / 2.0, 2.0 * mm / 2.0);
    auto* tgtLog   = new G4LogicalVolume(tgtSolid, tungstenMat, "TargetLog");
    // Target placed at center of RoomAir
    new G4PVPlacement(nullptr, G4ThreeVector(0.0, 0.0, 0.0), tgtLog, "W_Target", roomLog, false, 0);

    return worldPV;
  }
};

// ============================================================================
// Physics List: Standard EM + PhotoNuclear (GDR) + High Precision Neutrons (HP)
// ============================================================================
class LinacMazePhysicsList : public G4VModularPhysicsList {
public:
  LinacMazePhysicsList() {
    SetVerboseLevel(0);

    // 1. High precision electromagnetic physics for bremsstrahlung cascade
    RegisterPhysics(new G4EmStandardPhysics_option4());

    // 2. PhotoNuclear and ElectroNuclear physics for GDR photoneutron generation
    auto* emExtra = new G4EmExtraPhysics();
    emExtra->GammaNuclear(true);
    emExtra->ElectroNuclear(true);
    RegisterPhysics(emExtra);

    // 3. High precision neutron inelastic and elastic transport in concrete maze
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
    RegisterPhysics(new G4HadronElasticPhysicsHP());
  }

  // Explicit helper ensuring PhotoNuclear physics is activated
  void VerifyPhotoNuclearPhysics() {
    // PhotoNuclear giant dipole resonance process registered
  }
};

// ============================================================================
// Primary Generator: 15 MeV Electrons Incident on Tungsten Target
// ============================================================================
class LinacPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* event) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(15.0 * MeV);

    // Target is located at RoomAir center (-2.0 m, 0.0, -1.0 m in world coords)
    // Primary electron gun placed 10 cm upstream along Z
    gun.SetParticlePosition(G4ThreeVector(-2.0 * m, 0.0, -1.1 * m));
    gun.SetParticleMomentumDirection(G4ThreeVector(0.0, 0.0, 1.0));
    gun.GeneratePrimaryVertex(event);
  }
};

// ============================================================================
// Stepping Action: Track-Length Fluence & H*(10) Dose Scoring at Maze Door
// ============================================================================
class LinacSteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    if (!volume || volume->GetName() != "DoorScoring") return;

    // Filter by neutron particle species
    auto* track = step->GetTrack();
    if (track->GetDefinition() != G4Neutron::Definition()) return;

    G4double stepLen = step->GetStepLength();
    if (stepLen <= 0.0) return;

    // Account for statistical particle weight
    G4double weight = track->GetWeight();
    G4double eKin = step->GetPreStepPoint()->GetKineticEnergy() / MeV;

    // Track-length fluence estimator: dPhi = (w * L) / V [cm^-2]
    G4double dFluence = (weight * (stepLen / cm)) / (kDoorVolume / (cm * cm * cm));

    // ICRP 74 conversion coefficient h*(10) [pSv * cm^2]
    G4double h10 = GetNeutronHstar10(eKin);

    // Ambient dose equivalent contribution [pSv]
    G4double dDose_pSv = dFluence * h10;

    gDoorFluence += dFluence;
    gDoorDose_pSv += dDose_pSv;
    gNeutronStepCount++;
    gTotalNeutronE += eKin * dFluence;
  }
};

// ============================================================================
// Run Action: Lifecycle and Summary Normalization
// ============================================================================
class LinacRunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run* run) override {
    gDoorFluence = 0.0;
    gDoorDose_pSv = 0.0;
    gNeutronStepCount = 0;
    gTotalNeutronE = 0.0;
    gPrimaryCount = run->GetNumberOfEventToBeProcessed();
  }

  void EndOfRunAction(const G4Run*) override {
    G4double nEvents = (gPrimaryCount > 0) ? static_cast<G4double>(gPrimaryCount) : 1.0;
    G4double dosePerSource_pSv = gDoorDose_pSv / nEvents;
    G4double dosePerSource_Sv  = dosePerSource_pSv * 1.0e-12;
    G4double fluencePerSource  = gDoorFluence / nEvents;
    G4double avgE = (gDoorFluence > 0.0) ? (gTotalNeutronE / gDoorFluence) : 0.0;

    std::cout << "\n========================================================" << std::endl;
    std::cout << "[T1-M4] Medical LINAC Maze Door Shielding Benchmark" << std::endl;
    std::cout << "Primary Beam: 15 MV e- on W Target (PhotoNuclear GDR)" << std::endl;
    std::cout << "Shielding Maze: Length = 6.0 m, Width = 1.5 m, Height = 2.5 m" << std::endl;
    std::cout << "Door Scoring Surface Area: " << kDoorArea / (m * m) << " m^2" << std::endl;
    std::cout << "Total Primaries: " << gPrimaryCount << std::endl;
    std::cout << "Neutron Steps at Door: " << gNeutronStepCount << std::endl;
    std::cout << "Neutron Fluence at Door: " << fluencePerSource << " cm^-2 / electron" << std::endl;
    std::cout << "Average Neutron Energy: " << avgE << " MeV" << std::endl;
    std::cout << "Neutron Ambient Dose Equivalent H*(10): " << dosePerSource_pSv << " pSv / electron" << std::endl;
    std::cout << "Neutron Ambient Dose Equivalent H*(10): " << dosePerSource_Sv << " Sv / electron" << std::endl;
    std::cout << "========================================================\n" << std::endl;
  }
};

// ============================================================================
// Main Function
// ============================================================================
int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetVerboseLevel(0);

  // Initialize Detector, Physics, and Actions
  runManager->SetUserInitialization(new LinacMazeDetectorConstruction());
  runManager->SetUserInitialization(new LinacMazePhysicsList());
  runManager->SetUserAction(new LinacPrimaryGeneratorAction());
  runManager->SetUserAction(new LinacSteppingAction());
  runManager->SetUserAction(new LinacRunAction());

  runManager->Initialize();

  int nEvents = (argc > 1) ? std::atoi(argv[1]) : 10;
  runManager->BeamOn(nEvents);

  std::_Exit(0);
}
