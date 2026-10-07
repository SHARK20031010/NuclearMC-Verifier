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

static G4double gWaterDose = 0;
class Det1A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* slabSolid = new G4Box("FlatFilter", 5*cm, 5*cm, 1*cm);
    auto* slabLog = new G4LogicalVolume(slabSolid, nist->FindOrBuildMaterial("G4_Cu"), "FlatFilter");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-20*cm), slabLog, "FlatFilter", worldLog, false, 0);

    auto* phantomSolid = new G4Box("WaterTank", 15*cm, 15*cm, 15*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterTank");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15*cm), phantomLog, "WaterTank", worldLog, false, 1);
    return worldPV;
  }
};
class Phys1A : public G4VModularPhysicsList {
public:
  Phys1A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim1A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-30*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step1A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gWaterDose += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det1A()); rm->SetUserInitialization(new Phys1A());
  rm->SetUserAction(new Prim1A()); rm->SetUserAction(new Step1A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M1 Arm A] Water Dose = " << gWaterDose << std::endl;
  std::_Exit(0);
}
