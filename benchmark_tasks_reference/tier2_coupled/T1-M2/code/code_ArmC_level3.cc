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

static G4double gDose_Pb = 0;
class Det2A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* pbSolid = new G4Tubs("PbCylinder", 0, 30*cm, 50*cm, 0, 360*deg);
    auto* pbLog = new G4LogicalVolume(pbSolid, nist->FindOrBuildMaterial("G4_Pb"), "PbCylinder");
    new G4PVPlacement(nullptr, {}, pbLog, "PbCylinder", worldLog, false, 0);
    return worldPV;
  }
};
class Phys2A : public G4VModularPhysicsList {
public:
  Phys2A() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim2A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(2112));
    gun.SetParticleEnergy(2.1*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4ThreeVector(1,0,0));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step2A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gDose_Pb += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det2A()); rm->SetUserInitialization(new Phys2A());
  rm->SetUserAction(new Prim2A()); rm->SetUserAction(new Step2A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M2 Arm A] Total Pb Dose = " << gDose_Pb << std::endl;
  std::_Exit(0);
}
