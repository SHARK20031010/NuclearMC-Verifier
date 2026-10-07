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

static G4double gD2O_Edep = 0;
static G4double gH2O_Edep = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 40*cm, 40*cm, 40*cm), nist->FindOrBuildMaterial("G4_AIR"), "W");
    
    auto* elD = new G4Element("Deuterium", "D", 1., 2.014*g/mole);
    auto* elO = nist->FindOrBuildElement("O");
    auto* d2oMat = new G4Material("D2O", 1.11*g/cm3, 2);
    d2oMat->AddElement(elD, 2);
    d2oMat->AddElement(elO, 1);

    auto* d2oTank = new G4LogicalVolume(new G4Box("D2O_Tank", 10*cm, 10*cm, 10*cm), d2oMat, "D2O_Tank");
    auto* h2oTank = new G4LogicalVolume(new G4Box("H2O_Tank", 10*cm, 10*cm, 10*cm), nist->FindOrBuildMaterial("G4_WATER"), "H2O_Tank");
    new G4PVPlacement(nullptr, {-12*cm,0,0}, d2oTank, "D2O_Tank", world, false, 0);
    new G4PVPlacement(nullptr, { 12*cm,0,0}, h2oTank, "H2O_Tank", world, false, 1);
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(0.025 * eV);
    gun.SetParticlePosition({0,0,-15*cm});
    gun.SetParticleMomentumDirection({0,0,1});
    gun.GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    auto* pv = s->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv) return;
    if (pv->GetName() == "D2O_Tank") gD2O_Edep += s->GetTotalEnergyDeposit();
    if (pv->GetName() == "H2O_Tank") gH2O_Edep += s->GetTotalEnergyDeposit();
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
  std::cout << "T5-M10 D2O Edep = " << gD2O_Edep/keV << " keV, H2O Edep = " << gH2O_Edep/keV << " keV" << std::endl;
  std::_Exit(0);
}
