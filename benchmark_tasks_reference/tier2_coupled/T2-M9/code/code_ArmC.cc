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

static G4double g_r_profile[10] = {0};
class Det9C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_WATER"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 守恒护栏：192Ir 源芯与不锈钢包裹外壳，置于无限大水介质中
    auto* seedSolid = new G4Tubs("Ir192_Core", 0, 0.3*mm, 1.5*mm, 0, 360*deg);
    auto* seedLog = new G4LogicalVolume(seedSolid, nist->FindOrBuildMaterial("G4_Ir"), "Ir192_Core");
    new G4PVPlacement(nullptr, {}, seedLog, "Ir192_Core", worldLog, false, 0);
    return worldPV;
  }
};
class Phys9C : public G4VModularPhysicsList {
public:
  Phys9C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim9C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(0.38*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step9C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    G4double radius = pos.mag();
    // 依据 TG43 规范，在 0.5 cm 到 5 cm 范围分箱计算径向剂量函数 g_r
    if (radius >= 0.5*cm && radius <= 5.0*cm) {
      int bin = (radius - 0.5*cm) / (0.5*cm);
      if (bin >= 0 && bin < 10) {
        G4double G_r = 1.0 / (radius * radius);
        g_r_profile[bin] += s->GetTotalEnergyDeposit() / G_r;
      }
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det9C()); rm->SetUserInitialization(new Phys9C());
  rm->SetUserAction(new Prim9C()); rm->SetUserAction(new Step9C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M9 Arm C] TG-43 g_r at 1cm = " << g_r_profile[1] << std::endl;
  delete rm; return 0;
}
