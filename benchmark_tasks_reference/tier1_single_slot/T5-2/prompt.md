# T5-2 测试提示词

## A组：直接生成（原始提问）

> 细胞大小的体积里，质子沉积的比能分布是什么样？用 Geant4 算。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T5-2 - B组（通用自查重审）

## 原始需求：
> 细胞大小的体积里，质子沉积的比能分布是什么样？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T5-2: specific-energy (dose) distribution of protons in a cell-sized volume.
// Single .cc file, own main(), no external macro/data files.
// g++ -O2 -std=c++17 code.cc -o /tmp/T5-2 $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4ParticleGun.hh"
#include "G4Proton.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4AnalysisManager.hh"
#include "G4RandomTools.hh"
#include <cstdlib>
#include <cmath>
static G4LogicalVolume* gCell = nullptr;  // scoring volume
static double gMass = 0.;                 // cell mass [g]
static const double gMeVtoJ = 1.602176634e-13;  // MeV -> J (z in Gy needs SI)
static double gEdep = 0.;                 // energy deposited in cell, this event [MeV]
static const double gHalf = 5.*um;        // cell half-size -> 10 um cube (~1 pL water)
struct Det : G4VUserDetectorConstruction {
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto water = nist->FindOrBuildMaterial("G4_WATER");
    auto air = nist->FindOrBuildMaterial("G4_AIR");
    auto ws = new G4Box("World", 1.*mm, 1.*mm, 1.*mm);
    auto wl = new G4LogicalVolume(ws, air, "World");
    auto wp = new G4PVPlacement(nullptr, {}, wl, "World", nullptr, false, 0);
    auto cs = new G4Box("Cell", gHalf, gHalf, gHalf);      // cell-sized volume
    gCell = new G4LogicalVolume(cs, water, "Cell");
    new G4PVPlacement(nullptr, {}, gCell, "Cell", wl, false, 0);
    gMass = cs->GetCubicVolume()/cm3;       // 1 g/cm3 water -> mass in [g]
    return wp;
  }
};
struct Gun : G4VUserPrimaryGeneratorAction {
  G4ParticleGun gun{1};
  Gun() {
    gun.SetParticleDefinition(G4Proton::ProtonDefinition());
    gun.SetParticleEnergy(10.*MeV);                       // 10 MeV proton beam
    gun.SetParticleMomentumDirection({1, 0, 0});
  }
  void GeneratePrimaries(G4Event* ev) override {
    double r = 2.*um*std::sqrt(G4UniformRand()), a = CLHEP::twopi*G4UniformRand();
    gun.SetParticlePosition({-100.*um, r*std::cos(a), r*std::sin(a)});  // along +x
    gun.GeneratePrimaryVertex(ev);
  }
};
struct Act : G4UserSteppingAction, G4UserEventAction {
  void UserSteppingAction(const G4Step* st) override {
    if (st->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() == gCell)
      gEdep += st->GetTotalEnergyDeposit()/MeV;
  }
  void EndOfEventAction(const G4Event*) override {        // one proton -> one z sample
    auto am = G4AnalysisManager::Instance();
    am->FillH1(0, gEdep);                                 // E_dep per proton
    am->FillH1(1, gEdep*gMeVtoJ/(gMass*1e-3));            // z = E_dep/m  [Gy]
    gEdep = 0.;
  }
};
int main() {
  auto* run = new G4RunManager;
  run->SetUserInitialization(new Det);
  auto* pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics_option4);   // low-energy EM + step limit
  pl->RegisterPhysics(new G4DecayPhysics);
  pl->RegisterPhysics(new G4RadioactiveDecayPhysics);
  run->SetUserInitialization(pl);
  run->SetUserAction(new Gun);
  auto* act = new Act;
  run->SetUserAction(static_cast<G4UserSteppingAction*>(act));
  run->SetUserAction(static_cast<G4UserEventAction*>(act));
  auto* am = G4AnalysisManager::Instance();
  am->SetDefaultFileType("root");
  am->SetVerboseLevel(0);
  am->OpenFile("T5-2");
  am->CreateH1("edep", "E deposited in cell per proton;E_{dep} (MeV)", 100, 0., 2.);
  am->CreateH1("z", "Specific energy in cell per proton;z (Gy)", 100, 0., 0.6);
  run->Initialize();
  run->BeamOn(100);                                       // 100 protons: fast smoke test
  am->Write();
  am->CloseFile();
  G4cout << "100 protons done; histograms in T5-2.root" << G4endl;
  std::_Exit(0);  // skip G4 teardown (SIGSEGV in ~G4SteppingManager); file already closed
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

# 蒙特卡洛任务 T5-2 - C组（物理核验器干预）

## 原始需求：
> 细胞大小的体积里，质子沉积的比能分布是什么样？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T5-2: specific-energy (dose) distribution of protons in a cell-sized volume.
// Single .cc file, own main(), no external macro/data files.
// g++ -O2 -std=c++17 code.cc -o /tmp/T5-2 $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4ParticleGun.hh"
#include "G4Proton.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4AnalysisManager.hh"
#include "G4RandomTools.hh"
#include <cstdlib>
#include <cmath>
static G4LogicalVolume* gCell = nullptr;  // scoring volume
static double gMass = 0.;                 // cell mass [g]
static const double gMeVtoJ = 1.602176634e-13;  // MeV -> J (z in Gy needs SI)
static double gEdep = 0.;                 // energy deposited in cell, this event [MeV]
static const double gHalf = 5.*um;        // cell half-size -> 10 um cube (~1 pL water)
struct Det : G4VUserDetectorConstruction {
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto water = nist->FindOrBuildMaterial("G4_WATER");
    auto air = nist->FindOrBuildMaterial("G4_AIR");
    auto ws = new G4Box("World", 1.*mm, 1.*mm, 1.*mm);
    auto wl = new G4LogicalVolume(ws, air, "World");
    auto wp = new G4PVPlacement(nullptr, {}, wl, "World", nullptr, false, 0);
    auto cs = new G4Box("Cell", gHalf, gHalf, gHalf);      // cell-sized volume
    gCell = new G4LogicalVolume(cs, water, "Cell");
    new G4PVPlacement(nullptr, {}, gCell, "Cell", wl, false, 0);
    gMass = cs->GetCubicVolume()/cm3;       // 1 g/cm3 water -> mass in [g]
    return wp;
  }
};
struct Gun : G4VUserPrimaryGeneratorAction {
  G4ParticleGun gun{1};
  Gun() {
    gun.SetParticleDefinition(G4Proton::ProtonDefinition());
    gun.SetParticleEnergy(10.*MeV);                       // 10 MeV proton beam
    gun.SetParticleMomentumDirection({1, 0, 0});
  }
  void GeneratePrimaries(G4Event* ev) override {
    double r = 2.*um*std::sqrt(G4UniformRand()), a = CLHEP::twopi*G4UniformRand();
    gun.SetParticlePosition({-100.*um, r*std::cos(a), r*std::sin(a)});  // along +x
    gun.GeneratePrimaryVertex(ev);
  }
};
struct Act : G4UserSteppingAction, G4UserEventAction {
  void UserSteppingAction(const G4Step* st) override {
    if (st->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() == gCell)
      gEdep += st->GetTotalEnergyDeposit()/MeV;
  }
  void EndOfEventAction(const G4Event*) override {        // one proton -> one z sample
    auto am = G4AnalysisManager::Instance();
    am->FillH1(0, gEdep);                                 // E_dep per proton
    am->FillH1(1, gEdep*gMeVtoJ/(gMass*1e-3));            // z = E_dep/m  [Gy]
    gEdep = 0.;
  }
};
int main() {
  auto* run = new G4RunManager;
  run->SetUserInitialization(new Det);
  auto* pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics_option4);   // low-energy EM + step limit
  pl->RegisterPhysics(new G4DecayPhysics);
  pl->RegisterPhysics(new G4RadioactiveDecayPhysics);
  run->SetUserInitialization(pl);
  run->SetUserAction(new Gun);
  auto* act = new Act;
  run->SetUserAction(static_cast<G4UserSteppingAction*>(act));
  run->SetUserAction(static_cast<G4UserEventAction*>(act));
  auto* am = G4AnalysisManager::Instance();
  am->SetDefaultFileType("root");
  am->SetVerboseLevel(0);
  am->OpenFile("T5-2");
  am->CreateH1("edep", "E deposited in cell per proton;E_{dep} (MeV)", 100, 0., 2.);
  am->CreateH1("z", "Specific energy in cell per proton;z (Gy)", 100, 0., 0.6);
  run->Initialize();
  run->BeamOn(100);                                       // 100 protons: fast smoke test
  am->Write();
  am->CloseFile();
  G4cout << "100 protons done; histograms in T5-2.root" << G4endl;
  std::_Exit(0);  // skip G4 teardown (SIGSEGV in ~G4SteppingManager); file already closed
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

