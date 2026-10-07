// T4-1: HPGe 探测器测 Cs-137 (661.657 keV) 全能峰效率
// 针对 PHYS-0024/0029 缺陷修正：点源在 4pi 全立体角各向同性发射，修复零发散平行束超几何上限
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o T4-1
// 运行: ./T4-1 [事件数]   默认 100000
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4RandomDirection.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4ios.hh"
#include <cstdlib>
#include <cmath>

static G4LogicalVolume* gGe = nullptr;   // 锗晶体：只统计这里的沉积能
static G4double gEdep = 0.;              // 本事件在锗中的总沉积能

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto world = new G4LogicalVolume(new G4Box("W", 15*cm, 15*cm, 15*cm),
                                     nist->FindOrBuildMaterial("G4_Galactic"), "W");
    auto worldPV = new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
    // HPGe 同轴晶体：直径 6 cm、长 5 cm，前端面在 z=0
    auto ge = new G4LogicalVolume(new G4Tubs("Ge", 0, 3*cm, 2.5*cm, 0, 360*deg),
                                  nist->FindOrBuildMaterial("G4_Ge"), "Ge");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 2.5*cm), ge, "Ge", world, false, 0);
    gGe = ge;
    return worldPV;
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Prim() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    fGun->SetParticleEnergy(661.657*keV);                    // Cs-137 全能峰
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -5*cm));   // 点源距晶体前端面 5 cm
  }
  ~Prim() override { delete fGun; }
  void GeneratePrimaries(G4Event* e) override {
    // 针对 PHYS-0024/0029 缺陷修正：4pi 空间各向同性角抽样，真实反映点源几何立体角损失
    fGun->SetParticleMomentumDirection(G4RandomDirection());
    fGun->GeneratePrimaryVertex(e);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    if (st->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() == gGe)
      gEdep += st->GetTotalEnergyDeposit();
  }
};

class Evt : public G4UserEventAction {
  G4int fPeak = 0, fHit = 0;
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    if (gEdep > 0.) fHit++;
    if (std::abs(gEdep - 661.657*keV) < 1.0*keV) fPeak++;    // 全能峰计数
  }
  G4int Peak() const { return fPeak; }
  G4int Hit() const { return fHit; }
};

class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    auto ea = (Evt*)const_cast<G4UserEventAction*>(
        G4RunManager::GetRunManager()->GetUserEventAction());
    G4int n = r->GetNumberOfEvent();
    G4cout << "\n==== Cs-137 (661.657 keV) HPGe 全能峰绝对效率 ====" << G4endl
           << "发射总光子数    : " << n << G4endl
           << "锗中有作用事件数: " << ea->Hit() << "  (" << 100.0*ea->Hit()/n << " %)" << G4endl
           << "全能峰计数      : " << ea->Peak() << G4endl
           << "绝对全能峰效率  : " << 100.0*ea->Peak()/n << " %" << G4endl;
  }
};

class Act : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Prim);
    SetUserAction(new Run);
    SetUserAction(new Evt);
    SetUserAction(new Step);
  }
};

int main(int argc, char** argv) {
  G4int n = (argc > 1) ? std::atoi(argv[1]) : 100000;
  auto rm = new G4RunManager;
  rm->SetUserInitialization(new Det);
  auto pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics_option4);  // 低能电磁：光电/康普顿
  pl->RegisterPhysics(new G4DecayPhysics);
  rm->SetUserInitialization(pl);
  rm->SetUserInitialization(new Act);
  rm->Initialize();
  rm->BeamOn(n);
  delete rm;
  return 0;
}
