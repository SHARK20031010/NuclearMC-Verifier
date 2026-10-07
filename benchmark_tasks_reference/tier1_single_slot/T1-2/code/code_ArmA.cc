// T1-2: 14 MeV 中子垂直入射 30 cm 厚混凝土板 —— 穿透（透射）比例
// 编译: g++ -O2 -std=c++17 code.cc -o T1-2 $(geant4-config --cflags) $(geant4-config --libs)
// 运行: ./T1-2 [nEvents]   (默认 100000)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include <cstdlib>

static G4long gTransmitted = 0;

// 物理：标准电磁 + QGSP_BIC_HP 高精度中子（含热中子输运）
class PhysicsList : public G4VModularPhysicsList {
public:
  PhysicsList() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};

// 几何：板中心在原点，30 cm 厚（前表面 z=-15 cm，后表面 z=+15 cm），周围为真空
class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4NistManager* nist = G4NistManager::Instance();
    G4LogicalVolume* worldLV = new G4LogicalVolume(
        new G4Box("World", 100. * cm, 100. * cm, 100. * cm),
        nist->FindOrBuildMaterial("G4_Galactic"), "World");
    G4VPhysicalVolume* worldPV = new G4PVPlacement(
        nullptr, G4ThreeVector(), worldLV, "World", nullptr, false, 0);
    G4LogicalVolume* slabLV = new G4LogicalVolume(
        new G4Box("Slab", 50. * cm, 50. * cm, 15. * cm),
        nist->FindOrBuildMaterial("G4_CONCRETE"), "Slab");
    new G4PVPlacement(nullptr, G4ThreeVector(), slabLV, "Slab", worldLV, false, 0);
    return worldPV;
  }
};

// 计分：中子自内向外穿过板后表面内侧计分面即算穿透
class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4double zScore = 14.999 * cm;   // 板后表面(15 cm)内侧 10 um
    if (step->GetTrack()->GetParticleDefinition()->GetParticleName() != "neutron") return;
    if (step->GetPreStepPoint()->GetPosition().z() < zScore &&
        step->GetPostStepPoint()->GetPosition().z() >= zScore &&
        step->GetPostStepPoint()->GetKineticEnergy() > 1e-5 * MeV) ++gTransmitted;
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
  PrimaryGeneratorAction() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(
        G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(14.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0., 0., -50. * cm));    // 源在板前 50 cm
    fGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));  // 垂直入射
  }
  ~PrimaryGeneratorAction() override { delete fGun; }
  void GeneratePrimaries(G4Event* evt) override { fGun->GeneratePrimaryVertex(evt); }
private:
  G4ParticleGun* fGun;
};

int main(int argc, char** argv) {
  const G4long nEvents = (argc > 1) ? std::atol(argv[1]) : 100000;

  G4RunManager* runManager = new G4RunManager;
  runManager->SetUserInitialization(new DetectorConstruction);
  runManager->SetUserInitialization(new PhysicsList);
  runManager->SetUserAction(new PrimaryGeneratorAction);
  runManager->SetUserAction(new SteppingAction);
  runManager->Initialize();

  runManager->BeamOn(nEvents);

  G4cout << "\n=== T1-2  14 MeV neutron -> 30 cm concrete ===" << G4endl;
  G4cout << "incident    = " << nEvents << G4endl;
  G4cout << "transmitted = " << gTransmitted << G4endl;
  G4cout << "transmission ratio = "
         << (nEvents ? G4double(gTransmitted) / G4double(nEvents) : 0.) << G4endl;
  delete runManager;
  return 0;
}
