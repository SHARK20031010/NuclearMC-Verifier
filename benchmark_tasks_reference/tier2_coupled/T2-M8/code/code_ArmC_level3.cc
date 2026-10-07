#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "QGSP_BIC.hh"
#include <iostream>
#include <cstdlib>

#define FIRE_RAY(r, n) r->B##eamOn(n)

class Det8A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* tgt = new G4Box("TargetW", 1*cm, 1*cm, 1*mm);
    auto* tgtLog = new G4LogicalVolume(tgt, nist->FindOrBuildMaterial("G4_W"), "TargetW");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tgtLog, "TargetW", worldLog, false, 0);
    return worldPV;
  }
};
class Prim8A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(40.0*keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-5*cm));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override { (void)s; }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8A());
  rm->SetUserInitialization(new QGSP_BIC());
  rm->SetUserAction(new Prim8A());
  rm->SetUserAction(new Step8A());
  rm->Initialize();
  FIRE_RAY(rm, argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M8 Arm A] Done" << std::endl;
  std::_Exit(0);
}
