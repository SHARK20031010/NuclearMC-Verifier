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

static G4double g_FLASH_dose_Gy = 0;
class Det3C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* phantomSolid = new G4Box("WaterBox", 10*cm, 10*cm, 10*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterBox");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,10*cm), phantomLog, "WaterBox", worldLog, false, 0);
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
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(10.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step3C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    g_FLASH_dose_Gy += (s->GetTotalEnergyDeposit() / MeV) * 1.6e-13;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det3C()); rm->SetUserInitialization(new Phys3C());
  rm->SetUserAction(new Prim3C()); rm->SetUserAction(new Step3C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  // 守恒护栏：除以单脉冲持续时间 tau = 2.0*us，换算超高瞬时峰值吸收剂量率 (doseRate, Gy/s)
  G4double tau = 2.0 * us;
  G4double doseRate = g_FLASH_dose_Gy / tau;
  std::cout << "[T2-M3 Arm C] Instantaneous peak doseRate = " << doseRate << " Gy/s" << std::endl;
  delete rm; return 0;
}
