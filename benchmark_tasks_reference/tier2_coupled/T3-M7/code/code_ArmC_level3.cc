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
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

static G4long gAuCaptures = 0;
class Det7A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* foil = new G4Box("AuFoil", 1*cm, 1*cm, 0.05*mm);
    auto* foilLog = new G4LogicalVolume(foil, nist->FindOrBuildMaterial("G4_Au"), "AuFoil");
    new G4PVPlacement(nullptr, {}, foilLog, "AuFoil", worldLog, false, 0);
    return worldPV;
  }
};
class Phys7A : public G4VModularPhysicsList {
public:
  Phys7A() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim7A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(0.025*eV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step7A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetVolume() && s->GetTrack()->GetVolume()->GetName() == "AuFoil") gAuCaptures++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det7A()); rm->SetUserInitialization(new Phys7A());
  rm->SetUserAction(new Prim7A()); rm->SetUserAction(new Step7A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M7 Arm A] Bare Au captures = " << gAuCaptures << std::endl;
  std::_Exit(0);
}
