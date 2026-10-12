// T4-8: 硅半导体探测器测 5.5 MeV 阿尔法粒子电荷收集信号本征能量涨落方差
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
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
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <cstdlib>

static G4double gEventEdep = 0.0;
static G4double gTotalEdep = 0.0;
static G4double gTotalEdepSq = 0.0;
static G4int gTotalEvents = 0;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vac = nist->FindOrBuildMaterial("G4_Galactic");
    auto* si = nist->FindOrBuildMaterial("G4_Si");

    auto* worldBox = new G4Box("World", 10.0 * cm, 10.0 * cm, 10.0 * cm);
    auto* worldLV = new G4LogicalVolume(worldBox, vac, "World");

    // 硅半导体探测器: 1 cm x 1 cm x 300 um (5.5 MeV alpha 在硅中射程约 28 um，完全阻滞吸收)
    auto* detBox = new G4Box("Detector", 0.5 * cm, 0.5 * cm, 150.0 * um);
    auto* detLV = new G4LogicalVolume(detBox, si, "Detector");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 150.0 * um), detLV, "Detector", worldLV, false, 0);

    return new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
  }
};

class PhysicsList : public G4VModularPhysicsList {
public:
  PhysicsList() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    SetVerboseLevel(0);
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    auto* alpha = G4ParticleTable::GetParticleTable()->FindParticle("alpha");
    fGun->SetParticleDefinition(alpha);
    fGun->SetParticleEnergy(5.5 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~PrimaryGeneratorAction() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetName() == "Detector") {
      gEventEdep += step->GetTotalEnergyDeposit();
    }
  }
};

class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEventEdep = 0.0; }
  void EndOfEventAction(const G4Event*) override {
    gTotalEdep += gEventEdep;
    gTotalEdepSq += gEventEdep * gEventEdep;
    gTotalEvents++;
  }
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    gTotalEdep = 0.0;
    gTotalEdepSq = 0.0;
    gTotalEvents = 0;
  }
  void EndOfRunAction(const G4Run* run) override {
    G4long n = run->GetNumberOfEvent();
    if (n == 0) return;
    G4double meanE = gTotalEdep / n;
    G4double varE_geom = (gTotalEdepSq / n) - (meanE * meanE);
    if (varE_geom < 0.0) varE_geom = 0.0;

    // 半导体电离产生平均能量 w (Si: 3.6 eV) 与 Fano 因子 F = 0.115
    const G4double w = 3.6 * eV;
    const G4double Fano = 0.115;
    G4double meanN = meanE / w;

    // 本征电荷产生统计起伏方差: Var(N) = Fano * <N> (受 Fano 因子强烈抑制，非纯泊松 N)
    G4double var_N = Fano * meanN;
    G4double sigma_N = std::sqrt(var_N);

    // 等效本征能量展宽 (keV)
    G4double sigma_E = (sigma_N * w) / keV;
    G4double fwhm_E = 2.355 * sigma_E;

    G4cout << "\n================ T4-8 硅半导体探测器响应模拟 ================" << G4endl;
    G4cout << "入射粒子能量              : 5.5 MeV (alpha), 事件数: " << n << G4endl;
    G4cout << "平均沉积能量 <Edep>       : " << meanE / MeV << " MeV" << G4endl;
    G4cout << "平均产生电子-空穴对数 <N> : " << std::fixed << std::setprecision(1) << meanN << G4endl;
    G4cout << "半导体 Fano 因子 (F)      : " << std::setprecision(3) << Fano << G4endl;
    G4cout << "电荷收集本征方差 Var(N)   : " << std::setprecision(2) << var_N << G4endl;
    G4cout << "本征电荷起伏标准差 sigma  : " << std::setprecision(2) << sigma_N << " pairs" << G4endl;
    G4cout << "本征能量分辨 sigma_E/FWHM : " << std::setprecision(3) << sigma_E << " keV / "
           << fwhm_E << " keV" << G4endl;
    G4cout << "=============================================================" << G4endl;

    std::cout << "T4-8 Charge Variance = " << var_N << std::endl;
  }
};

int main(int argc, char** argv) {
  G4long nEvents = (argc > 1) ? std::atol(argv[1]) : 100;
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new DetectorConstruction());
  rm->SetUserInitialization(new PhysicsList());
  rm->SetUserAction(new PrimaryGeneratorAction());
  rm->SetUserAction(new RunAction());
  rm->SetUserAction(new EventAction());
  rm->SetUserAction(new SteppingAction());
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
