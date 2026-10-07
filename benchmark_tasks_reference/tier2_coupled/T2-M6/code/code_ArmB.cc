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

static G4double gScleraDose = 0;
class Det6B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 通用自检：眼球巩膜 (Sclera) 与 125I 敷贴器 (Plaque)
    auto* eyeSolid = new G4Sphere("Eye_Sclera", 11*mm, 12*mm, 0, 360*deg, 0, 180*deg);
    auto* eyeLog = new G4LogicalVolume(eyeSolid, nist->FindOrBuildMaterial("G4_WATER"), "Eye_Sclera");
    new G4PVPlacement(nullptr, {}, eyeLog, "Eye_Sclera", worldLog, false, 0);

    auto* plaqueSolid = new G4Sphere("Eye_Plaque_125I", 12*mm, 13*mm, 0, 180*deg, 0, 90*deg);
    auto* plaqueLog = new G4LogicalVolume(plaqueSolid, nist->FindOrBuildMaterial("G4_Cu"), "Eye_Plaque_125I");
    new G4PVPlacement(nullptr, {}, plaqueLog, "Eye_Plaque_125I", worldLog, false, 1);
    return worldPV;
  }
};
class Phys6B : public G4VModularPhysicsList {
public:
  Phys6B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim6B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(27.0*keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,12*mm));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step6B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetVolume() && s->GetTrack()->GetVolume()->GetName() == "Eye_Sclera") {
      gScleraDose += s->GetTotalEnergyDeposit();
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det6B()); rm->SetUserInitialization(new Phys6B());
  rm->SetUserAction(new Prim6B()); rm->SetUserAction(new Step6B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M6 Arm B] Eye Sclera Plaque Dose = " << gScleraDose << std::endl;
  delete rm; return 0;
}
