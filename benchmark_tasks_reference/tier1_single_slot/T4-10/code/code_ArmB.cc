// T4-10: 高能电子穿过水体产生的契伦科夫辐射光子数与发射角锥模拟
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
#include "G4OpticalPhysics.hh"
#include "G4OpticalPhoton.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <cstdlib>

static G4long gTotalPhotons = 0, gCountInit = 0;
static G4double gSumPhotonsSq = 0., gSumThetaRel = 0., gSumThetaInit = 0., gTotalTrackLen = 0.;
static G4int gEventPhotons = 0;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* water = G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
    auto* mpt = new G4MaterialPropertiesTable();
    // 水在光学波段 (200~800 nm, 1.55~6.20 eV) 的折射率设定为 1.333
    std::vector<G4double> ePhoton = {1.55 * eV, 6.20 * eV}, rIndex = {1.333, 1.333};
    mpt->AddProperty("RINDEX", ePhoton, rIndex);
    water->SetMaterialPropertiesTable(mpt);
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 50*cm, 50*cm, 50*cm), water, "World");
    return new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
  }
};

class PhysicsList : public G4VModularPhysicsList {
public:
  PhysicsList() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4OpticalPhysics());
    SetVerboseLevel(0);
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    fGun->SetParticleEnergy(10 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -10 * cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~PrimaryGeneratorAction() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};

class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* track = step->GetTrack();
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
      track->SetTrackStatus(fStopAndKill); return; // 计分后终止光子，提高效率
    }
    if (track->GetTrackID() == 1) gTotalTrackLen += step->GetStepLength();
    const auto* secondaries = step->GetSecondaryInCurrentStep();
    if (!secondaries || secondaries->empty()) return;

    G4ThreeVector eDir = step->GetPreStepPoint()->GetMomentumDirection();
    G4double eKin = step->GetPreStepPoint()->GetKineticEnergy();
    for (const auto* sec : *secondaries) {
      if (sec->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
        gEventPhotons++;
        G4double theta = std::acos(std::max(-1.0, std::min(1.0, eDir.dot(sec->GetMomentumDirection()))));
        gSumThetaRel += theta;
        if (eKin > 9.9 * MeV) { gSumThetaInit += theta; gCountInit++; }
      }
    }
  }
};

class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEventPhotons = 0; }
  void EndOfEventAction(const G4Event*) override {
    gTotalPhotons += gEventPhotons;
    gSumPhotonsSq += (G4double)gEventPhotons * gEventPhotons;
  }
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    gTotalPhotons = 0; gSumPhotonsSq = 0.; gSumThetaRel = 0.; gSumThetaInit = 0.; gCountInit = 0; gTotalTrackLen = 0.;
  }
  void EndOfRunAction(const G4Run* run) override {
    G4long n = run->GetNumberOfEvent();
    if (n == 0 || gTotalPhotons == 0) return;
    G4double meanN = (G4double)gTotalPhotons / n;
    G4double varN = (gSumPhotonsSq / n) - (meanN * meanN);
    G4double errN = std::sqrt(std::max(0.0, varN) / n);
    G4double meanTheta = (gSumThetaRel / gTotalPhotons) * 180.0 / CLHEP::pi;
    G4double initTheta = gCountInit ? (gSumThetaInit / gCountInit) * 180.0 / CLHEP::pi : 0.0;
    G4double theoTheta = std::acos(1.0 / 1.333) * 180.0 / CLHEP::pi;
    G4double eThresh = 511.0 * (1.0 / std::sqrt(1.0 - 1.0 / (1.333 * 1.333)) - 1.0); // keV
    G4double meanLen = (gTotalTrackLen / n) / cm;

    G4cout << "\n================ T4-10 水中契伦科夫辐射模拟结果 ================" << G4endl
           << "入射电子能量              : 10.0 MeV, 统计事件数: " << n << G4endl
           << "水体折射率 n              : 1.333 (波段 200~800 nm, 1.55~6.20 eV)" << G4endl
           << "电子契伦科夫辐射阈能      : " << std::fixed << std::setprecision(2) << eThresh << " keV" << G4endl
           << "初级电子平均径迹长        : " << std::setprecision(2) << meanLen << " cm" << G4endl
           << "单电子产生契伦科夫光子数  : " << std::setprecision(1) << meanN << " +/- " << errN << G4endl
           << "单位径迹光子产额 (估算)   : " << std::setprecision(1) << (meanN / meanLen) << " photons/cm" << G4endl
           << "理论发射角半角 (beta->1)  : " << std::setprecision(2) << theoTheta << " deg" << G4endl
           << "初始发射角锥半角 (10 MeV) : " << initTheta << " deg" << G4endl
           << "全径迹平均发射角锥半角    : " << meanTheta << " deg" << G4endl
           << "================================================================" << G4endl;
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
  rm->Initialize(); rm->BeamOn(nEvents);
  delete rm; return 0;
}
