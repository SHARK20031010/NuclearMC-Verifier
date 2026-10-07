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

static G4double g_sclera_dose = 0;
static G4double g_optic_nerve_dose = 0;
class Det6C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 守恒护栏：眼球模型 (Eye) 巩膜 (Sclera) 与 125I 敷贴器 (Plaque) 靶区/危及器官独立解耦
    auto* scleraSolid = new G4Sphere("Sclera_Target", 11*mm, 12*mm, 0, 360*deg, 0, 180*deg);
    auto* scleraLog = new G4LogicalVolume(scleraSolid, nist->FindOrBuildMaterial("G4_WATER"), "Sclera_Target");
    new G4PVPlacement(nullptr, {}, scleraLog, "Sclera_Target", worldLog, false, 0);

    auto* plaqueSolid = new G4Sphere("Brass_Plaque", 12*mm, 13*mm, 0, 180*deg, 0, 90*deg);
    auto* plaqueLog = new G4LogicalVolume(plaqueSolid, nist->FindOrBuildMaterial("G4_Cu"), "Brass_Plaque");
    new G4PVPlacement(nullptr, {}, plaqueLog, "Brass_Plaque", worldLog, false, 1);
    return worldPV;
  }
};
class Phys6C : public G4VModularPhysicsList {
public:
  Phys6C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim6C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(27.4*keV); // 125I principal emission
    gun.SetParticlePosition(G4ThreeVector(0,0,12.2*mm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,-1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetVolume()) {
      auto name = s->GetTrack()->GetVolume()->GetName();
      if (name == "Sclera_Target") g_sclera_dose += s->GetTotalEnergyDeposit();
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6C()); rm->SetUserInitialization(new Phys6C());
  rm->SetUserAction(new Prim6C()); rm->SetUserAction(new Step6C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M6 Arm C] 125I Plaque Sclera Dose = " << g_sclera_dose << std::endl;
  delete rm; return 0;
}
