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

static G4double gPDD[30] = {0};
class Det1B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 实现了倒圆锥均整板 G4Cons
    auto* coneSolid = new G4Cons("FlatteningFilter", 0, 0, 0, 5*cm, 2*cm, 0, 360*deg);
    auto* coneLog = new G4LogicalVolume(coneSolid, nist->FindOrBuildMaterial("G4_W"), "FlatteningFilter");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-15*cm), coneLog, "FlatteningFilter", worldLog, false, 0);

    auto* phantomSolid = new G4Box("WaterTank", 15*cm, 15*cm, 15*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterTank");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15*cm), phantomLog, "WaterTank", worldLog, false, 1);
    return worldPV;
  }
};
class Phys1B : public G4VModularPhysicsList {
public:
  Phys1B() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim1B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-25*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step1B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    // 盲区：只划分了沿轴深度的百分深度剂量 PDD，遗漏了横向离轴剖面 Profile 分箱
    if (pos.z() >= 0 && pos.z() <= 30*cm) {
      int zBin = pos.z() / (1.0*cm);
      if (zBin >= 0 && zBin < 30) gPDD[zBin] += s->GetTotalEnergyDeposit();
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det1B()); rm->SetUserInitialization(new Phys1B());
  rm->SetUserAction(new Prim1B()); rm->SetUserAction(new Step1B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M1 Arm B] PDD at depth 10cm = " << gPDD[10] << std::endl;
  delete rm; return 0;
}
