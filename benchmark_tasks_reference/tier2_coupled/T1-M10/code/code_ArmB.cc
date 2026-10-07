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

static G4double depth_bin[10] = {0};
class Det10B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 3*m, 3*m, 4*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* soilSolid = new G4Box("SoilLayer", 1*m, 1*m, 1*m);
    auto* soilLog = new G4LogicalVolume(soilSolid, nist->FindOrBuildMaterial("G4_CONCRETE"), "SoilLayer");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,1*m), soilLog, "SoilLayer", worldLog, false, 0);
    return worldPV;
  }
};
class Phys10B : public G4VModularPhysicsList {
public:
  Phys10B() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim10B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(10.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step10B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    if (pos.z() >= 0 && pos.z() <= 2.0*m) {
      int zBin = pos.z() / (0.2*m);
      if (zBin >= 0 && zBin < 10) depth_bin[zBin] += 1.0;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det10B()); rm->SetUserInitialization(new Phys10B());
  rm->SetUserAction(new Prim10B()); rm->SetUserAction(new Step10B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M10 Arm B] Soil depth bin 0 = " << depth_bin[0] << std::endl;
  delete rm; return 0;
}
