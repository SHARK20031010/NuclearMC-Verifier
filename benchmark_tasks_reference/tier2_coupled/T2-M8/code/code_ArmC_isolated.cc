/*
```guardrail-intent
{
  "F1a": {
    "v": "dose",
    "src": "U"
  },
  "F1b": {
    "v": "微米焦点钼靶出射特征谱与表面吸收剂量",
    "src": "U"
  },
  "F2": {
    "v": "absorbed",
    "src": "U"
  },
  "F3": {
    "v": "Gy",
    "src": "A"
  },
  "F4": {
    "v": "surface_avg",
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

static G4long g_mo_kalpha_counts = 0;
class Det8C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 守恒护栏：微米焦点钼靶 (Mo) + 0.8 mm 铍窗 (Be) + 0.03 mm 钼滤过板 (Molybdenum Filter)
    auto* tgt = new G4Box("Mo_Anode", 5*mm, 5*mm, 0.5*mm);
    auto* tgtLog = new G4LogicalVolume(tgt, nist->FindOrBuildMaterial("G4_Mo"), "Mo_Anode");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), tgtLog, "Mo_Anode", worldLog, false, 0);

    auto* win = new G4Box("Be_Window", 1*cm, 1*cm, 0.4*mm);
    auto* winLog = new G4LogicalVolume(win, nist->FindOrBuildMaterial("G4_Be"), "Be_Window");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,2*cm), winLog, "Be_Window", worldLog, false, 1);

    auto* flt = new G4Box("Mo_Filter", 1*cm, 1*cm, 0.015*mm);
    auto* fltLog = new G4LogicalVolume(flt, nist->FindOrBuildMaterial("G4_Mo"), "Mo_Filter");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,2.5*cm), fltLog, "Mo_Filter", worldLog, false, 2);
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
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(40.0*keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-3*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step8C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 3.0*cm && s->GetPreStepPoint()->GetPosition().z() <= 3.0*cm) {
      G4double e_keV = s->GetTrack()->GetKineticEnergy() / keV;
      if (e_keV >= 17.0 && e_keV <= 20.0) g_mo_kalpha_counts++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det8C()); rm->SetUserInitialization(new Phys8C());
  rm->SetUserAction(new Prim8C()); rm->SetUserAction(new Step8C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M8 Arm C] Characteristic Mo K-alpha counts = " << g_mo_kalpha_counts << std::endl;
  delete rm; return 0;
}
