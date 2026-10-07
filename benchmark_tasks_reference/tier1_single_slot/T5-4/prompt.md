# T5-4 测试提示词

## A组：直接生成（原始提问）

> 质子束打进水里，布拉格峰附近的剂量分布是什么样？Geant4 算一下。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T5-4 - B组（通用自查重审）

## 原始需求：
> 质子束打进水里，布拉格峰附近的剂量分布是什么样？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T5-4: 150 MeV 质子束垂直入射水模体，统计深度剂量分布（布拉格峰）
// g++ -O2 -std=c++17 code.cc -o /tmp/T5-4 $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4UserLimits.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Proton.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4ios.hh"
#include "FTFP_BERT.hh"
#include "G4EmStandardPhysics_option4.hh"
#include <vector>
#include <algorithm>

static const G4int NBIN = 400;                 // 1 mm/箱，覆盖 0-40 cm
static std::vector<G4double> gEdep(NBIN, 0.);

// 几何: 20x20x40 cm 水模体, 沿 z 从 0 到 40 cm（最简平板）
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto worldL = new G4LogicalVolume(new G4Box("World", 20*cm, 20*cm, 60*cm), nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto worldP = new G4PVPlacement(nullptr, {}, worldL, "World", nullptr, false, 0);
    auto waterL = new G4LogicalVolume(new G4Box("Water", 10*cm, 10*cm, 20*cm), nist->FindOrBuildMaterial("G4_WATER"), "Water");
    waterL->SetUserLimits(new G4UserLimits(1.*mm));   // 步长 <= 1 mm，深度分箱才正确
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 20*cm), waterL, "Water", worldL, false, 0);
    return worldP;
  }
};

// 初级粒子: 150 MeV 质子, 沿 +z 入射
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Proton::ProtonDefinition());
    fGun->SetParticleEnergy(150*MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -1*cm));   // 水中 z=0 起为水
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};

// 逐 step 累积能量沉积，按深度（z）分箱
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    G4double e = step->GetTotalEnergyDeposit();
    if (e <= 0.) return;
    auto pre = step->GetPreStepPoint()->GetPhysicalVolume();
    auto post = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!((pre && pre->GetName() == "Water") || (post && post->GetName() == "Water"))) return;
    G4double z = 0.5*(step->GetPreStepPoint()->GetPosition().z() + step->GetPostStepPoint()->GetPosition().z());
    G4int i = (G4int)(z/mm);
    if (i < 0) i = 0;                                  // 入水那一步归到第 0 箱
    if (i < NBIN) gEdep[i] += e;
  }
};

// 运行结束打印深度剂量曲线（单位: MeV / mm / 质子）
class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override { std::fill(gEdep.begin(), gEdep.end(), 0.); }
  void EndOfRunAction(const G4Run* run) override {
    G4int n = run->GetNumberOfEvent();
    G4cout << "# depth_mm  dose_MeV_per_mm_per_proton  (150 MeV p in water, " << n << " events)" << G4endl;
    for (G4int i = 0; i < NBIN; ++i)
      if (gEdep[i] > 0.) G4cout << i + 0.5 << "  " << gEdep[i]/n << G4endl;
  }
};

int main() {
  auto rm = new G4RunManager();
  rm->SetUserInitialization(new Detector());
  auto pl = new FTFP_BERT();
  pl->ReplacePhysics(new G4EmStandardPhysics_option4());   // 低能质子电离/射程更准
  pl->RegisterPhysics(new G4StepLimiterPhysics());         // 启用几何步长限制
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Primary());
  rm->SetUserAction(new RunAction());
  rm->SetUserAction(new Stepping());
  rm->Initialize();
  rm->BeamOn(100);                                         // 验证用：100 个质子
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

# 蒙特卡洛任务 T5-4 - C组（物理核验器干预）

## 原始需求：
> 质子束打进水里，布拉格峰附近的剂量分布是什么样？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T5-4: 150 MeV 质子束垂直入射水模体，统计深度剂量分布（布拉格峰）
// g++ -O2 -std=c++17 code.cc -o /tmp/T5-4 $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4UserLimits.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Proton.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4ios.hh"
#include "FTFP_BERT.hh"
#include "G4EmStandardPhysics_option4.hh"
#include <vector>
#include <algorithm>

static const G4int NBIN = 400;                 // 1 mm/箱，覆盖 0-40 cm
static std::vector<G4double> gEdep(NBIN, 0.);

// 几何: 20x20x40 cm 水模体, 沿 z 从 0 到 40 cm（最简平板）
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    auto worldL = new G4LogicalVolume(new G4Box("World", 20*cm, 20*cm, 60*cm), nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto worldP = new G4PVPlacement(nullptr, {}, worldL, "World", nullptr, false, 0);
    auto waterL = new G4LogicalVolume(new G4Box("Water", 10*cm, 10*cm, 20*cm), nist->FindOrBuildMaterial("G4_WATER"), "Water");
    waterL->SetUserLimits(new G4UserLimits(1.*mm));   // 步长 <= 1 mm，深度分箱才正确
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 20*cm), waterL, "Water", worldL, false, 0);
    return worldP;
  }
};

// 初级粒子: 150 MeV 质子, 沿 +z 入射
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Proton::ProtonDefinition());
    fGun->SetParticleEnergy(150*MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -1*cm));   // 水中 z=0 起为水
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override { fGun->GeneratePrimaryVertex(ev); }
};

// 逐 step 累积能量沉积，按深度（z）分箱
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    G4double e = step->GetTotalEnergyDeposit();
    if (e <= 0.) return;
    auto pre = step->GetPreStepPoint()->GetPhysicalVolume();
    auto post = step->GetPostStepPoint()->GetPhysicalVolume();
    if (!((pre && pre->GetName() == "Water") || (post && post->GetName() == "Water"))) return;
    G4double z = 0.5*(step->GetPreStepPoint()->GetPosition().z() + step->GetPostStepPoint()->GetPosition().z());
    G4int i = (G4int)(z/mm);
    if (i < 0) i = 0;                                  // 入水那一步归到第 0 箱
    if (i < NBIN) gEdep[i] += e;
  }
};

// 运行结束打印深度剂量曲线（单位: MeV / mm / 质子）
class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override { std::fill(gEdep.begin(), gEdep.end(), 0.); }
  void EndOfRunAction(const G4Run* run) override {
    G4int n = run->GetNumberOfEvent();
    G4cout << "# depth_mm  dose_MeV_per_mm_per_proton  (150 MeV p in water, " << n << " events)" << G4endl;
    for (G4int i = 0; i < NBIN; ++i)
      if (gEdep[i] > 0.) G4cout << i + 0.5 << "  " << gEdep[i]/n << G4endl;
  }
};

int main() {
  auto rm = new G4RunManager();
  rm->SetUserInitialization(new Detector());
  auto pl = new FTFP_BERT();
  pl->ReplacePhysics(new G4EmStandardPhysics_option4());   // 低能质子电离/射程更准
  pl->RegisterPhysics(new G4StepLimiterPhysics());         // 启用几何步长限制
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Primary());
  rm->SetUserAction(new RunAction());
  rm->SetUserAction(new Stepping());
  rm->Initialize();
  rm->BeamOn(100);                                         // 验证用：100 个质子
  delete rm;
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

