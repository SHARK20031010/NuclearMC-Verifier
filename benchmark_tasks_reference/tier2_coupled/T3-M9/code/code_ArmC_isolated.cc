/*
```guardrail-intent
{
  "F1a": {
    "v": "activity",
    "src": "U"
  },
  "F1b": {
    "v": "T3-M9 目标几何空间",
    "src": "U"
  },
  "F2": {
    "v": "atoms",
    "src": "U"
  },
  "F3": {
    "v": "other",
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

static G4long g_ic_electrons = 0;
static G4long g_kalpha_xrays = 0;
class Det9C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    return new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);
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
    // 守恒护栏：Tc-99m (140.5 keV) 内转换 (IC) 剥离 K 层电子 (conversion electron) 并伴随特征 K-alpha X 射线
    double r = G4UniformRand();
    if (r < 0.10) {
      // Internal conversion electron
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
      gun.SetParticleEnergy(119.5*keV);
      gun.GeneratePrimaryVertex(ev);
      // Characteristic K-alpha X-ray
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
      gun.SetParticleEnergy(18.3*keV);
      gun.GeneratePrimaryVertex(ev);
    } else {
      gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
      gun.SetParticleEnergy(140.5*keV);
      gun.GeneratePrimaryVertex(ev);
    }
  }
};
class Step9C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto name = s->GetTrack()->GetParticleDefinition()->GetParticleName();
    if (name == "e-") g_ic_electrons++;
    else if (name == "gamma" && s->GetTrack()->GetKineticEnergy() < 25*keV) g_kalpha_xrays++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det9C()); rm->SetUserInitialization(new Phys9C());
  rm->SetUserAction(new Prim9C()); rm->SetUserAction(new Step9C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M9 Arm C] IC conversion electron = " << g_ic_electrons << ", K-Xray = " << g_kalpha_xrays << std::endl;
  delete rm; return 0;
}
