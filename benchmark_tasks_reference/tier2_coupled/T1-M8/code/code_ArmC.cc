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

static G4long g_scattered_duct_flux = 0;
class Det8C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 6*m, 6*m, 6*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* conc = nist->FindOrBuildMaterial("G4_CONCRETE");
    auto* ductWall = new G4Box("Duct_Shield_Wall", 1*m, 1*m, 20*cm);
    auto* ductLog = new G4LogicalVolume(ductWall, conc, "Duct_Shield_Wall");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,1.2*m), ductLog, "Duct_Shield_Wall", worldLog, false, 0);

    auto* pipeAir = new G4Box("AirPipe_Duct", 20*cm, 20*cm, 80*cm);
    auto* pipeLog = new G4LogicalVolume(pipeAir, nist->FindOrBuildMaterial("G4_AIR"), "AirPipe_Duct");
    new G4PVPlacement(nullptr, G4ThreeVector(0.5*m, 0, 1.2*m), pipeLog, "AirPipe_Duct", worldLog, false, 1);
    return worldPV;
  }
};
class Phys8C : public G4VModularPhysicsList {
public:
  Phys8C() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim8C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(230.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 2.2*m && s->GetTrack()->GetParticleDefinition()->GetParticleName() == "neutron") {
      g_scattered_duct_flux++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8C()); rm->SetUserInitialization(new Phys8C());
  rm->SetUserAction(new Prim8C()); rm->SetUserAction(new Step8C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M8 Arm C] Pipe Duct Scattered Flux = " << g_scattered_duct_flux << std::endl;
  delete rm; return 0;
}
