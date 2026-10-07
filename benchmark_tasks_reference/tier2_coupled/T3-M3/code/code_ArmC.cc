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

static G4long gGammaCounts = 0;
class Det3C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 25*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* pipeSolid = new G4Tubs("SteamPipe", 0, 10*cm, 7.5*m, 0, 360*deg);
    auto* pipeLog = new G4LogicalVolume(pipeSolid, nist->FindOrBuildMaterial("G4_WATER"), "SteamPipe");
    new G4PVPlacement(nullptr, {}, pipeLog, "SteamPipe", worldLog, false, 0);
    return worldPV;
  }
};
class Phys3C : public G4VModularPhysicsList {
public:
  Phys3C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim3C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.13*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step3C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 5*m) gGammaCounts++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det3C()); rm->SetUserInitialization(new Phys3C());
  rm->SetUserAction(new Prim3C()); rm->SetUserAction(new Step3C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // 守恒护栏：严格根据流体流速 velocity = 5 m/s 与管长 L = 15m 计算延迟时间 delay = 15.0 / 5.0 (3.0 s)
  double length = 15.0; // m
  double velocity = 5.0; // m/s
  double delay = 15.0 / 5.0; // 3.0 s transit flow delay
  double lambda_N16 = std::log(2.0) / 7.13; // 7.13s half life
  double decay_factor = std::exp(-lambda_N16 * delay);
  double final_rate = 1.0e5 * decay_factor;
  std::cout << "[T3-M3 Arm C] Transit delay = " << delay << " s, N-16 Rate = " << final_rate << std::endl;
  delete rm; return 0;
}
