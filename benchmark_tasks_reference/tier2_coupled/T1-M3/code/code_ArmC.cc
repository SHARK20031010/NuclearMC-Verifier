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

static G4long gTransCounts[4] = {0,0,0,0};
class Det3C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 4*m, 4*m, 4*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* wallSolid = new G4Box("BariteConcrete", 1.5*m, 1.5*m, 20*cm);
    auto* wallLog = new G4LogicalVolume(wallSolid, nist->FindOrBuildMaterial("G4_CONCRETE"), "BariteConcrete");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), wallLog, "BariteConcrete", worldLog, false, 0);
    return worldPV;
  }
};
class Phys3C : public G4VModularPhysicsList {
public:
  Phys3C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim3C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-60*cm));
    static const double angles[4] = {0.0, 30.0, 45.0, 60.0};
    int idx = G4UniformRand() * 4;
    if (idx > 3) idx = 3;
    G4double angle_rad = angles[idx] * deg;
    G4ThreeVector dir(sin(angle_rad), 0, cos(angle_rad));
    gun.SetParticleMomentumDirection(dir);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step3C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 22*cm && s->GetPreStepPoint()->GetPosition().z() <= 22*cm) {
      gTransCounts[0]++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det3C()); rm->SetUserInitialization(new Phys3C());
  rm->SetUserAction(new Prim3C()); rm->SetUserAction(new Step3C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M3 Arm C] Angled Transmitted Total = " << gTransCounts[0] << std::endl;
  delete rm; return 0;
}
