# T4-1 测试提示词

## A组：直接生成（原始提问）

> 用高纯锗探测器测 Cs-137，全能峰效率是多少？写个 Geant4 程序。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T4-1 - B组（通用自查重审）

## 原始需求：
> 用高纯锗探测器测 Cs-137，全能峰效率是多少？写个 Geant4 程序。

## 你此前初步生成的 Geant4 代码：
```cpp
// T4-1: HPGe 探测器测 Cs-137 (661.657 keV) 全能峰效率
// g++ -O2 -std=c++17 code.cc -o /tmp/T4-1 $(geant4-config --cflags) $(geant4-config --libs)
// ./T4-1 [事件数]   默认 100000
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
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -5*cm));   // 源距晶体前端面 5 cm
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
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
  G4int Peak() const { return fPeak; }  G4int Hit() const { return fHit; }
};
class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    auto ea = (Evt*)const_cast<G4UserEventAction*>(
        G4RunManager::GetRunManager()->GetUserEventAction());
    G4int n = r->GetNumberOfEvent();
    G4cout << "\n==== Cs-137 (661.657 keV) HPGe 全能峰效率 ====" << G4endl
           << "入射光子数      : " << n << G4endl
           << "锗中有作用事件数: " << ea->Hit() << "  (" << 100.0*ea->Hit()/n << " %)" << G4endl
           << "全能峰计数      : " << ea->Peak() << G4endl
           << "全能峰效率      : " << 100.0*ea->Peak()/n << " %" << G4endl;
  }
};
class Act : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Prim); SetUserAction(new Run);
    SetUserAction(new Evt); SetUserAction(new Step);
  }
};
int main(int argc, char** argv) {
  G4int n = (argc > 1) ? atoi(argv[1]) : 100000;
  auto rm = new G4RunManager;
  rm->SetUserInitialization(new Det);  auto pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics_option4);  // 低能电磁：光电/康普顿
  pl->RegisterPhysics(new G4DecayPhysics);
  rm->SetUserInitialization(pl);  rm->SetUserInitialization(new Act);
  rm->Initialize();  rm->BeamOn(n);  delete rm;
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

# 蒙特卡洛任务 T4-1 - C组（物理核验器干预）

## 原始需求：
> 用高纯锗探测器测 Cs-137，全能峰效率是多少？写个 Geant4 程序。

## 你此前初步生成的 Geant4 代码：
```cpp
// T4-1: HPGe 探测器测 Cs-137 (661.657 keV) 全能峰效率
// g++ -O2 -std=c++17 code.cc -o /tmp/T4-1 $(geant4-config --cflags) $(geant4-config --libs)
// ./T4-1 [事件数]   默认 100000
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
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -5*cm));   // 源距晶体前端面 5 cm
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
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
  G4int Peak() const { return fPeak; }  G4int Hit() const { return fHit; }
};
class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    auto ea = (Evt*)const_cast<G4UserEventAction*>(
        G4RunManager::GetRunManager()->GetUserEventAction());
    G4int n = r->GetNumberOfEvent();
    G4cout << "\n==== Cs-137 (661.657 keV) HPGe 全能峰效率 ====" << G4endl
           << "入射光子数      : " << n << G4endl
           << "锗中有作用事件数: " << ea->Hit() << "  (" << 100.0*ea->Hit()/n << " %)" << G4endl
           << "全能峰计数      : " << ea->Peak() << G4endl
           << "全能峰效率      : " << 100.0*ea->Peak()/n << " %" << G4endl;
  }
};
class Act : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Prim); SetUserAction(new Run);
    SetUserAction(new Evt); SetUserAction(new Step);
  }
};
int main(int argc, char** argv) {
  G4int n = (argc > 1) ? atoi(argv[1]) : 100000;
  auto rm = new G4RunManager;
  rm->SetUserInitialization(new Det);  auto pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics_option4);  // 低能电磁：光电/康普顿
  pl->RegisterPhysics(new G4DecayPhysics);
  rm->SetUserInitialization(pl);  rm->SetUserInitialization(new Act);
  rm->Initialize();  rm->BeamOn(n);  delete rm;
  return 0;
}
```

## 认知护栏 / 老师傅插件诊断报告与质询：
【认知护栏检测结果】：
✓ 物理守恒与相空间测度不变性核验通过 （自动识别核素：Cs-137）。

## 修正要求：
请仔细阅读上述认知护栏/老师傅插件给出的检测提示与质询问题，针对报告中指出的具体可疑槽位与物理缺陷进行核实与修改。

请严格遵照以下外壳规范交付修正后的最终代码：
1. 目标平台 Geant4 11.2.2，单个 .cc 文件，自带 main()，能直接 g++ 编译运行，不依赖外部宏文件。
2. 代码尽量短：控制在 100~150 行以内，不要复杂几何。
3. 输出一份完整可编译且物理修正后的单一 .cc C++ 源码。

