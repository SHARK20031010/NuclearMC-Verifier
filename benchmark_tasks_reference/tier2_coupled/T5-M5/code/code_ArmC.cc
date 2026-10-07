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
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

static G4double gYstarSum = 0;
static G4double gTotalDose = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 10*um, 10*um, 10*um), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* sphere = new G4LogicalVolume(new G4Sphere("Cell", 0, 1*um, 0, 360*deg, 0, 180*deg), nist->FindOrBuildMaterial("G4_WATER"), "Cell");
    new G4PVPlacement(nullptr, {0,0,0}, sphere, "Cell", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    gun.SetParticleEnergy(1 * MeV);
    gun.SetParticlePosition({0,0,-2*um});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    G4double edep = s->GetTotalEnergyDeposit();
    if (edep > 0) {
      // 护栏修复: 引入 Kellerer-Rossi 饱和线能 y0 = 125 keV/um 进行过杀饱和修正 (saturation y*)
      G4double l_bar = 1.333 * um;
      G4double y = edep / l_bar;
      G4double y0 = 125.0 * keV / um;
      G4double ystar = (y0 * y0 / y) * (1.0 - std::exp(-std::pow(y / y0, 2)));
      gYstarSum += ystar * edep;
      gTotalDose += edep;
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "T5-M5 Saturation Corrected y* = " << (gTotalDose > 0 ? gYstarSum/gTotalDose : 0)/(keV/um) << " keV/um" << std::endl;
  delete rm; return 0;
}
