// T5-1 (Arm B): TEPC 组织等效正比计数器微剂量线能谱模拟
// 自查复核：根据 ICRU 微剂量学严格定义，线能 y = epsilon / l_bar
// 对于半径 r = 0.5 um 的球形灵敏体积，平均弦长为 4r/3 = 0.667 um，而非直径 1 um。
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4NistManager.hh"
#include "G4Orb.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "QGSP_BIC_HP.hh"
#include <cmath>

static const int    kNBin = 40;            // 0-200 keV/um, 5 keV/um per bin
static const double kBinW = 5.;            // keV/um
static const double kRadius = 0.5 * um;    // 半径 r = 0.5 um
// ICRU 平均弦长 4r/3
static const double meanChord = (4.0/3.0) * kRadius;
static double fSpectrum[kNBin];
static long   fNhit = 0;
static double fSumY = 0., fEdep = 0.;

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4Material* te = G4NistManager::Instance()->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
    auto* lWorld = new G4LogicalVolume(new G4Orb("World", 50. * um), te, "World");
    auto* pWorld = new G4PVPlacement(nullptr, {}, lWorld, "World", nullptr, false, 0);
    auto* lCav = new G4LogicalVolume(new G4Orb("Cavity", kRadius), te, "Cavity");
    new G4PVPlacement(nullptr, {}, lCav, "Cavity", lWorld, false, 0);
    return pWorld;
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4VPhysicalVolume* pv = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv || pv->GetName() != "Cavity") return;
    G4double e = step->GetTotalEnergyDeposit();
    if (e > 0.) fEdep += e;
  }
};

class Primary : public G4VUserPrimaryGeneratorAction {
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    double th = std::acos(2. * G4UniformRand() - 1.);
    double ph = CLHEP::twopi * G4UniformRand();
    fGun->SetParticleEnergy(G4UniformRand() * 1. * MeV + 1. * eV);
    fGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
    fGun->SetParticleMomentumDirection(
        G4ThreeVector(std::sin(th) * std::cos(ph), std::sin(th) * std::sin(ph), std::cos(th)));
    fGun->GeneratePrimaryVertex(ev);
  }
private:
  G4ParticleGun* fGun;
};

class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { fEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (fEdep > 0.) {
      double y = (fEdep / keV) / (meanChord / um);
      fNhit++;
      fSumY += y;
      int ib = static_cast<int>(y / kBinW);
      if (ib >= kNBin) ib = kNBin - 1;
      fSpectrum[ib] += 1.;
    }
  }
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    fNhit = 0;
    fSumY = 0.;
    for (int i = 0; i < kNBin; ++i) fSpectrum[i] = 0.;
  }
  void EndOfRunAction(const G4Run* run) override {
    G4cout << "\n# TEPC lineal-energy spectrum (Arm B 自查修正版)\n"
           << "# Mean chord length l_bar = 4r/3 = " << meanChord / um << " um\n"
           << "# hits = " << fNhit << ", mean y = " << (fNhit ? fSumY / fNhit : 0.)
           << " keV/um\n";
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new Stepping);
  rm->SetUserAction(new EventAction);
  rm->SetUserAction(new RunAction);
  rm->Initialize();
  G4UImanager::GetUIpointer()->ApplyCommand("/run/verbose 0");
  rm->BeamOn((argc > 1) ? std::atoi(argv[1]) : 100);
  delete rm;
  return 0;
}
