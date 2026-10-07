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

static G4double gEvtHpge = 0, gEvtBgo = 0;
static G4double gAcceptedHpge = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 20*cm, 20*cm, 20*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* hpge = new G4LogicalVolume(new G4Tubs("HPGe", 0, 2*cm, 2.5*cm, 0, 360*deg), nist->FindOrBuildMaterial("G4_Ge"), "HPGe");
    // 护栏修复: 建立 BGO 反符合环
    auto* bgo = new G4LogicalVolume(new G4Tubs("BGO_Ring", 2.2*cm, 5*cm, 3*cm, 0, 360*deg), nist->FindOrBuildMaterial("G4_BGO"), "BGO_Ring");
    new G4PVPlacement(nullptr, {0,0,0}, hpge, "HPGe", world, false, 0);
    new G4PVPlacement(nullptr, {0,0,0}, bgo, "BGO_Ring", world, false, 1);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.332 * MeV);
    gun.SetParticlePosition({0,0,-10*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv) return;
    if (pv->GetName() == "HPGe") gEvtHpge += s->GetTotalEnergyDeposit();
    if (pv->GetName() == "BGO_Ring") gEvtBgo += s->GetTotalEnergyDeposit();
  }
};

class Evt : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEvtHpge = gEvtBgo = 0; }
  void EndOfEventAction(const G4Event*) override {
    // 反符合判定: 若 BGO 在符合窗内有信号，则予以 veto 舍弃
    if (gEvtHpge > 0 && gEvtBgo < 50*keV) {
      gAcceptedHpge += gEvtHpge;
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
  std::cout << "T4-M9 BGO Anti-coincidence Veto Suppressed, Net HPGe = " << gAcceptedHpge/MeV << " MeV" << std::endl;
  delete rm; return 0;
}
