// WILD-04 Fixed Code
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "FTFP_BERT_HP.hh"
#include <iostream>

static G4double gFlux = 0.0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 2*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    for (int i=0; i<6; ++i) {
      char name[16]; std::snprintf(name, sizeof(name), "layer_%d", i);
      auto* layer = new G4LogicalVolume(new G4Box(name, 50*cm, 50*cm, 5*cm), nist->FindOrBuildMaterial("G4_Fe"), name);
      new G4PVPlacement(nullptr, {0,0,(5+i*10)*cm}, layer, name, world, false, i);
    }
    auto* det = new G4LogicalVolume(new G4Box("Det", 50*cm, 50*cm, 5*cm), nist->FindOrBuildMaterial("G4_AIR"), "Det");
    new G4PVPlacement(nullptr, {0,0,70*cm}, det, "Det", world, false, 99);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(14.0 * MeV);
    gun.SetParticlePosition({0,0,-5*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    // 修复：严格乘入当前粒子统计权重 GetWeight()
    if (pv && pv->GetName() == "Det") {
      gFlux += s->GetTrack()->GetWeight(); 
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new FTFP_BERT_HP());
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 20);
  std::cout << "WILD-04 Weighted Flux = " << gFlux << std::endl;
  delete rm; return 0;
}
