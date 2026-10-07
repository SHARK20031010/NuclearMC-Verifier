# T1-1 测试提示词

## A组：直接生成（原始提问）

> 1 MeV 的伽马垂直打在一块 5 厘米厚的铅板上，有多少能穿过去？写个 Geant4 程序算一下。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T1-1 - B组（通用自查重审）

## 原始需求：
> 1 MeV 的伽马垂直打在一块 5 厘米厚的铅板上，有多少能穿过去？写个 Geant4 程序算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T1-1: 1 MeV gamma pencil beam through a 5 cm lead plate -> transmission.
// build: g++ -O2 -std=c++17 code.cc -o t11 $(geant4-config --cflags) $(geant4-config --libs)
// run  : ./t11 [nEvents]     (default 2000)
#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4VModularPhysicsList.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Gamma.hh"
#include <cstdio>
#include <cstdlib>
#include <cmath>

static const G4double kThick = 5.0 * cm;   // lead plate thickness (front face at z = 0)
static const G4double kHalf = 10.0 * cm;   // transverse half size of the plate
static G4long gPrimary = 0, gThrough = 0, gUncollided = 0;

// --------- geometry: world + lead plate + thin scoring slab behind the plate ---
class Detector : public G4VUserDetectorConstruction {
 public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* pb = nist->FindOrBuildMaterial("G4_Pb");
    auto* world = new G4LogicalVolume(new G4Box("World", 40 * cm, 40 * cm, 40 * cm), air, "World");
    auto* physW = new G4PVPlacement(nullptr, {}, world, "World", nullptr, false, 0);
    auto* plate = new G4LogicalVolume(new G4Box("Plate", kHalf, kHalf, kThick / 2), pb, "Plate");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kThick / 2), plate, "Plate", world, false, 0);
    // scoring slab: a 5 mm air layer touching the rear face of the plate
    auto* score = new G4LogicalVolume(new G4Box("Score", kHalf, kHalf, 0.25 * cm), air, "Score");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kThick + 0.25 * cm), score, "Score", world, false, 0);
    return physW;
  }
};

// --------- source: 1 MeV gammas, pencil beam along +z --------------------------
class Primary : public G4VUserPrimaryGeneratorAction {
 public:
  Primary() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    fGun->SetParticleEnergy(1.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -10 * cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    fGun->GeneratePrimaryVertex(ev);
    ++gPrimary;
  }

 private:
  G4ParticleGun* fGun;
};

// --------- score: gammas stepping from the plate into the rear air slab --------
class Stepping : public G4UserSteppingAction {
 public:
  void UserSteppingAction(const G4Step* step) override {
    const G4VPhysicalVolume* pre = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetLogicalVolume()->GetName() != "Plate") return;
    const G4VPhysicalVolume* post = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!post || post->GetLogicalVolume()->GetName() != "Score") return;  // left rear face
    const G4Track* trk = step->GetTrack();
    if (trk->GetDefinition() != G4Gamma::Gamma()) return;   // only photons counted
    ++gThrough;
    if (trk->GetTrackID() == 1 && trk->GetMomentumDirection().z() > 0 &&
        trk->GetKineticEnergy() > 0.999 * MeV) ++gUncollided;
  }
};

int main(int argc, char** argv) {
  const G4long nEvents = (argc > 1) ? std::atol(argv[1]) : 2000;
  auto* run = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  run->SetUserInitialization(new Detector);
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());   // EM only: no hadronic data needed
  pl->SetVerboseLevel(0);
  run->SetUserInitialization(pl);
  run->SetUserAction(new Primary);
  run->SetUserAction(new Stepping);
  run->Initialize();
  run->BeamOn((G4int)nEvents);

  const G4double n = (G4double)gPrimary;
  // NIST XCOM at 1 MeV: mu/rho = 0.0711 cm2/g, rho(Pb) = 11.35 g/cm3 -> mu = 0.807 /cm
  std::printf("\n==== T1-1: 1 MeV gamma / 5 cm Pb ====\n");
  std::printf("incident gammas               : %ld\n", gPrimary);
  std::printf("gammas out of the rear face   : %ld  (%.3f %% +- %.3f %%)   all energies\n", gThrough,
              100 * gThrough / n, 100 * std::sqrt(gThrough * (1.0 - gThrough / n)) / n);
  std::printf("  ... uncollided 1 MeV forward: %ld  (%.3f %%)\n", gUncollided, 100 * gUncollided / n);
  std::printf("narrow-beam exp(-mu*x)        : %.3f %%   (mu = 0.807 /cm)\n", 100 * std::exp(-0.807 * 5.0));
  delete run;
  return 0;
}

```

## 自查重审与修正要求：
请你仔细复核上面这段 Geant4 代码。检查几何构建、材料与物理列表设置、粒子源空间与能量抽样、计分步点选择与观测量定义、归一化与单位换算、以及统计不确定度是否存在物理错误或逻辑疏漏。

请严格遵照以下外壳规范交付最终代码：
1. 目标平台 Geant4 11.2.2，单个 .cc 文件，自带 main()，能直接 g++ 编译运行，不依赖外部宏文件。
2. 代码尽量短：控制在 100~150 行以内，不要复杂几何。
3. 请在发现问题后进行针对性修正，输出一份完整可编译的 C++ 源码。


---

## C组：加核验器（物理守恒检查报错后重修）

# 蒙特卡洛任务 T1-1 - C组（物理核验器干预）

## 原始需求：
> 1 MeV 的伽马垂直打在一块 5 厘米厚的铅板上，有多少能穿过去？写个 Geant4 程序算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T1-1: 1 MeV gamma pencil beam through a 5 cm lead plate -> transmission.
// build: g++ -O2 -std=c++17 code.cc -o t11 $(geant4-config --cflags) $(geant4-config --libs)
// run  : ./t11 [nEvents]     (default 2000)
#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4VModularPhysicsList.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4Gamma.hh"
#include <cstdio>
#include <cstdlib>
#include <cmath>

static const G4double kThick = 5.0 * cm;   // lead plate thickness (front face at z = 0)
static const G4double kHalf = 10.0 * cm;   // transverse half size of the plate
static G4long gPrimary = 0, gThrough = 0, gUncollided = 0;

// --------- geometry: world + lead plate + thin scoring slab behind the plate ---
class Detector : public G4VUserDetectorConstruction {
 public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* pb = nist->FindOrBuildMaterial("G4_Pb");
    auto* world = new G4LogicalVolume(new G4Box("World", 40 * cm, 40 * cm, 40 * cm), air, "World");
    auto* physW = new G4PVPlacement(nullptr, {}, world, "World", nullptr, false, 0);
    auto* plate = new G4LogicalVolume(new G4Box("Plate", kHalf, kHalf, kThick / 2), pb, "Plate");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kThick / 2), plate, "Plate", world, false, 0);
    // scoring slab: a 5 mm air layer touching the rear face of the plate
    auto* score = new G4LogicalVolume(new G4Box("Score", kHalf, kHalf, 0.25 * cm), air, "Score");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kThick + 0.25 * cm), score, "Score", world, false, 0);
    return physW;
  }
};

// --------- source: 1 MeV gammas, pencil beam along +z --------------------------
class Primary : public G4VUserPrimaryGeneratorAction {
 public:
  Primary() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    fGun->SetParticleEnergy(1.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -10 * cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    fGun->GeneratePrimaryVertex(ev);
    ++gPrimary;
  }

 private:
  G4ParticleGun* fGun;
};

// --------- score: gammas stepping from the plate into the rear air slab --------
class Stepping : public G4UserSteppingAction {
 public:
  void UserSteppingAction(const G4Step* step) override {
    const G4VPhysicalVolume* pre = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!pre || pre->GetLogicalVolume()->GetName() != "Plate") return;
    const G4VPhysicalVolume* post = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!post || post->GetLogicalVolume()->GetName() != "Score") return;  // left rear face
    const G4Track* trk = step->GetTrack();
    if (trk->GetDefinition() != G4Gamma::Gamma()) return;   // only photons counted
    ++gThrough;
    if (trk->GetTrackID() == 1 && trk->GetMomentumDirection().z() > 0 &&
        trk->GetKineticEnergy() > 0.999 * MeV) ++gUncollided;
  }
};

int main(int argc, char** argv) {
  const G4long nEvents = (argc > 1) ? std::atol(argv[1]) : 2000;
  auto* run = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  run->SetUserInitialization(new Detector);
  auto* pl = new G4VModularPhysicsList();
  pl->RegisterPhysics(new G4EmStandardPhysics_option4());   // EM only: no hadronic data needed
  pl->SetVerboseLevel(0);
  run->SetUserInitialization(pl);
  run->SetUserAction(new Primary);
  run->SetUserAction(new Stepping);
  run->Initialize();
  run->BeamOn((G4int)nEvents);

  const G4double n = (G4double)gPrimary;
  // NIST XCOM at 1 MeV: mu/rho = 0.0711 cm2/g, rho(Pb) = 11.35 g/cm3 -> mu = 0.807 /cm
  std::printf("\n==== T1-1: 1 MeV gamma / 5 cm Pb ====\n");
  std::printf("incident gammas               : %ld\n", gPrimary);
  std::printf("gammas out of the rear face   : %ld  (%.3f %% +- %.3f %%)   all energies\n", gThrough,
              100 * gThrough / n, 100 * std::sqrt(gThrough * (1.0 - gThrough / n)) / n);
  std::printf("  ... uncollided 1 MeV forward: %ld  (%.3f %%)\n", gUncollided, 100 * gUncollided / n);
  std::printf("narrow-beam exp(-mu*x)        : %.3f %%   (mu = 0.807 /cm)\n", 100 * std::exp(-0.807 * 5.0));
  delete run;
  return 0;
}

```

## 认知护栏 / 老师傅插件诊断报告与质询：
【认知护栏检测结果】：
✓ 物理守恒与相空间测度不变性核验通过 。

## 修正要求：
请仔细阅读上述认知护栏/老师傅插件给出的检测提示与质询问题，针对报告中指出的具体可疑槽位与物理缺陷进行核实与修改。

请严格遵照以下外壳规范交付修正后的最终代码：
1. 目标平台 Geant4 11.2.2，单个 .cc 文件，自带 main()，能直接 g++ 编译运行，不依赖外部宏文件。
2. 代码尽量短：控制在 100~150 行以内，不要复杂几何。
3. 输出一份完整可编译且物理修正后的单一 .cc C++ 源码。

