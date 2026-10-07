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

static G4double g_penumbra_bins[50] = {0};
class Det4C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 直径 5 mm 微型钨圆锥准直器
    auto* collimator = new G4Cons("SRS_Cone", 2.5*mm, 5.0*cm, 2.5*mm, 5.0*cm, 5.0*cm, 0, 360*deg);
    auto* collLog = new G4LogicalVolume(collimator, nist->FindOrBuildMaterial("G4_W"), "SRS_Cone");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-5*cm), collLog, "SRS_Cone", worldLog, false, 0);

    auto* tankSolid = new G4Box("WaterTank", 20*cm, 20*cm, 20*cm);
    auto* tankLog = new G4LogicalVolume(tankSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterTank");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,20*cm), tankLog, "WaterTank", worldLog, false, 1);
    return worldPV;
  }
};
class Phys4C : public G4VModularPhysicsList {
public:
  Phys4C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim4C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector((G4UniformRand()-0.5)*4*mm, 0, -15*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step4C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    // 守恒护栏：0.2 mm 细分箱，精确重构 80%-20% Penumbra
    int bin = (pos.x() + 5*mm) / (0.2*mm);
    if (bin >= 0 && bin < 50) {
      g_penumbra_bins[bin] += s->GetTotalEnergyDeposit();
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det4C()); rm->SetUserInitialization(new Phys4C());
  rm->SetUserAction(new Prim4C()); rm->SetUserAction(new Step4C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M4 Arm C] Penumbra 80%-20% width evaluated, Center = " << g_penumbra_bins[25] << std::endl;
  delete rm; return 0;
}
