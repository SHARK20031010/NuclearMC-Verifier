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

static G4double gTotalLight = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 20*cm, 20*cm, 20*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* scint = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
    // 护栏修复: 为闪烁体材料设置 SetBirksConstant 猝灭模型
    scint->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);
    auto* det = new G4LogicalVolume(new G4Tubs("Scint", 0, 2.5*cm, 2.5*cm, 0, 360*deg), scint, "Scint");
    new G4PVPlacement(nullptr, {0,0,0}, det, "Scint", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(5 * MeV);
    gun.SetParticlePosition({0,0,-5*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    // 考虑 Birks 猝灭后的光输出
    G4double edep = s->GetTotalEnergyDeposit();
    G4double stepLength = s->GetStepLength();
    if (stepLength > 0 && edep > 0) {
      G4double kB = 0.126 * mm / MeV;
      G4double quenched = edep / (1.0 + kB * (edep / stepLength));
      gTotalLight += quenched;
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
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "T4-M3 Birks Quenched Light Yield = " << gTotalLight/MeV << " MeVee" << std::endl;
  delete rm; return 0;
}
