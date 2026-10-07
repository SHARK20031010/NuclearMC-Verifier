// T3-2: 纯铁样品被 14 MeV 中子照射 -> 生成哪些放射性核素、各多少个
// g++ -O2 -std=c++17 code.cc -o /tmp/T3-2 $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Step.hh"
#include "G4VPhysicalVolume.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include <cstdio>
#include <map>
using namespace std;
// 天然铁被中子照射会生成的放射性核素(Z*1000+A), 预声明为稳定, 免得初始化时暂停等输入
static const G4int kRA[] = {26053, 26055, 25054, 25056, 27056, 27057, 27058, 27060,
                            24510, 23048, 23049, 21046, 21047, 21048};
static G4bool isRA(G4int k)
{ for (G4int r : kRA) if (r == k) return true; return false; }
class Det : public G4VUserDetectorConstruction { public:
  G4VPhysicalVolume* Construct() override
  {
    auto* fe = G4NistManager::Instance()->FindOrBuildMaterial("G4_Fe");
    auto* lv = new G4LogicalVolume(new G4Box("Fe", 3*cm, 3*cm, 3*cm), fe, "Fe");
    return new G4PVPlacement(nullptr, {}, lv, "Fe", nullptr, false, 0);   // 纯铁 6x6x6 cm
  }
};
class Phys : public G4VModularPhysicsList { public:
  Phys()
  {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4HadronElasticPhysicsHP());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());   // HP 中子输运到 20 MeV
    RegisterPhysics(new G4RadioactiveDecayPhysics());    // 产物核可继续衰变
    SetVerboseLevel(0);
  }
};
class Gun : public G4VUserPrimaryGeneratorAction { public:
  G4ParticleGun* fGun;
  Gun() : fGun(new G4ParticleGun(1))
  {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(14*MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -3*cm));   // 源在样品前表面
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
};
map<G4int, G4int> gAtoms;   // 各放射性核素生成个数, 对全部入射中子累加
// 判据: 样品内的核, 由中子作用直接产生, 或由光子蒸发/衰变过程留下的残余核
class Step : public G4UserSteppingAction { public:
  void UserSteppingAction(const G4Step* step) override
  {
    auto* tr = step->GetTrack(); auto* def = tr->GetDefinition();
    auto* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc == nullptr || tr->GetTrackID() == 1 || def->GetParticleType() != "nucleus") return;
    if (tr->GetVolume() == nullptr || tr->GetVolume()->GetName() != "Fe") return;
    G4String pn = proc->GetProcessName();
    if (pn == "neutronInelastic" || pn == "nCapture" || pn == "hadElastic" || pn == "nFission"
        || pn == "PhotoEvaporation" || pn == "RadioactiveDecay" || pn == "Decay")
      gAtoms[G4int(def->GetPDGCharge() / eplus + 0.5) * 1000 + def->GetBaryonNumber()]++;
  }
};
class Run : public G4UserRunAction { public:
  void EndOfRunAction(const G4Run* run) override
  {
    printf("\n==== 纯铁 14 MeV 中子照射: 放射性核素生成量 (入射中子 %d 个) ====\n%-8s %10s\n",
           run->GetNumberOfEvent(), "Z-A", "生成个数");
    for (auto& it : gAtoms) {
      char n[16];
      snprintf(n, sizeof n, "%d-%d", it.first / 1000, it.first % 1000);
      if (isRA(it.first) && it.second > 0) printf("%-8s %10d\n", n, it.second);
    }
  }
};
int main()
{
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Gun());
  rm->SetUserAction(new Run()); rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(100);   // 中子数, 调大可得更好统计
  return 0;
}
