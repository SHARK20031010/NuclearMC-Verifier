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

static G4long g_thermal_absorbed_Cd = 0;
class Det7C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* waxSolid = new G4Sphere("Paraffin_Mod", 0, 15*cm, 0, 360*deg, 0, 180*deg);
    auto* waxLog = new G4LogicalVolume(waxSolid, nist->FindOrBuildMaterial("G4_PARAFFIN"), "Paraffin_Mod");
    new G4PVPlacement(nullptr, {}, waxLog, "Paraffin_Mod", worldLog, false, 0);

    auto* cdSolid = new G4Sphere("Cadmium_Layer", 15*cm, 15.1*cm, 0, 360*deg, 0, 180*deg);
    auto* cdLog = new G4LogicalVolume(cdSolid, nist->FindOrBuildMaterial("G4_Cd"), "Cadmium_Layer");
    new G4PVPlacement(nullptr, {}, cdLog, "Cadmium_Layer", worldLog, false, 1);
    return worldPV;
  }
};
class Phys7C : public G4VModularPhysicsList {
public:
  Phys7C() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim7C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(2.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step7C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetVolume() && s->GetTrack()->GetVolume()->GetName() == "Cadmium_Layer") {
      if (s->GetTrack()->GetKineticEnergy() < 0.5*eV) g_thermal_absorbed_Cd++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det7C()); rm->SetUserInitialization(new Phys7C());
  rm->SetUserAction(new Prim7C()); rm->SetUserAction(new Step7C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M7 Arm C] Absorbed in Cadmium = " << g_thermal_absorbed_Cd << std::endl;
  delete rm; return 0;
}
