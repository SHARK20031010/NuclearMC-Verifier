// T6-2: 质子束打厚靶 -> 中子产额与角分布 (Geant4 11.2.2, 单文件)
// 针对 PHYS-0028 缺陷修正：在每事件开始时清空去重集合 gSeen，避免跨事件 TrackID 碰撞导致中子漏计
#include "G4RunManagerFactory.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4PVPlacement.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "QGSP_BIC_HP.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ios.hh"
#include <set>
#include <cstdlib>

static const G4int NB = 12;                  // 角度分箱: 0-180 deg, 15 deg/箱
static const G4double R0 = 50*cm;            // 记录球面半径
static G4double gYield = 0., gAng[NB] = {0};
static std::set<G4int> gSeen;

// 中子每次穿过半径 R0 球面记一次, 按出射极角分箱
class StepAct : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    G4Track* t = s->GetTrack();
    if (t->GetDefinition()->GetParticleName() != "neutron") return;
    if (t->GetPosition().mag() < R0) return;               // 尚未到达球面
    if (!gSeen.insert(t->GetTrackID()).second) return;     // 同一事件内同个中子只记一次
    G4int b = (G4int)(t->GetMomentumDirection().theta() / deg / 15.);
    gYield += 1.;
    gAng[b > NB - 1 ? NB - 1 : (b < 0 ? 0 : b)] += 1.;
  }
};

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* W = nist->FindOrBuildMaterial("G4_W");          // 钨靶, 19.3 g/cm3
    auto* world = new G4LogicalVolume(new G4Box("world", 60*cm, 60*cm, 60*cm),
                                      nist->FindOrBuildMaterial("G4_AIR"), "world");
    auto* pw = new G4PVPlacement(0, G4ThreeVector(), world, "world", 0, false, 0);
    auto* wl = new G4LogicalVolume(new G4Tubs("target", 0, 5*cm, 10*cm, 0, 360*deg), W, "target");
    new G4PVPlacement(0, G4ThreeVector(), wl, "target", world, false, 0);  // 厚 20 cm
    return pw;
  }
};

class Gun : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun{1};
public:
  Gun() {
    fGun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    fGun.SetParticleEnergy(1*GeV);                        // 质子动能
    fGun.SetParticlePosition(G4ThreeVector(0, 0, -12*cm));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun.GeneratePrimaryVertex(e); }
};

class EventAct : public G4UserEventAction {
public:
  // PHYS-0028 修正：Geant4 中每事件 TrackID 从 1 重新编号，每事件开始清空去重容器
  void BeginOfEventAction(const G4Event*) override { gSeen.clear(); }
};

class RunAct : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    gYield = 0.;
    gSeen.clear();
    for (G4int i = 0; i < NB; i++) gAng[i] = 0.;
  }
  void EndOfRunAction(const G4Run* r) override {
    G4double tot = 0.;
    for (G4int i = 0; i < NB; i++) tot += gAng[i];
    G4cout << "\n==== T6-2 : 1 GeV 质子轰击 20 cm 厚钨靶 ====" << G4endl;
    G4cout << "初级质子数 = " << r->GetNumberOfEvent() << G4endl;
    G4cout << "中子产额 = " << gYield << "  (每质子 " << gYield / r->GetNumberOfEvent()
           << " 个中子)" << G4endl;
    G4cout << " 角度区间[deg]   中子数     归一化角分布" << G4endl;
    for (G4int i = 0; i < NB; i++)
      G4cout << "  " << i * 15 << "-" << (i + 1) * 15 << "\t\t" << gAng[i] << "\t"
             << (tot > 0. ? gAng[i] / tot : 0.) << G4endl;
  }
};

class ActInit : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Gun);
    SetUserAction(new RunAct);
    SetUserAction(new EventAct);
    SetUserAction(new StepAct);
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100;
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new Det);
  rm->SetUserInitialization(new QGSP_BIC_HP);   // 级联 + 高精度中子输运
  rm->SetUserInitialization(new ActInit);
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
