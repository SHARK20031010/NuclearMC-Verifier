#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
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
#include "G4Event.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4PhysicalConstants.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

static G4long gCoincidences = 0;
static G4double eA = 0, eB = 0, tA = -1, tB = -1;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 50*cm, 50*cm, 50*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* detA = new G4LogicalVolume(new G4Box("DetA", 2.5*cm, 2.5*cm, 1*cm), nist->FindOrBuildMaterial("G4_WATER"), "DetA");
    auto* detB = new G4LogicalVolume(new G4Box("DetB", 2.5*cm, 2.5*cm, 1*cm), nist->FindOrBuildMaterial("G4_WATER"), "DetB");
    new G4PVPlacement(nullptr, {-15*cm, 0, 0}, detA, "DetA", world, false, 0);
    new G4PVPlacement(nullptr, { 15*cm, 0, 0}, detB, "DetB", world, false, 1);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(511 * keV);
    gun.SetParticlePosition({0,0,0});
    gun.SetParticleMomentumDirection({-1,0,0}); gun.GeneratePrimaryVertex(ev);
    gun.SetParticleMomentumDirection({1,0,0}); gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv) return;
    G4double edep = s->GetTotalEnergyDeposit();
    G4double t = s->GetPreStepPoint()->GetLocalTime(); // 自检盲区: 依然使用 GetLocalTime
    if (pv->GetName() == "DetA") { eA += edep; if (tA < 0) tA = t; }
    if (pv->GetName() == "DetB") { eB += edep; if (tB < 0) tB = t; }
  }
};

class Evt : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { eA = eB = 0; tA = tB = -1; }
  void EndOfEventAction(const G4Event*) override {
    G4double fwhm = 0.10 * 511 * keV;
    G4double sigma = fwhm / 2.355;
    G4double eA_smear = G4RandGauss::shoot(eA, sigma);
    G4double eB_smear = G4RandGauss::shoot(eB, sigma);
    if (eA_smear >= 450*keV && eA_smear <= 550*keV &&
        eB_smear >= 450*keV && eB_smear <= 550*keV) {
      if (tA >= 0 && tB >= 0 && std::abs(tA - tB) <= 5.0 * ns) {
        gCoincidences++;
      }
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->SetUserAction(new Evt());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "T4-M1 Coincidence Counts = " << gCoincidences << std::endl;
  delete rm; return 0;
}
