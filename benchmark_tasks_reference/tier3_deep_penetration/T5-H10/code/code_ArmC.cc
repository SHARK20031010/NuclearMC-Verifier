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
#include "G4UserRunAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// ============================================================================
// 任务 T5-H10: 纳米团簇瞬态静电能量转移模拟
// 组别: Arm C
// ============================================================================

static G4double gCoulombPotentialEnergy = 0.0;
static G4long gExplosionEvents = 0;

class T5H10Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 100.0 * nm, 100.0 * nm, 100.0 * nm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 5 nm 纳米水团簇
    auto* clusSolid = new G4Sphere("ClusSolid", 0.0 * nm, 5.0 * nm, 0, 360*deg, 0, 180*deg);
    auto* clusLog = new G4LogicalVolume(clusSolid, water, "ClusLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), clusLog, "ClusPhys", worldLog, false, 1);

    return worldPhys;
  }
};

class T5H10Physics : public G4VModularPhysicsList {
public:
  T5H10Physics() {
    RegisterPhysics(new G4EmStandardPhysics());
  }
};

class T5H10Generator : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* anEvent) override {
    // 高电荷态重离子模拟
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("alpha"));
    gun.SetParticleEnergy(40.0 * MeV);
    gun.SetParticlePosition(G4ThreeVector(0, 0, -20.0 * nm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    gun.GeneratePrimaryVertex(anEvent);
  }
};

class T5H10SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* aStep) override {
    auto* pre = aStep->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetName() != "ClusPhys") return;

    // Arm C: 重离子高电荷剥离下纳米团簇库仑爆炸 (Coulomb explosion) 正电荷中心瞬态势能 (potential charge)
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 50.0 * eV) {
      G4int stripped_electrons = std::min(15, (int)(edep / (30.0 * eV)));
      G4double e_charge = 1.6e-19; // 库仑
      G4double eps0 = 8.85e-12;
      G4double cluster_radius = 5.0 * nm;
      // 库仑爆炸静电势能: U = (3/5) * Q^2 / (4*pi*eps0*R)
      G4double total_charge = stripped_electrons * e_charge;
      G4double U_coulomb = 0.6 * (total_charge * total_charge) / (4.0 * M_PI * eps0 * cluster_radius);
      gCoulombPotentialEnergy += U_coulomb / (1.6e-19 * eV);
      gExplosionEvents++;
    }

  }
};

class T5H10RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {}
  void EndOfRunAction(const G4Run*) override {
    std::cout << "[T5-H10] Energy transfer complete." << std::endl;
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H10Detector());
  runManager->SetUserInitialization(new T5H10Physics());
  runManager->SetUserAction(new T5H10Generator());
  runManager->SetUserAction(new T5H10RunAction());
  runManager->SetUserAction(new T5H10SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H10] Completed" << std::endl;

  delete runManager;
  return 0;
}
