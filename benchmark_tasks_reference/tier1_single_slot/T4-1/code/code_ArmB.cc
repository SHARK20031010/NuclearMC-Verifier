// T4-1: HPGe 探测器测 Cs-137 (661.657 keV) 全能峰效率
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o T4-1
// 运行: ./T4-1 [事件数] (默认 100000)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "Randomize.hh"
#include <cmath>
#include <iomanip>
#include <cstdlib>

static G4LogicalVolume* gGe = nullptr;
static G4double gEdep = 0.;

// 探测器几何：同轴 HPGe 晶体 (直径 6 cm, 长度 5 cm)，前表面在 z = 0
class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 20 * cm, 20 * cm, 20 * cm),
                                        nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    gGe = new G4LogicalVolume(new G4Tubs("GeCrystal", 0, 3 * cm, 2.5 * cm, 0, 360 * deg),
                              nist->FindOrBuildMaterial("G4_Ge"), "GeCrystal");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 2.5 * cm), gGe, "GeCrystal", worldLV, false, 0);
    return worldPV;
  }
};

// 初级粒子源：Cs-137 点源位于 (0, 0, -5 cm)，向 4pi 各向同性发射 661.657 keV 伽马
class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    fGun->SetParticleEnergy(661.657 * keV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -5 * cm));
  }
  ~PrimaryGeneratorAction() override { delete fGun; }
  void GeneratePrimaries(G4Event* event) override {
    // 4pi 空间各向同性发射角抽样 (消除平行束导致的虚高)
    G4double cosTheta = 2.0 * G4UniformRand() - 1.0;
    G4double sinTheta = std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));
    G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
    fGun->SetParticleMomentumDirection(
        G4ThreeVector(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta));
    fGun->GeneratePrimaryVertex(event);
  }
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() == gGe)
      gEdep += step->GetTotalEnergyDeposit();
  }
};

class EventAction : public G4UserEventAction {
  G4long fPeak = 0, fHit = 0;
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (gEdep > 0.) fHit++;
    // 全能峰计数 (661.657 keV 能量全部沉积在探测器中)
    if (std::abs(gEdep - 661.657 * keV) < 1.0 * keV) fPeak++;
  }
  G4long GetPeak() const { return fPeak; }
  G4long GetHit() const { return fHit; }
};

class RunAction : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* run) override {
    auto* ea = static_cast<const EventAction*>(
        G4RunManager::GetRunManager()->GetUserEventAction());
    G4long n = run->GetNumberOfEvent(), peak = ea->GetPeak(), hit = ea->GetHit();
    G4double absEff = n ? (G4double)peak / n : 0., absErr = n ? std::sqrt((G4double)peak) / n : 0.;
    G4double intEff = hit ? (G4double)peak / hit : 0., intErr = hit ? std::sqrt((G4double)peak) / hit : 0.;

    G4cout << "\n================ Cs-137 (661.657 keV) HPGe 全能峰效率 ================" << G4endl
           << "发射光子总数 (4pi)    : " << n << G4endl
           << "锗晶体中作用事件数    : " << hit << " (" << std::fixed << std::setprecision(2)
           << (n ? 100.0 * hit / n : 0.0) << " %)" << G4endl
           << "全能峰计数            : " << peak << G4endl
           << "绝对全能峰效率 (4pi基准): " << std::fixed << std::setprecision(4)
           << (absEff * 100.0) << " % +/- " << (absErr * 100.0) << " %" << G4endl
           << "本征全能峰效率 (晶体截面): " << std::fixed << std::setprecision(2)
           << (intEff * 100.0) << " % +/- " << (intErr * 100.0) << " %" << G4endl
           << "====================================================================" << G4endl;
  }
};

int main(int argc, char** argv) {
  G4long nEvents = (argc > 1) ? std::atol(argv[1]) : 100000;
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new DetectorConstruction);
  auto* pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics_option4);
  pl->SetVerboseLevel(0);
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new PrimaryGeneratorAction);
  rm->SetUserAction(new RunAction);
  rm->SetUserAction(new EventAction);
  rm->SetUserAction(new SteppingAction);
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
