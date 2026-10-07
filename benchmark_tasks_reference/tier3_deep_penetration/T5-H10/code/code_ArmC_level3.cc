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
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4OpticalPhysics.hh"
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
// 组别: Arm C (Level 3 第一性原理修复)
// ============================================================================

static G4double gCoulombExplosionEnergy = 0.0;
static G4long gCoulombClusterEvents = 0;

class T5H10Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* water = nist->FindOrBuildMaterial("G4_WATER");

    auto* worldSolid = new G4Box("WorldBox", 50.0 * nm, 50.0 * nm, 50.0 * nm);
    auto* worldLog = new G4LogicalVolume(worldSolid, vacuum, "WorldLog");
    auto* worldPhys = new G4PVPlacement(nullptr, {}, worldLog, "WorldPhys", nullptr, false, 0);

    // 纳米团簇 (Cluster, R = 5 nm)
    auto* clusSolid = new G4Sphere("ClusSolid", 0.0 * nm, 5.0 * nm, 0, 360*deg, 0, 180*deg);
    auto* clusLog = new G4LogicalVolume(clusSolid, water, "ClusLog");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,0), clusLog, "ClusterPhys", worldLog, false, 1);

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
    if (!pre || pre->GetName() != "ClusterPhys") return;

    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 10.0 * eV) {
      // 重离子诱发纳米团簇库仑爆炸 (Coulomb explosion)
      // 超高电离产生瞬态高密度正电荷中心 (charge) 与静电排斥势能 (potential)
      G4double W_ion = 25.0 * eV;
      G4double net_charge_Q = (edep / W_ion) * 1.6e-19; // 正电荷总量
      G4double R_cluster = 5.0 * nm;

      // 静电势能: U = (3/5) * Q^2 / (4*pi*epsilon_0*R)
      G4double eps0 = 8.854e-12;
      G4double coulomb_potential_energy_J = (0.6 * net_charge_Q * net_charge_Q) / (4.0 * M_PI * eps0 * (R_cluster / m));
      G4double coulomb_energy_eV = coulomb_potential_energy_J / 1.6e-19;

      gCoulombExplosionEnergy += coulomb_energy_eV * eV;
      gCoulombClusterEvents++;
    }
  }
};

int main(int argc, char** argv) {
  auto* runManager = new G4RunManager();
  runManager->SetUserInitialization(new T5H10Detector());
  runManager->SetUserInitialization(new T5H10Physics());
  runManager->SetUserAction(new T5H10Generator());
  runManager->SetUserAction(new T5H10SteppingAction());
  runManager->Initialize();

  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;
  runManager->BeamOn(nEvents);

  std::cout << "[T5-H10] Coulomb Explosion Energy: " << gCoulombExplosionEnergy / keV 
            << " keV" << std::endl;

  delete runManager;
  std::_Exit(0);
}
