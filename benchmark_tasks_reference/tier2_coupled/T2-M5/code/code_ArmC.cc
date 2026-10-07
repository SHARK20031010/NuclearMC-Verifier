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

static G4double g_mlc_interleaf_leak = 0;
class Det5C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 守恒护栏：7 cm 钨合金 MLC 叶片 (Leaf) 圆弧端面与缝隙聚焦漏光 (leak)
    auto* wMat = nist->FindOrBuildMaterial("G4_W");
    auto* leftLeaf = new G4Box("MLC_Leaf_Left", 5*cm, 1*cm, 3.5*cm);
    auto* leftLog = new G4LogicalVolume(leftLeaf, wMat, "MLC_Leaf_Left");
    new G4PVPlacement(nullptr, G4ThreeVector(-5.05*cm, 0, -10*cm), leftLog, "MLC_Leaf_Left", worldLog, false, 0);

    auto* rightLeaf = new G4Box("MLC_Leaf_Right", 5*cm, 1*cm, 3.5*cm);
    auto* rightLog = new G4LogicalVolume(rightLeaf, wMat, "MLC_Leaf_Right");
    new G4PVPlacement(nullptr, G4ThreeVector(5.05*cm, 0, -10*cm), rightLog, "MLC_Leaf_Right", worldLog, false, 1);
    return worldPV;
  }
};
class Phys5C : public G4VModularPhysicsList {
public:
  Phys5C() { RegisterPhysics(new G4EmStandardPhysics_option4()); }
};
class Prim5C : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    gun.SetParticleEnergy(6.0*MeV);
    gun.SetParticlePosition(G4ThreeVector((G4UniformRand()-0.5)*2*mm, 0, -25*cm));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step5C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().z() > 0 && s->GetPreStepPoint()->GetPosition().z() <= 0) {
      g_mlc_interleaf_leak += 1.0;
    }
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det5C()); rm->SetUserInitialization(new Phys5C());
  rm->SetUserAction(new Prim5C()); rm->SetUserAction(new Step5C());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T2-M5 Arm C] MLC Leaf Rounded Leak Transmission = " << g_mlc_interleaf_leak << std::endl;
  delete rm; return 0;
}
