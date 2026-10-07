# T4-2 测试提示词

## A组：直接生成（原始提问）

> NaI 探测器测 662 keV 的伽马，能谱是什么样？Geant4 算一下。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T4-2 - B组（通用自查重审）

## 原始需求：
> NaI 探测器测 662 keV 的伽马，能谱是什么样？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// NaI(Tl) 探测器测 662 keV 伽马（Cs-137）能谱 —— 单文件 Geant4 程序
// 编译: g++ -O2 -std=c++17 code.cc -o t4 $(geant4-config --cflags) $(geant4-config --libs)
// 运行: ./t4 100000   → 生成 nai_spectrum.csv（280 道 × 2.5 keV 的能谱计数）
#include "G4RunManagerFactory.hh"
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4UserEventAction.hh"
#include "G4UserRunAction.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include <cstdlib>
#include <cstdio>

static const G4int kNbins = 280;          // 2.5 keV/道，0 – 700 keV
static const G4double kEmax = 700.*keV;
static G4double gEdep = 0.;               // 本事件在 NaI 晶体内的总沉积能量
static G4int gSpec[kNbins] = {0};         // 能谱

// 几何：3"×3" 圆柱 NaI(Tl) 晶体；点源在轴线上，距晶体中心 16 cm
class MyDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 15.*cm, 15.*cm, 15.*cm),
                                        nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    auto* naiLV = new G4LogicalVolume(new G4Tubs("NaI", 0., 3.81*cm, 3.81*cm, 0., twopi),
                                      nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "NaI");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 8.*cm), naiLV, "NaI", worldLV, false, 0);
    return worldPV;
  }
};

// 物理：Opt4 标准电磁（低能伽马光电/康普顿更准）+ 衰变
class Physics : public G4VModularPhysicsList {
public:
  Physics() {
    SetVerboseLevel(0);
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
  }
};

// 初级粒子：662 keV 伽马，沿 +z 射向晶体
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticleEnergy(662.*keV);
    fGun->SetParticlePosition(G4ThreeVector(0., 0., -8.*cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
  ~Primary() override { delete fGun; }
};

// 步进：累加晶体内沉积能量
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* lv = step->GetTrack()->GetVolume()->GetLogicalVolume();
    if (lv && lv->GetName() == "NaI") gEdep += step->GetTotalEnergyDeposit();
  }
};

// 事件：每事件填一次谱
class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    G4int ib = (G4int)(gEdep/kEmax*kNbins);
    if (ib >= 0 && ib < kNbins) gSpec[ib]++;
  }
};

// 运行：结束后把能谱写成 CSV
class RunAction : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run*) override {
    FILE* f = std::fopen("nai_spectrum.csv", "w");
    if (!f) return;
    std::fprintf(f, "energy_keV,counts\n");
    for (G4int i = 0; i < kNbins; ++i)
      std::fprintf(f, "%.2f,%d\n", (i + 0.5)*kEmax/kNbins/keV, gSpec[i]);
    std::fclose(f);
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100000;
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new MyDetector());
  rm->SetUserInitialization(new Physics());
  rm->SetUserAction(new Primary());
  rm->SetUserAction(new Stepping());
  rm->SetUserAction(new EventAction());
  rm->SetUserAction(new RunAction());
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
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

# 蒙特卡洛任务 T4-2 - C组（物理核验器干预）

## 原始需求：
> NaI 探测器测 662 keV 的伽马，能谱是什么样？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// NaI(Tl) 探测器测 662 keV 伽马（Cs-137）能谱 —— 单文件 Geant4 程序
// 编译: g++ -O2 -std=c++17 code.cc -o t4 $(geant4-config --cflags) $(geant4-config --libs)
// 运行: ./t4 100000   → 生成 nai_spectrum.csv（280 道 × 2.5 keV 的能谱计数）
#include "G4RunManagerFactory.hh"
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4DecayPhysics.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4UserEventAction.hh"
#include "G4UserRunAction.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include <cstdlib>
#include <cstdio>

static const G4int kNbins = 280;          // 2.5 keV/道，0 – 700 keV
static const G4double kEmax = 700.*keV;
static G4double gEdep = 0.;               // 本事件在 NaI 晶体内的总沉积能量
static G4int gSpec[kNbins] = {0};         // 能谱

// 几何：3"×3" 圆柱 NaI(Tl) 晶体；点源在轴线上，距晶体中心 16 cm
class MyDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldLV = new G4LogicalVolume(new G4Box("World", 15.*cm, 15.*cm, 15.*cm),
                                        nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);
    auto* naiLV = new G4LogicalVolume(new G4Tubs("NaI", 0., 3.81*cm, 3.81*cm, 0., twopi),
                                      nist->FindOrBuildMaterial("G4_SODIUM_IODIDE"), "NaI");
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 8.*cm), naiLV, "NaI", worldLV, false, 0);
    return worldPV;
  }
};

// 物理：Opt4 标准电磁（低能伽马光电/康普顿更准）+ 衰变
class Physics : public G4VModularPhysicsList {
public:
  Physics() {
    SetVerboseLevel(0);
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
  }
};

// 初级粒子：662 keV 伽马，沿 +z 射向晶体
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticleEnergy(662.*keV);
    fGun->SetParticlePosition(G4ThreeVector(0., 0., -8.*cm));
    fGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
  ~Primary() override { delete fGun; }
};

// 步进：累加晶体内沉积能量
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* lv = step->GetTrack()->GetVolume()->GetLogicalVolume();
    if (lv && lv->GetName() == "NaI") gEdep += step->GetTotalEnergyDeposit();
  }
};

// 事件：每事件填一次谱
class EventAction : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override { gEdep = 0.; }
  void EndOfEventAction(const G4Event*) override {
    G4int ib = (G4int)(gEdep/kEmax*kNbins);
    if (ib >= 0 && ib < kNbins) gSpec[ib]++;
  }
};

// 运行：结束后把能谱写成 CSV
class RunAction : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run*) override {
    FILE* f = std::fopen("nai_spectrum.csv", "w");
    if (!f) return;
    std::fprintf(f, "energy_keV,counts\n");
    for (G4int i = 0; i < kNbins; ++i)
      std::fprintf(f, "%.2f,%d\n", (i + 0.5)*kEmax/kNbins/keV, gSpec[i]);
    std::fclose(f);
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 100000;
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new MyDetector());
  rm->SetUserInitialization(new Physics());
  rm->SetUserAction(new Primary());
  rm->SetUserAction(new Stepping());
  rm->SetUserAction(new EventAction());
  rm->SetUserAction(new RunAction());
  rm->Initialize();
  rm->BeamOn(nEvents);
  delete rm;
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

