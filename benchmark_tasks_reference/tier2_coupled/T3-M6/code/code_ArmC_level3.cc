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

static G4double gPromptDose = 0;
class Det6A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* organ = new G4Sphere("Bladder", 0, 4*cm, 0, 360*deg, 0, 180*deg);
    auto* organLog = new G4LogicalVolume(organ, nist->FindOrBuildMaterial("G4_WATER"), "Bladder");
    new G4PVPlacement(nullptr, {}, organLog, "Bladder", worldLog, false, 0);
    return worldPV;
  }
};
class Phys6A : public G4VModularPhysicsList {
public:
  Phys6A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim6A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e+"));
    gun.SetParticleEnergy(0.633*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gPromptDose += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6A()); rm->SetUserInitialization(new Phys6A());
  rm->SetUserAction(new Prim6A()); rm->SetUserAction(new Step6A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M6 Arm A] Prompt Dose = " << gPromptDose << std::endl;
  std::_Exit(0);
}
