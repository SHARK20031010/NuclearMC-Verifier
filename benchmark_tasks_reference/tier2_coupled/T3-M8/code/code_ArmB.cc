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

static G4long gRecoilDPA = 0;
class Det8B : public G4VUserDetectorConstruction {
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
class Phys8B : public G4VModularPhysicsList {
public:
  Phys8B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim8B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(5.5*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    // 通用自检：重核 alpha 反冲核 (recoil) 位移损伤 (dpa / damage)
    G4double edep = s->GetTotalEnergyDeposit();
    if (edep > 10*keV) gRecoilDPA += (G4long)(edep / (25.0*eV)); // Kinchin-Pease damage
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8B()); rm->SetUserInitialization(new Phys8B());
  rm->SetUserAction(new Prim8B()); rm->SetUserAction(new Step8B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M8 Arm B] recoil displacement damage dpa = " << gRecoilDPA << std::endl;
  delete rm; return 0;
}
