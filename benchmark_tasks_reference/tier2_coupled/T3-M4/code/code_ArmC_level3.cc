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

static G4long gSingleHits = 0;
class Det4A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* naiSolid = new G4Tubs("NaIDet", 0, 4*cm, 4*cm, 0, 360*deg);
    auto* naiLog = new G4LogicalVolume(naiSolid, nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "NaIDet");
    new G4PVPlacement(nullptr, {}, naiLog, "NaIDet", worldLog, false, 0);
    return worldPV;
  }
};
class Phys4A : public G4VModularPhysicsList {
public:
  Phys4A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim4A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(1.369*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step4A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTotalEnergyDeposit() > 1.0*MeV) gSingleHits++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det4A()); rm->SetUserInitialization(new Phys4A());
  rm->SetUserAction(new Prim4A()); rm->SetUserAction(new Step4A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M4 Arm A] Hits = " << gSingleHits << std::endl;
  std::_Exit(0);
}
