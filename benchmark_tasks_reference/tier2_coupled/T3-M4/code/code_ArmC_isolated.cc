/*
```guardrail-intent
{
  "F1a": {
    "v": "activity",
    "src": "U"
  },
  "F1b": {
    "v": "T3-M4 目标几何空间",
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

static G4double gEventEdep = 0;
static G4long g_sum_peak_4123 = 0;
class Det4C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 1*m, 1*m, 1*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 4π 井型 NaI 探测器
    auto* naiSolid = new G4Tubs("Well_NaI", 1*cm, 5*cm, 5*cm, 0, 360*deg);
    auto* naiLog = new G4LogicalVolume(naiSolid, nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "Well_NaI");
    new G4PVPlacement(nullptr, {}, naiLog, "Well_NaI", worldLog, false, 0);
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
    // 守恒护栏：Na-24 瞬间级联发射 1.369 MeV 与 2.754 MeV 双伽马光子，形成 4.123 MeV 相加峰
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.SetParticleEnergy(1.369*MeV);
    gun.GeneratePrimaryVertex(ev);

    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.SetParticleEnergy(2.754*MeV);
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step4C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    gEventEdep += s->GetTotalEnergyDeposit();
    if (gEventEdep >= 4.0*MeV && gEventEdep <= 4.2*MeV) {
      g_sum_peak_4123++;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det4C()); rm->SetUserInitialization(new Phys4C());
  rm->SetUserAction(new Prim4C()); rm->SetUserAction(new Step4C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T3-M4 Arm C] True Coincidence Sum Peak 4.123 MeV = " << g_sum_peak_4123 << std::endl;
  delete rm; return 0;
}
