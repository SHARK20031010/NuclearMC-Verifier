#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4Cons.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

static G4long gMoSpectrum = 0;
class Det8B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 通用自检：钼靶 (Mo) 与铍窗 (Be)
    auto* tgt = new G4Box("MoTarget", 1*cm, 1*cm, 1*mm);
    auto* tgtLog = new G4LogicalVolume(tgt, nist->FindOrBuildMaterial("G4_Mo"), "MoTarget");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tgtLog, "MoTarget", worldLog, false, 0);

    auto* win = new G4Box("BeWindow", 2*cm, 2*cm, 0.4*mm);
    auto* winLog = new G4LogicalVolume(win, nist->FindOrBuildMaterial("G4_Be"), "BeWindow");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,2*cm), winLog, "BeWindow", worldLog, false, 1);
    return worldPV;
  }
};
class Phys8B : public G4VModularPhysicsList {
public:
  Phys8B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim8B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(40.0*keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-5*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 5*cm) gMoSpectrum++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8B()); rm->SetUserInitialization(new Phys8B());
  rm->SetUserAction(new Prim8B()); rm->SetUserAction(new Step8B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M8 Arm B] Mo-Be spectrum count = " << gMoSpectrum << std::endl;
  delete rm; return 0;
}
