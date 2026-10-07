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

static G4long gTrans_45 = 0;
class Det3B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 3*m, 3*m, 3*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* wallSolid = new G4Box("ConcreteWall", 1*m, 1*m, 20*cm);
    auto* wallLog = new G4LogicalVolume(wallSolid, nist->FindOrBuildMaterial("G4_CONCRETE"), "ConcreteWall");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), wallLog, "ConcreteWall", worldLog, false, 0);
    return worldPV;
  }
};
class Phys3B : public G4VModularPhysicsList {
public:
  Phys3B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim3B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-50*cm));
    G4double incident_deg = 45.0 * deg;
    G4ThreeVector dir(sin(incident_deg), 0, cos(incident_deg));
    gun.SetParticleMomentumDirection(dir);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step3B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 25*cm && s->GetPreStepPoint()->GetPosition().z() <= 25*cm) gTrans_45++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det3B()); rm->SetUserInitialization(new Phys3B());
  rm->SetUserAction(new Prim3B()); rm->SetUserAction(new Step3B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M3 Arm B] 45-deg Transmitted = " << gTrans_45 << std::endl;
  delete rm; return 0;
}
