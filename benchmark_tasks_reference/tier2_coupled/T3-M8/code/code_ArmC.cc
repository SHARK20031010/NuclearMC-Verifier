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

static G4long g_dpa_cascade_displacements = 0;
class Det8C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 硅酸盐玻璃固化体
    auto* glass = new G4Box("BorosilicateGlass", 10*cm, 10*cm, 10*cm);
    auto* glassLog = new G4LogicalVolume(glass, nist->FindOrBuildMaterial("G4_Pyrex_Glass"), "BorosilicateGlass");
    new G4PVPlacement(nullptr, {}, glassLog, "BorosilicateGlass", worldLog, false, 0);
    return worldPV;
  }
};
class Phys8C : public G4VModularPhysicsList {
public:
  Phys8C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim8C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    // 守恒护栏：Am-241 alpha 衰变产生约 100 keV 重反冲核 (recoil) 造成晶格位移损伤 (DPA damage)
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("GenericIon"));
    gun.SetParticleCharge(93); // Np-237 recoil nucleus
    gun.SetParticleEnergy(93.0*keV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    G4double edep = s->GetTotalEnergyDeposit();
    // NRT 模型初级离位原子数计算 (DPA damage)
    G4long dpa = (G4long)(0.8 * edep / (2.0 * 28.0*eV));
    g_dpa_cascade_displacements += dpa;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8C()); rm->SetUserInitialization(new Phys8C());
  rm->SetUserAction(new Prim8C()); rm->SetUserAction(new Step8C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M8 Arm C] recoil alpha damage cascade DPA = " << g_dpa_cascade_displacements << std::endl;
  delete rm; return 0;
}
