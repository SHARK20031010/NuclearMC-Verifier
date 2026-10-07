// T3-2: 纯铁样品被 14 MeV 中子照射 -> 生成放射性核素种类与数量
// 编译: g++ -O2 code.cc $(geant4-config --cflags --libs) -o code
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
#include "G4HadronicProcess.hh"
#include "G4IonTable.hh"
#include "G4Step.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>

// 几何构建：世界体为真空，样品为 6x6x6 cm 纯铁块，居中放置
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 20*cm, 20*cm, 20*cm),
                                        nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    auto* feLV = new G4LogicalVolume(new G4Box("Fe", 3*cm, 3*cm, 3*cm),
                                    nist->FindOrBuildMaterial("G4_Fe"), "Fe");
    new G4PVPlacement(nullptr, {}, feLV, "Fe", worldLV, false, 0);
    return worldPV;
  }
};

// 物理列表：高精度中子物理模型 HP + 放射性衰变过程
class Phys : public G4VModularPhysicsList {
public:
  Phys() {
    RegisterPhysics(new G4EmStandardPhysics_option4()); RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4HadronElasticPhysicsHP()); RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
    RegisterPhysics(new G4RadioactiveDecayPhysics()); SetVerboseLevel(0);
  }
};

// 粒子源：14 MeV 中子，垂直入射铁块前表面
class Gun : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun{1};
public:
  Gun() {
    fGun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun.SetParticleEnergy(14 * MeV);
    fGun.SetParticlePosition({0, 0, -5 * cm}); fGun.SetParticleMomentumDirection({0, 0, 1});
  }
  void GeneratePrimaries(G4Event* e) override { fGun.GeneratePrimaryVertex(e); }
};

struct NuclideInfo { G4int Z, A, count; };
static std::map<std::string, NuclideInfo> gNuclides;

// 计分：在铁样品内发生非弹性散射或俘获时，通过核子数与电荷数守恒精确判定残余核(弥补HP模型缺重反冲核)
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* prePV = step->GetPreStepPoint()->GetPhysicalVolume();
    auto* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (!prePV || prePV->GetName() != "Fe" || !proc) return;
    G4String pn = proc->GetProcessName();
    if (pn != "neutronInelastic" && pn != "nCapture") return;

    auto* hadProc = dynamic_cast<const G4HadronicProcess*>(proc);
    const G4Nucleus* target = hadProc ? hadProc->GetTargetNucleus() : nullptr;
    auto* sec = step->GetSecondaryInCurrentStep();
    if (!target || target->GetZ_asInt() <= 0 || !sec) return;

    G4int sumZ = 0, sumA = 0;
    for (auto* trk : *sec) {
      auto* def = trk->GetDefinition();
      if (def->GetAtomicMass() <= 4) {
        sumZ += def->GetAtomicNumber();
        sumA += def->GetAtomicMass();
      }
    }
    G4int resZ = target->GetZ_asInt() - sumZ, resA = target->GetA_asInt() + 1 - sumA;
    if (resZ > 0 && resA > 0) {
      auto* ion = G4IonTable::GetIonTable()->GetIon(resZ, resA, 0.0);
      if (ion && !ion->GetPDGStable()) {
        auto& entry = gNuclides[ion->GetParticleName()];
        entry.Z = resZ; entry.A = resA; entry.count++;
      }
    }
  }
};

class Run : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override { gNuclides.clear(); }
  void EndOfRunAction(const G4Run* run) override {
    G4int n = run->GetNumberOfEvent();
    std::printf("\n==== 纯铁 14 MeV 中子照射: 放射性核素生成量 (入射中子 %d 个) ====\n", n);
    std::printf("%-8s %-8s %10s %14s %16s\n", "核素", "Z-A", "生成个数", "统计误差", "每中子产额");
    for (const auto& kv : gNuclides) {
      char za[16];
      std::snprintf(za, sizeof(za), "%d-%d", kv.second.Z, kv.second.A);
      G4double err = std::sqrt(kv.second.count), y = n > 0 ? (G4double)kv.second.count / n : 0.0;
      std::printf("%-8s %-8s %10d   ± %-8.1f %16.4e\n", kv.first.c_str(), za, kv.second.count, err, y);
    }
  }
};

int main(int argc, char** argv) {
  G4long nEvents = (argc > 1) ? std::atol(argv[1]) : 10000;
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
  rm->SetUserInitialization(new Det()); rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Gun()); rm->SetUserAction(new Run()); rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
  return 0;
}
