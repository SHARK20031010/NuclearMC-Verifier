// T3-2: 纯铁样品被 14 MeV 中子照射 -> 生成哪些放射性核素、各多少个 (Level 3 第一性原理修复版)
// g++ -O2 -std=c++17 code_level3.cc $(geant4-config --cflags --libs)
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
#include "G4IonTable.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include <cstdio>
#include <map>

using namespace std;

// 基于第一性原理激发态完备性：查询该 (Z, A) 核素的基态放射性属性，杜绝硬编码与同质异能激发态混淆
static G4bool isRA(G4int k)
{
  G4int Z = k / 1000;
  G4int A = k % 1000;
  if (Z < 1 || A < 1) return false;
  auto* ground = G4IonTable::GetIonTable()->GetIon(Z, A, 0.0);
  return (ground != nullptr && !ground->GetPDGStable() && ground->GetPDGLifeTime() > 0.0);
}

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

// 产生奇点与连续输运线积分隔离：在核反应微观创生奇点捕获次级核素
class Step : public G4UserSteppingAction { public:
  void UserSteppingAction(const G4Step* step) override
  {
    auto* vol = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!vol || vol->GetName() != "Fe") return;

    auto* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (!proc) return;

    G4String pn = proc->GetProcessName();
    // 仅在中子核反应（非弹性散射与俘获）奇点提取新生核素，排除弹性散射回弹核与衰变产物
    if (pn == "neutronInelastic" || pn == "nCapture") {
      const auto* secondaries = step->GetSecondaryInCurrentStep();
      if (!secondaries) return;
      for (const auto* secTrack : *secondaries) {
        auto* def = secTrack->GetDefinition();
        if (def && def->GetParticleType() == "nucleus") {
          G4int Z = G4int(def->GetPDGCharge() / eplus + 0.5);
          G4int A = def->GetBaryonNumber();
          G4int code = Z * 1000 + A;
          if (isRA(code)) {
            gAtoms[code]++;
          }
        }
      }
    }
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
  rm->SetUserAction(new Run());
  rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(10000);   // 10000 个中子，保证统计收敛性
  return 0;
}
