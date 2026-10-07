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

static G4long g_det_stray_45 = 0;
static G4long g_det_stray_90 = 0;
class Det10C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 3*m, 3*m, 3*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* phantomSolid = new G4Box("WaterPhantom", 15*cm, 15*cm, 20*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterPhantom");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,20*cm), phantomLog, "WaterPhantom", worldLog, false, 0);

    // 守恒护栏：在靶点 50 cm 处设置侧向 45 度与 90 度杂散探测器 (det)
    auto* detSolid = new G4Box("StrayDetector", 5*cm, 5*cm, 5*cm);
    auto* detLog = new G4LogicalVolume(detSolid, nist->FindOrBuildMaterial("G4_AIR"), "StrayDetector");
    new G4PVPlacement(nullptr, G4ThreeVector(50*cm, 0, 20*cm), detLog, "det_90", worldLog, false, 1);
    new G4PVPlacement(nullptr, G4ThreeVector(35*cm, 0, 55*cm), detLog, "det_45", worldLog, false, 2);
    return worldPV;
  }
};
class Phys10C : public G4VModularPhysicsList {
public:
  Phys10C() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim10C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("GenericIon"));
    gun.SetParticleCharge(6);
    gun.SetParticleEnergy(290.0*MeV * 12);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step10C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetParticleDefinition()->GetParticleName() == "neutron") {
      if (s->GetTrack()->GetVolume()) {
        auto name = s->GetTrack()->GetVolume()->GetName();
        if (name == "det_90") g_det_stray_90++;
        else if (name == "det_45") g_det_stray_45++;
      }
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det10C()); rm->SetUserInitialization(new Phys10C());
  rm->SetUserAction(new Prim10C()); rm->SetUserAction(new Step10C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M10 Arm C] Stray neutron counts: 45deg = " << g_det_stray_45 << ", 90deg = " << g_det_stray_90 << std::endl;
  delete rm; return 0;
}
