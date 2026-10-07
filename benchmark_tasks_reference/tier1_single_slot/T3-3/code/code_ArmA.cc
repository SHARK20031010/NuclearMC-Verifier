// T3-3: residual dose rate of an irradiated sample after a cooling time t_cool.
// Sample = activated point source (Na-24: E_g = 1.37 MeV, T1/2 = 15 h, A0 = 1 GBq at
// the sample itself = point source at the centre).  Air kerma at R = 10 cm from the
// K = sum(L*E*mu_en/rho)/V in a shell 9.5-10.5 cm; D_dot(t) = K*A0*exp(-lam*t).
// mu_en/rho = 0.0266 cm^2/g (air, ~1 MeV).  Usage: ./T3-3 [nevents]
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4EmStandardPhysics.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const G4double kR = 10.0*cm, kT = 0.5*cm, kE = 1.37*MeV, kMu = 0.0266; // cm^2/g
static const G4double kA0 = 1.0e9;                 // activity at end of irradiation [Bq]
static const G4double kT12 = 14.997*3600.;         // half-life [s] (Na-24)
static G4double gK = 0.;                           // track-length kerma sum [MeV*cm^3/g]
class Sens : public G4VSensitiveDetector {         // track-length kerma scorer
public:
  Sens() : G4VSensitiveDetector("shell") {}
  G4bool ProcessHits(G4Step* st, G4TouchableHistory*) override {
    gK += (st->GetStepLength()/cm) * st->GetTrack()->GetKineticEnergy()/MeV * kMu;
    return true;
  }
};
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4Material* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    G4LogicalVolume* lw = new G4LogicalVolume(new G4Box("w", 30*cm, 30*cm, 30*cm), air, "world");
    G4LogicalVolume* ld = new G4LogicalVolume(new G4Sphere("sD", kR-kT, kR+kT, 0, 360*deg, 0, 180*deg), air, "doseShell");
    new G4PVPlacement(0, G4ThreeVector(), ld, "pD", lw, false, 0);
    G4VSensitiveDetector* sd = new Sens;
    G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    ld->SetSensitiveDetector(sd);
    return new G4PVPlacement(0, G4ThreeVector(), lw, "physWorld", 0, false, 0);
  }
};
class Prim : public G4VUserPrimaryGeneratorAction { // isotropic gammas from the sample
  G4ParticleGun* fGun;
public:
  Prim() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticleEnergy(kE);
    fGun->SetParticlePosition(G4ThreeVector());
  }
  void GeneratePrimaries(G4Event* ev) override {
    G4double cz = 2.*G4UniformRand() - 1., rho = std::sqrt(1. - cz*cz), ph = 2.*M_PI*G4UniformRand();
    fGun->SetParticleMomentumDirection(G4ThreeVector(rho*std::cos(ph), rho*std::sin(ph), cz));
    fGun->GeneratePrimaryVertex(ev);
  }
};
class RunAct : public G4UserRunAction {             // per cooling time: reset + report
  G4double fTc;
public:
  RunAct() : fTc(0.) {}
  void SetTcool(G4double t) { fTc = t; }
  void BeginOfRunAction(const G4Run*) override { gK = 0.; }
  void EndOfRunAction(const G4Run* run) override {
    G4double vol = (4./3.)*M_PI*(std::pow((kR+kT)/cm, 3) - std::pow((kR-kT)/cm, 3)); // cm^3
    G4double lam = std::log(2.)/kT12;
    G4double A = kA0*std::exp(-lam*fTc);                              // Bq at t_cool
    G4double kerma = gK/vol*1.602176634e-10/run->GetNumberOfEvent();  // Gy per primary
    G4double rate = kerma*A;                                          // Gy/s
    std::printf("t_cool=%10.4e s (%9.4f d)  A(t)=%9.3e Bq  D_dot=%9.3e Gy/s = %9.3f uGy/h\n",
                fTc, fTc/86400., A, rate, rate*3600./1e-6);
  }
};
int main(int argc, char** argv) {
  G4int nev = (argc > 1) ? atoi(argv[1]) : 20000;
  G4RunManager* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  G4VModularPhysicsList* pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics());
  pl->SetVerboseLevel(0);
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());       // G4RunManager API: no ActionInitialization needed
  RunAct* ra = new RunAct;
  rm->SetUserAction(ra);
  rm->Initialize();
  std::printf("--- T3-3 residual dose rate: A0=%.3g Bq, E=%.3f MeV, T1/2=%.4f h, kerma at R=%.1f+-%.1f cm, %d primaries ---\n",
              kA0, kE/MeV, kT12/3600., kR/cm, kT/cm, nev);
  const G4double tc[5] = {0., 3600., 6*3600., 86400., 7*86400.};
  for (G4int i = 0; i < 5; i++) { ra->SetTcool(tc[i]); rm->BeamOn(nev); }
  delete rm;                                   // clean shutdown (silences store warnings)
  return 0;
}
