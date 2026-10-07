// T1-4: 中子与伽马混合场穿透含硼聚乙烯+铅球壳屏蔽的泄漏能谱
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o T1-4
// 自查修正: 在 PostStepPoint 上读取 fGeomBoundary 判定边界穿透出射 (PHYS-0019)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ios.hh"
#include "QGSP_BIC_HP.hh"
#include <cstdlib>

static const G4int NBin = 60;
static const G4double EMax = 15 * MeV;
static G4double gNE[NBin] = {0}, gNG[NBin] = {0};
static G4double gTotN = 0, gTotG = 0;

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    G4Material* pe = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
    G4Material* bpe = new G4Material("BPE", 0.95 * g / cm3, 2);
    bpe->AddMaterial(pe, 95 * perCent);
    bpe->AddElement(nist->FindOrBuildElement("B"), 5 * perCent);
    G4Material* pb = nist->FindOrBuildMaterial("G4_Pb");
    auto worldL = new G4LogicalVolume(new G4Box("World", 30 * cm, 30 * cm, 30 * cm),
                                      nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto worldP = new G4PVPlacement(0, {}, worldL, "World", 0, false, 0);
    new G4PVPlacement(0, {}, new G4LogicalVolume(
        new G4Sphere("BPE", 0, 10 * cm, 0, 360 * deg, 0, 180 * deg), bpe, "BPE"),
        "BPE", worldL, false, 0);
    new G4PVPlacement(0, {}, new G4LogicalVolume(
        new G4Sphere("Pb", 10 * cm, 13 * cm, 0, 360 * deg, 0, 180 * deg), pb, "Pb"),
        "Pb", worldL, false, 0);
    return worldP;
  }
};

class Prim : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Prim() : fGun(new G4ParticleGun(1)) {}
  ~Prim() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    auto pt = G4ParticleTable::GetParticleTable();
    G4bool isN = (ev->GetEventID() % 2 == 0);
    fGun->SetParticleDefinition(pt->FindParticle(isN ? "neutron" : "gamma"));
    fGun->SetParticleEnergy(isN ? 14 * MeV : 1.25 * MeV);
    fGun->SetParticlePosition({0, 0, 0});
    fGun->SetParticleMomentumDirection({0, 0, 1});
    fGun->GeneratePrimaryVertex(ev);
  }
};

class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    auto pre = st->GetPreStepPoint();
    auto post = st->GetPostStepPoint();
    // 修正: 边界出射判定必须在 PostStepPoint 上读取 fGeomBoundary
    if (post->GetStepStatus() != fGeomBoundary) return;
    if (!pre->GetPhysicalVolume() || pre->GetPhysicalVolume()->GetName() != "Pb") return;
    if (!post->GetPhysicalVolume() || post->GetPhysicalVolume()->GetName() != "World") return;
    G4double e = post->GetKineticEnergy();
    G4int b = (G4int)(e / EMax * NBin);
    if (b < 0 || b >= NBin) return;
    G4String p = st->GetTrack()->GetDefinition()->GetParticleName();
    if (p == "neutron") { gNE[b]++; gTotN++; }
    else if (p == "gamma") { gNG[b]++; gTotG++; }
    st->GetTrack()->SetTrackStatus(fStopAndKill);
  }
};

class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    G4cout << "\nT1-4  events=" << r->GetNumberOfEvent()
           << "   leaked neutrons=" << gTotN << "   leaked gammas=" << gTotG << G4endl;
    G4cout << "  E(MeV)   n_leak   g_leak" << G4endl;
    for (G4int i = 0; i < NBin; i++) {
      if (gNE[i] + gNG[i] > 0)
        G4cout << "  " << (i + 0.5) * EMax / NBin / MeV << "   " << gNE[i] << "   " << gNG[i] << G4endl;
    }
  }
};

int main(int argc, char** argv) {
  G4int n = (argc > 1) ? std::atoi(argv[1]) : 100;
  auto rm = new G4RunManager;
  rm->SetUserInitialization(new Det);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Prim);
  rm->SetUserAction(new Run);
  rm->SetUserAction(new Step);
  rm->Initialize();
  rm->BeamOn(n);
  delete rm;
  return 0;
}
