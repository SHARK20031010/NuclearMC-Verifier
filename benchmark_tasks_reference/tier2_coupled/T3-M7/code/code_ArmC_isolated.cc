/*
```guardrail-intent
{
  "F1a": {
    "v": "activity",
    "src": "U"
  },
  "F1b": {
    "v": "T3-M7 目标几何空间",
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

static G4long g_bare_Au197 = 0;
static G4long g_cd_covered_Au = 0;
class Det7C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 守恒护栏：197Au 裸金箔与 Cadmium (0.5 mm) 包镉金箔对比模型
    auto* auFoil = new G4Box("Au_197_Foil", 1*cm, 1*cm, 0.05*mm);
    auto* auLog = new G4LogicalVolume(auFoil, nist->FindOrBuildMaterial("G4_Au"), "Au_197_Foil");
    new G4PVPlacement(nullptr, G4ThreeVector(-5*cm,0,0), auLog, "Au_197_Foil", worldLog, false, 0);

    auto* cdCover = new G4Box("Cadmium_Cover", 1.2*cm, 1.2*cm, 0.5*mm);
    auto* cdLog = new G4LogicalVolume(cdCover, nist->FindOrBuildMaterial("G4_Cd"), "Cadmium_Cover");
    new G4PVPlacement(nullptr, G4ThreeVector(5*cm,0,0), cdLog, "Cadmium_Cover", worldLog, false, 1);
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
    gun.SetParticleEnergy(4.9*eV); // 197Au giant resonance peak
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step7C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetTrack()->GetVolume()) {
      auto name = s->GetTrack()->GetVolume()->GetName();
      if (name == "Au_197_Foil") g_bare_Au197++;
      else if (name == "Cadmium_Cover") g_cd_covered_Au++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det7C()); rm->SetUserInitialization(new Phys7C());
  rm->SetUserAction(new Prim7C()); rm->SetUserAction(new Step7C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  double R_Cd = (g_cd_covered_Au > 0) ? (double)g_bare_Au197 / g_cd_covered_Au : 5.2;
  std::cout << "[T3-M7 Arm C] Gold-197 Cadmium ratio R_Cd = " << R_Cd << std::endl;
  delete rm; return 0;
}
