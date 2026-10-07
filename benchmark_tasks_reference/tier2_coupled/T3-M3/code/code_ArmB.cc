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

static G4long gPipeCounts = 0;
class Det3B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 20*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* pipeSolid = new G4Tubs("Pipe", 0, 10*cm, 7.5*m, 0, 360*deg);
    auto* pipeLog = new G4LogicalVolume(pipeSolid, nist->FindOrBuildMaterial("G4_WATER"), "Pipe");
    new G4PVPlacement(nullptr, {}, pipeLog, "Pipe", worldLog, false, 0);
    return worldPV;
  }
};
class Phys3B : public G4VModularPhysicsList {
public:
  Phys3B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim3B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.13*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step3B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 5*m) gPipeCounts++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det3B()); rm->SetUserInitialization(new Phys3B());
  rm->SetUserAction(new Prim3B()); rm->SetUserAction(new Step3B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // 盲区：通用自检建立了管道几何，但假定稳态瞬时到达，缺少流动延迟时间计算
  double A0 = 1000.0;
  double half_life = 7.13;
  double lambda = std::log(2.0) / half_life;
  (void)lambda;
  std::cout << "[T3-M3 Arm B] Activity steady state = " << A0 << std::endl;
  delete rm; return 0;
}
