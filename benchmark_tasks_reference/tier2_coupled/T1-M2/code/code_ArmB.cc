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

static G4double gTotalDose = 0;
class Det2B : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* feSolid = new G4Tubs("Fe_Tube", 0, 5*cm, 50*cm, 0, 360*deg);
    auto* feLog = new G4LogicalVolume(feSolid, nist->FindOrBuildMaterial("G4_Fe"), "Fe_Tube");
    new G4PVPlacement(nullptr, {}, feLog, "Fe_Tube", worldLog, false, 0);

    auto* peSolid = new G4Tubs("PE_Tube", 5*cm, 20*cm, 50*cm, 0, 360*deg);
    auto* peLog = new G4LogicalVolume(peSolid, nist->FindOrBuildMaterial("G4_POLYETHYLENE"), "PE_Tube");
    new G4PVPlacement(nullptr, {}, peLog, "PE_Tube", worldLog, false, 1);

    auto* pbSolid = new G4Tubs("Pb_Tube", 20*cm, 25*cm, 50*cm, 0, 360*deg);
    auto* pbLog = new G4LogicalVolume(pbSolid, nist->FindOrBuildMaterial("G4_Pb"), "Pb_Tube");
    new G4PVPlacement(nullptr, {}, pbLog, "Pb_Tube", worldLog, false, 2);
    return worldPV;
  }
};
class Phys2B : public G4VModularPhysicsList {
public:
  Phys2B() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim2B : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(2112));
    gun.SetParticleEnergy(2.1*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4ThreeVector(1,0,0));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step2B : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    // 盲区：只粗放累加总能量沉积，未解耦次级伽马与中子穿透贡献
    gTotalDose += s->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det2B()); rm->SetUserInitialization(new Phys2B());
  rm->SetUserAction(new Prim2B()); rm->SetUserAction(new Step2B());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M2 Arm B] Total Dose = " << gTotalDose << std::endl;
  delete rm; return 0;
}
