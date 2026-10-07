/*
```guardrail-intent
{
  "F1a": {
    "v": "dose",
    "src": "U"
  },
  "F1b": {
    "v": "人体模型软组织/骨/肺各器官等效剂量",
    "src": "U"
  },
  "F2": {
    "v": "equivalent",
    "src": "U"
  },
  "F3": {
    "v": "Sv",
    "src": "A"
  },
  "F4": {
    "v": "volume_avg",
    "src": "U"
  },
  "F5": {
    "v": "steady",
    "src": "A"
  },
  "F6": {
    "v": "per_source",
    "src": "U"
  },
  "F7": {
    "v": "trend",
    "src": "A"
  },
  "F8": {
    "v": "other_mc",
    "src": "A"
  },
  "F9": {
    "v": "N/A",
    "src": "U"
  },
  "F10": {
    "v": "scalar",
    "src": "U"
  },
  "warnings": [
    "no_sievert",
    "per_source_needs_strength"
  ]
}
```
*/

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

static G4double g_ICRP103_dose = 0;
class Det7C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* phantomSolid = new G4Box("Phantom", 20*cm, 20*cm, 10*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "Phantom");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,10*cm), phantomLog, "Phantom", worldLog, false, 0);
    return worldPV;
  }
};
class Phys7C : public G4VModularPhysicsList {
public:
  Phys7C() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim7C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(1.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-10*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step7C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    G4double E = s->GetTrack()->GetKineticEnergy() / MeV;
    G4double w_R = 2.5 + 17.5 * std::exp(-std::pow(std::log(2.0*E + 1e-4), 2.0) / 6.0);
    if (w_R > 20.0) w_R = 20.0;
    g_ICRP103_dose += s->GetTotalEnergyDeposit() * w_R;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det7C()); rm->SetUserInitialization(new Phys7C());
  rm->SetUserAction(new Prim7C()); rm->SetUserAction(new Step7C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M7 Arm C] ICRP 103 Dose = " << g_ICRP103_dose << std::endl;
  delete rm; return 0;
}
