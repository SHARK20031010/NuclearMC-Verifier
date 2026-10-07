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

static G4long g_step_leakage = 0;
class Det9C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 3*m, 3*m, 3*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* wallSolid = new G4Box("LeadWall", 1*m, 1*m, 5*cm);
    auto* wallLog = new G4LogicalVolume(wallSolid, nist->FindOrBuildMaterial("G4_Pb"), "LeadWall");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), wallLog, "LeadWall", worldLog, false, 0);

    auto* winSolid = new G4Box("Lead_Glass", 25*cm, 25*cm, 7.5*cm);
    auto* winLog = new G4LogicalVolume(winSolid, nist->FindOrBuildMaterial("G4_GLASS_LEAD"), "Lead_Glass");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), winLog, "Lead_Glass", worldLog, false, 1);

    auto* stepJoint = new G4Box("Z_Step_Overlap", 2*cm, 25*cm, 2*cm);
    auto* stepLog = new G4LogicalVolume(stepJoint, nist->FindOrBuildMaterial("G4_Pb"), "Z_Step_Overlap");
    new G4PVPlacement(nullptr, G4ThreeVector(26*cm, 0, 0), stepLog, "Z_Step_Overlap", worldLog, false, 2);
    return worldPV;
  }
};
class Phys9C : public G4VModularPhysicsList {
public:
  Phys9C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim9C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.33*MeV);
    gun.SetParticlePosition(G4ThreeVector(25*cm, 0, -40*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step9C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 12*cm && s->GetPreStepPoint()->GetPosition().z() <= 12*cm) {
      g_step_leakage++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det9C()); rm->SetUserInitialization(new Phys9C());
  rm->SetUserAction(new Prim9C()); rm->SetUserAction(new Step9C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M9 Arm C] Step Overlap Leakage = " << g_step_leakage << std::endl;
  delete rm; return 0;
}
