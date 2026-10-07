// T2-2: Cs-137 点源(662 keV 伽马)距 30 cm 组织等效(A-150 组织等效塑料)立方体前表面 5 cm
//       统计立方体内的吸收剂量。
// 编译: g++ -O2 -std=c++17 code.cc -o code $(geant4-config --cflags) $(geant4-config --libs)
// 运行: ./code [源粒子数]      (默认 1000)
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PhysListFactory.hh"
#include "G4UImanager.hh"
#include "G4ios.hh"
#include "Randomize.hh"
#include <cmath>
#include <cstdlib>

static const G4double kSide = 30 * cm;  // 立方体边长
static const G4double kGap = 5 * cm;    // 源到前表面距离
static G4double gEdep = 0.;             // 立方体内累积沉积能量
static G4double gMass = 0.;             // 立方体质量

// ---------------- 几何：30 cm 组织等效立方体，置于空气世界中心 ----------------
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4PVPlacement(0, G4ThreeVector(), new G4LogicalVolume(
        new G4Box("World", 2 * kSide, 2 * kSide, 2 * kSide),
        nist->FindOrBuildMaterial("G4_AIR"), "World"), "World", 0, false, 0);
    auto* tissue = nist->FindOrBuildMaterial("G4_A-150_TISSUE");  // 组织等效塑料, 1.127 g/cm3
    auto* cube = new G4LogicalVolume(new G4Box("Cube", kSide / 2, kSide / 2, kSide / 2),
                                     tissue, "Cube");
    new G4PVPlacement(0, G4ThreeVector(), cube, "Cube", world->GetLogicalVolume(), false, 0);
    gMass = tissue->GetDensity() * kSide * kSide * kSide;  // 体积 27 L -> 30.43 kg
    return world;
  }
};

// ---------------- 源：Cs-137 点源在方块前 5 cm 处，662 keV 伽马各向同性发射 ----------------
class Gen : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Gen() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    fGun->SetParticleEnergy(662 * keV);                                   // Cs-137 主伽马
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -(kSide / 2 + kGap)));  // z = -20 cm
  }
  void GeneratePrimaries(G4Event* evt) override {  // 点源：各向同性抽样出射方向
    G4double ct = 2 * G4UniformRand() - 1, ph = 2 * CLHEP::pi * G4UniformRand();
    G4double st = std::sqrt(1 - ct * ct);
    fGun->SetParticleMomentumDirection(G4ThreeVector(st * std::cos(ph), st * std::sin(ph), ct));
    fGun->GeneratePrimaryVertex(evt);
  }
};

// ---------------- 计分：步长起点落在方块内即累加该步沉积能量 ----------------
class Dose : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* stp) override {
    auto* vol = stp->GetPreStepPoint()->GetPhysicalVolume();
    if (vol && vol->GetName() == G4String("Cube")) gEdep += stp->GetTotalEnergyDeposit();
  }
};

int main(int argc, char** argv) {
  G4int n = (argc > 1) ? std::atoi(argv[1]) : 1000;  // 源粒子数
  auto* run = new G4RunManager;
  run->SetUserInitialization(new Det);
  run->SetUserInitialization(G4PhysListFactory().GetReferencePhysList("QGSP_BIC_HP"));
  run->SetUserAction(new Gen);
  run->SetUserAction(new Dose);
  run->Initialize();
  G4UImanager::GetUIpointer()->ApplyCommand("/run/printProgress 500");
  run->BeamOn(n);

  G4double doseTot = gEdep / gMass;  // 沉积能量/质量 (Geant4 单位制); doseTot/gray 即 Gy
  G4cout << "\n=== T2-2  Cs-137 (662 keV) -> 30 cm 组织等效立方体, 源距前表面 5 cm ===" << G4endl
         << "源粒子数            : " << n << G4endl
         << "立方体质量          : " << gMass / g << " g   (30x30x30 cm3)" << G4endl
         << "平均沉积能量/源粒子 : " << gEdep / n / MeV << " MeV" << G4endl
         << "吸收剂量/源粒子     : " << doseTot / n / gray << " Gy" << G4endl
         << "本次模拟总吸收剂量  : " << doseTot / gray << " Gy" << G4endl
         << "注: 源为各向同性点源, 实际剂量 = 单粒子剂量 x 源发射粒子总数" << G4endl;
  delete run;
  return 0;
}
