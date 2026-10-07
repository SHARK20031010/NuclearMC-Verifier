// Ir-192 gamma spectrum with Geant4: single file, own main(), no macro/data files.
// Point source -> 3"x3" NaI(Tl), front face 5 cm away; output = pulse-height
// spectrum (energy deposited in the crystal), 5 keV bins. Usage: ./T6-4 [nEvents]
#include "G4RunManager.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4TouchableHandle.hh"
#include "Randomize.hh"
#include <cmath>
#include <cstdlib>

static const int NB = 200;                 // 5 keV bins over 0-1000 keV
static const double EMAX = 1000.*keV;
static long hist[NB] = {0};
static double eDep = 0.;

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto worldL = new G4LogicalVolume(new G4Box("World", 30*cm, 30*cm, 30*cm),
                                      nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto detL = new G4LogicalVolume(new G4Tubs("Crystal", 0, 3.81*cm, 3.81*cm, 0, 360*deg),
                                    nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "Crystal");
    new G4PVPlacement(0, G4ThreeVector(0, 0, 8.81*cm), detL, "Crystal", worldL, false, 0);
    return new G4PVPlacement(0, G4ThreeVector(), worldL, "World", 0, false, 0);
  }
};

class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* gun;
public:
  Primary() : gun(new G4ParticleGun(1)) {
    gun->SetParticleDefinition(G4Gamma::GammaDefinition());
    gun->SetParticlePosition(G4ThreeVector());
  }
  ~Primary() override { delete gun; }
  void GeneratePrimaries(G4Event* ev) override {
    const double E[7] = {295.9, 308.5, 316.5, 468.1, 588.6, 604.4, 884.5};
    const double I[7] = { 28.7,  29.7,  82.7,  47.8,   4.5,   8.2,   0.3};
    double cum[7], sum = 0.;
    for (int i = 0; i < 7; i++) { sum += I[i]; cum[i] = sum; }
    double r = G4UniformRand() * sum; int k = 6;
    for (int i = 0; i < 7; i++) { if (r <= cum[i]) { k = i; break; } }
    gun->SetParticleEnergy(E[k]*keV);
    double ct = 2.0*G4UniformRand() - 1.0;
    double ph = 2.0*M_PI*G4UniformRand();
    double st = std::sqrt(std::max(0.0, 1.0 - ct*ct));
    gun->SetParticleMomentumDirection(G4ThreeVector(st*std::cos(ph), st*std::sin(ph), ct));
    gun->GeneratePrimaryVertex(ev);
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto pre = step->GetPreStepPoint();
    if (pre && pre->GetTouchableHandle()->GetVolume() &&
        pre->GetTouchableHandle()->GetVolume()->GetName() == "Crystal") {
      eDep += step->GetTotalEnergyDeposit();
    }
  }
};

class Event : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { eDep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (eDep > 0.) {
      int b = static_cast<int>(eDep / EMAX * NB);
      if (b >= NB) b = NB - 1;
      hist[b]++;
    }
    eDep = 0.;
  }
};

class Run : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    for (int i = 0; i < NB; i++) hist[i] = 0;
  }
  void EndOfRunAction(const G4Run* r) override {
    G4cout << "# Ir-192 gamma pulse-height spectrum, NaI(Tl) 3x3 inch, 5 cm, "
           << r->GetNumberOfEvent() << " events\n# E_lo_keV\tE_hi_keV\tcounts" << G4endl;
    for (int i = 0; i < NB; i++) {
      G4cout << i*5 << "\t" << (i+1)*5 << "\t" << hist[i] << G4endl;
    }
  }
};

class Physics : public G4VModularPhysicsList {
public:
  Physics() { RegisterPhysics(new G4EmStandardPhysics()); }
};

int main(int argc, char** argv) {
  G4int n = (argc > 1) ? std::atoi(argv[1]) : 10000;
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new Physics);
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new Run);
  rm->SetUserAction(new Event);
  rm->SetUserAction(new Stepping);
  rm->Initialize();
  rm->BeamOn(n);
  delete rm;
  return 0;
}
