// Cs-137 point source (1 mCi) -> absorbed dose rate in air at 1 m.
// Method: track-length fluence estimator in a 10 cm thick air shell centred on
// r = 1 m, converted to air kerma by (mu_en/rho) = 0.0293 cm2/g at 662 keV.
// (Secondary electrons range ~1 m in air, so local edep is not usable here.)
// build: g++ -O2 -std=c++17 code.cc -o run $(geant4-config --cflags) $(geant4-config --libs)
// run:   ./run [nEvents]     (default 10000; the estimator converges within ~1e3)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserPhysicsList.hh"
#include "G4VModularPhysicsList.hh"
#include "G4PhysListFactory.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4Run.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ios.hh"
#include <cstdlib>
#include <cmath>

static G4LogicalVolume* gShell = nullptr;  // air shell around r = 1 m
static G4double gVol = 0.;                 // shell volume
static G4double gTE = 0., gLen = 0.;       // sum of tracklength*energy, and tracklength
static G4long gEdep = 0;                   // energy-deposit steps in shell (diagnostic)

// world = air ; scoring shell = air shell from r = 95 cm to r = 105 cm
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    auto world = new G4LogicalVolume(new G4Box("World", 1.5*m, 1.5*m, 1.5*m), air, "World");
    auto wp = new G4PVPlacement(nullptr, G4ThreeVector(), world, "World", nullptr, false, 0);
    gShell = new G4LogicalVolume(new G4Sphere("Shell", 95.*cm, 105.*cm, 0., twopi, 0., pi), air, "Shell");
    new G4PVPlacement(nullptr, G4ThreeVector(), gShell, "Shell", world, false, 0);
    gVol = gShell->GetSolid()->GetCubicVolume();
    return wp;
  }
};

// isotropic 661.657 keV gamma (Ba-137m) from a point source at the origin
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Definition());
    fGun->SetParticleEnergy(661.657*keV);
    fGun->SetParticlePosition(G4ThreeVector());
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    G4double c = 2.*G4UniformRand() - 1., p = twopi*G4UniformRand(), s = std::sqrt(1. - c*c);
    fGun->SetParticleMomentumDirection(G4ThreeVector(s*std::cos(p), s*std::sin(p), c));
    fGun->GeneratePrimaryVertex(ev);
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    if (st->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() != gShell) return;
    if (st->GetTrack()->GetDefinition() != G4Gamma::Definition()) return;
    gTE += st->GetStepLength() * st->GetPreStepPoint()->GetKineticEnergy();
    gLen += st->GetStepLength();
    if (st->GetTotalEnergyDeposit() > 0.) gEdep++;
  }
};

class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    G4double n = r->GetNumberOfEvent();
    G4double psi = (gTE/n)/gVol/(MeV/cm2);      // energy fluence per gamma, MeV/cm2
    // air kerma = psi * (mu_en/rho) ;  1 MeV/g = 1.602176634e-10 Gy
    G4double gyPerGamma = psi * 0.0293 * 1.602176634e-10;
    G4double rate = gyPerGamma * 3.7e7 * 0.851; // Gy/s for 1 mCi (85.1% per decay)
    G4cout << "\n=== Cs-137 1 mCi point source: absorbed dose rate in air at 1 m ===\n"
           << "  primaries             : " << n << "\n"
           << "  energy fluence at 1 m : " << psi << " MeV/cm2 per gamma\n"
           << "  air kerma (662 keV)   : " << gyPerGamma << " Gy per gamma\n"
           << "  dose rate             : " << rate*1.e6 << " uGy/s = "
           << rate*3.6e9 << " uGy/h\n"
           << "  mean chord in shell   : " << (gLen/n)/cm << " cm   (edep steps " << gEdep << ")\n";
  }
};

int main(int argc, char** argv) {
  G4int n = (argc > 1) ? std::atoi(argv[1]) : 10000;
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(G4PhysListFactory().GetReferencePhysList("QGSP_BIC_HP"));
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new Run);
  rm->SetUserAction(new Stepping);
  rm->Initialize();
  rm->BeamOn(n);
  delete rm;
  return 0;
}
