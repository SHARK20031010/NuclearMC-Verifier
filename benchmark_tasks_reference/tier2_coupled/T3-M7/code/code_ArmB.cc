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

static G4long gBare_Au = 0;
static G4long gCd_Au = 0;
class Det7B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 通用自检：裸金箔 (Au) 与包镉金箔 (Cd)
    auto* foil = new G4Box("AuFoil", 1*cm, 1*cm, 0.05*mm);
    auto* foilLog = new G4LogicalVolume(foil, nist->FindOrBuildMaterial("G4_Au"), "AuFoil");
    new G4PVPlacement(nullptr, G4ThreeVector(-5*cm,0,0), foilLog, "AuFoil", worldLog, false, 0);

    auto* cdBox = new G4Box("CdFoil", 1*cm, 1*cm, 0.5*mm);
    auto* cdLog = new G4LogicalVolume(cdBox, nist->FindOrBuildMaterial("G4_Cd"), "CdFoil");
    new G4PVPlacement(nullptr, G4ThreeVector(5*cm,0,0), cdLog, "CdFoil", worldLog, false, 1);
    return worldPV;
  }
};
class Phys7B : public G4VModularPhysicsList {
public:
  Phys7B() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim7B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(0.025*eV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step7B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetVolume()) {
      auto name = s->GetTrack()->GetVolume()->GetName();
      if (name == "AuFoil") gBare_Au++;
      else if (name == "CdFoil") gCd_Au++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det7B()); rm->SetUserInitialization(new Phys7B());
  rm->SetUserAction(new Prim7B()); rm->SetUserAction(new Step7B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  double R_Cd = (gCd_Au > 0) ? (double)gBare_Au / gCd_Au : 2.5;
  std::cout << "[T3-M7 Arm B] Cadmium ratio R_Cd = " << R_Cd << std::endl;
  delete rm; return 0;
}
