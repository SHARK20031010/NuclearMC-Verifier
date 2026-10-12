// WILD-02 Fixed Code - Arm C Pure First Principles
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
#include "QGSP_BIC_HP.hh"
#include <iostream>

static G4double gTofSum = 0.0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 1*m, 1*m, 6*m), nist->FindOrBuildMaterial("G4_AIR"), "W");
    auto* det = new G4LogicalVolume(new G4Box("Det", 20*cm, 20*cm, 5*cm), nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE"), "Det");
    new G4PVPlacement(nullptr, {0,0,5*m}, det, "Det", world, false, 0);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(14.0 * MeV);
    gun.SetParticlePosition({0,0,0});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    // Principle 2: Causality and Absolute Clock Monotonicity.
    // Time-of-flight must use GetGlobalTime() relative to global event origin, not GetLocalTime()
    // which resets to 0 for newly created secondary tracks or resets per track lifecycle.
    // In addition, TOF must be recorded when the incident particle crosses the boundary into the detector.
    auto* track = step->GetTrack();
    auto* pre = step->GetPreStepPoint();
    auto* pv = pre->GetPhysicalVolume();
    if (track->GetTrackID() == 1 && pre->GetStepStatus() == fGeomBoundary && pv && pv->GetName() == "Det") {
      G4double tof = pre->GetGlobalTime();
      gTofSum += tof;
    }
  }
};

int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new QGSP_BIC_HP());
  rm->SetUserAction(new Prim());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 20);
  std::cout << "WILD-02 TOF Sum = " << gTofSum / ns << " ns" << std::endl;
  delete rm; 
  std::_Exit(0);
}
