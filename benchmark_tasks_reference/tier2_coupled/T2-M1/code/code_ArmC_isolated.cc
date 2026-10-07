/*
```guardrail-intent
{
  "F1a": {
    "v": "dose",
    "src": "U"
  },
  "F1b": {
    "v": "水箱沿轴百分深度剂量与横向 Profile",
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
    "v": "distribution",
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
    "v": "curve",
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

static G4double gPDD[30] = {0};
static G4double gProfile[30] = {0};
class Det1C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 倒圆锥均整板 G4Cons
    auto* coneSolid = new G4Cons("ConicalFilter", 0, 0, 0, 5*cm, 2*cm, 0, 360*deg);
    auto* coneLog = new G4LogicalVolume(coneSolid, nist->FindOrBuildMaterial("G4_W"), "ConicalFilter");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,-15*cm), coneLog, "ConicalFilter", worldLog, false, 0);

    auto* phantomSolid = new G4Box("WaterTank", 15*cm, 15*cm, 15*cm);
    auto* phantomLog = new G4LogicalVolume(phantomSolid, nist->FindOrBuildMaterial("G4_WATER"), "WaterTank");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,15*cm), phantomLog, "WaterTank", worldLog, false, 1);
    return worldPV;
  }
};
class Phys1C : public G4VModularPhysicsList {
public:
  Phys1C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim1C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-25*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step1C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto pos = s->GetPostStepPoint()->GetPosition();
    G4double edep = s->GetTotalEnergyDeposit();
    // 守恒护栏：同时完成沿轴 PDD (zBin) 与横向离轴 Profile (xBin) 平坦度 (flatness) 评估
    if (pos.z() >= 0 && pos.z() <= 30*cm) {
      int zBin = pos.z() / (1.0*cm);
      if (zBin >= 0 && zBin < 30) gPDD[zBin] += edep;
    }
    if (pos.z() >= 9.5*cm && pos.z() <= 10.5*cm) {
      int xBin = (pos.x() + 15*cm) / (1.0*cm);
      if (xBin >= 0 && xBin < 30) gProfile[xBin] += edep; // beam flatness profile
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det1C()); rm->SetUserInitialization(new Phys1C());
  rm->SetUserAction(new Prim1C()); rm->SetUserAction(new Step1C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M1 Arm C] PDD[10] = " << gPDD[10] << ", Profile[15] = " << gProfile[15] << std::endl;
  delete rm; return 0;
}
