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
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "G4StepLimiterPhysics.hh"
#include "G4UserLimits.hh"
static G4double gShellEdep = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 10*um, 10*um, 10*um), nist->FindOrBuildMaterial("G4_WATER"), "W");
    auto* au = nist->FindOrBuildMaterial("G4_Au");
    auto* water = nist->FindOrBuildMaterial("G4_WATER");
    auto* np = new G4LogicalVolume(new G4Sphere("GoldNP", 0, 25*nm, 0, 360*deg, 0, 180*deg), au, "GoldNP");
    // 护栏修复: 为界面水层挂载 G4StepLimiter 限制微步长为 10 nm
    auto* shell = new G4LogicalVolume(new G4Sphere("WaterShell", 25*nm, 125*nm, 0, 360*deg, 0, 180*deg), water, "WaterShell");
    shell->SetUserLimits(new G4UserLimits(10*nm));
    new G4PVPlacement(nullptr, {0,0,0}, np, "GoldNP", world, false, 0);
    new G4PVPlacement(nullptr, {0,0,0}, shell, "WaterShell", world, false, 1);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(50 * keV);
    gun.SetParticlePosition({0,0,-1*um});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (pv && pv->GetName() == "WaterShell") gShellEdep += s->GetTotalEnergyDeposit();
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());
  pl->RegisterPhysics(new G4StepLimiterPhysics());
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "T5-M2 StepLimiter Configured, Interface Edep = " << gShellEdep/keV << " keV" << std::endl;
  delete rm; return 0;
}
