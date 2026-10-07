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

static G4double gIonizationEdep = 0;
class Det8A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* glass = new G4Box("GlassBlock", 10*cm, 10*cm, 10*cm);
    auto* glassLog = new G4LogicalVolume(glass, nist->FindOrBuildMaterial("G4_Pyrex_Glass"), "GlassBlock");
    new G4PVPlacement(nullptr, {}, glassLog, "GlassBlock", worldLog, false, 0);
    return worldPV;
  }
};
class Phys8A : public G4VModularPhysicsList {
public:
  Phys8A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim8A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(1000020040)); // He4
    gun.SetParticleEnergy(5.5*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gIonizationEdep += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8A()); rm->SetUserInitialization(new Phys8A());
  rm->SetUserAction(new Prim8A()); rm->SetUserAction(new Step8A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M8 Arm A] Ionization Edep = " << gIonizationEdep << std::endl;
  delete rm; return 0;
}
