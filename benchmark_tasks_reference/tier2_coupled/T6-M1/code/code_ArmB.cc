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

static G4double gCenterEdep = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 1.5*m, 1.5*m, 1.5*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* target = new G4LogicalVolume(new G4Box("Target", 10*cm, 10*cm, 10*cm), nist->FindOrBuildMaterial("G4_WATER"), "Target");
    new G4PVPlacement(nullptr, {0,0,0}, target, "Target", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6 * MeV);
    // 自检盲区: 增加了发散角扰动，但发射主轴依然固定，未动态指向等中心
    G4double phi = G4UniformRand() * 2.0 * M_PI;
    G4double R = 1.0 * m;
    G4ThreeVector pos(R * std::cos(phi), R * std::sin(phi), 0);
    gun.SetParticlePosition(pos);
    gun.SetParticleMomentumDirection({0.01 * G4UniformRand(), 0.01 * G4UniformRand(), 1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (pv && pv->GetName() == "Target") gCenterEdep += s->GetTotalEnergyDeposit();
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
  std::cout << "T6-M1 Self-check Center Edep = " << gCenterEdep/MeV << " MeV" << std::endl;
  delete rm; return 0;
}
