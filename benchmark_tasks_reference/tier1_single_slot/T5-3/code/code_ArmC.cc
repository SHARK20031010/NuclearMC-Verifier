// T5-3 中子场微剂量学量 y_D (Geant4 11.2.2); 用法 ./T5-3 [nEvents]
// 球形软组织敏感体积(直径 10 um)置于真空中, 1 MeV 中子束穿过球心.
// y = eps/lbar, lbar = 4V/S = 4r/3 (球平均弦长); y_F = Σy/N; y_D = Σy^2/Σy
// 体积越小事件率越低(正比于平均弦长), 直径 1 um 需约 1e6 个历史.
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Orb.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4UserLimits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "QGSP_BIC_HP.hh"
#include "G4SystemOfUnits.hh"
#include <cstdlib>
static const G4double kR = 5. * um;               // 敏感体积半径 (直径 10 um)
static G4LogicalVolume* gSite = nullptr;
static G4double gEdep = 0.;                       // 本事件在敏感体积内的沉积能量
static G4long gN = 0;                             // 单次事件数 (eps > 0)
static G4double gSy = 0., gSy2 = 0.;              // Σy, Σy^2  [keV/um]

class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto worldS = new G4Box("World", 5 * cm, 5 * cm, 5 * cm);
    auto worldL = new G4LogicalVolume(worldS, nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto worldP = new G4PVPlacement(nullptr, {}, worldL, "World", nullptr, false, 0);
    auto siteL = new G4LogicalVolume(new G4Orb("Site", kR),
                                     nist->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP"), "Site");
    new G4PVPlacement(nullptr, {}, siteL, "Site", worldL, false, 0);
    siteL->SetUserLimits(new G4UserLimits(0.05 * um));   // 微剂量学: 限制步长
    gSite = siteL;
    return worldP;
  }
};
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(1. * MeV);
    fGun->SetParticlePosition({-1. * cm, 0., 0.});
    fGun->SetParticleMomentumDirection({1., 0., 0.});
  }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    auto pv = st->GetPreStepPoint()->GetPhysicalVolume();
    if (pv && pv->GetLogicalVolume() == gSite) gEdep += st->GetTotalEnergyDeposit();
  }
};
class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (gEdep <= 0.) return;                        // 无沉积 -> 不计为一次事件
    G4double y = (gEdep / keV) / (4. * kR / 3. / um);   // keV/um
    gN++; gSy += y; gSy2 += y * y;
  }
};
class RunAction : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* run) override {
    G4cout << "\n=== T5-3 neutron microdosimetry: site d = 10 um tissue ===" << G4endl
           << "histories = " << run->GetNumberOfEvent() << ", single events = " << gN << G4endl;
    if (gN > 0)
      G4cout << "y_F = " << gSy / gN << " keV/um\n"
             << "y_D = " << gSy2 / gSy << " keV/um  (dose-mean lineal energy)" << G4endl;
    else
      G4cout << "no deposition events: raise nEvents" << G4endl;
  }
};
int main(int argc, char** argv) {
  auto rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new RunAction);
  rm->SetUserAction(new EventAction);
  rm->SetUserAction(new Stepping);
  rm->Initialize();
  rm->BeamOn((argc > 1) ? std::atoi(argv[1]) : 100000);
  delete rm;
  return 0;
}
