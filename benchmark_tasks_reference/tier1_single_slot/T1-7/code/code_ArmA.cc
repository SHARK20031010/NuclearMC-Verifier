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
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

static G4long gTrans = 0;
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 10*cm, 10*cm, 10*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* wpv = new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
    auto* cu = new G4LogicalVolume(new G4Box("Cu", 5*cm, 5*cm, 0.05*mm), nist->FindOrBuildMaterial("G4_Cu"), "Cu");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), cu, "Cu", world, false, 0);
    return wpv;
  }
};
class Phys : public G4VModularPhysicsList {
public:
  Phys() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    SetDefaultCutValue(0.01 * mm); // 设置微米级产生截断
  }
};
class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(100 * keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-2*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() >= 0.05*mm && s->GetPreStepPoint()->GetPosition().z() < 0.05*mm) gTrans++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det()); rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim()); rm->SetUserAction(new Step());
  rm->Initialize(); rm->BeamOn(argc>1?std::atoi(argv[1]):100);
  std::cout << "T1-7 Transmitted = " << gTrans << std::endl;
  delete rm; return 0;
}
