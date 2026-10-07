# T4-4 测试提示词

## A组：直接生成（原始提问）

> 探测器前面有一层死层，它的厚度对探测效率影响多大？Geant4 算一下。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T4-4 - B组（通用自查重审）

## 原始需求：
> 探测器前面有一层死层，它的厚度对探测效率影响多大？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// 死层厚度对探测效率的影响：Ge 探测器前表面死层厚度扫描
// 5 个同尺寸探测器并排，1 MeV 伽马沿各自轴线准直正入射(源距 8 cm)，
// 逐事件累计灵敏区/死层沉积能量，给出探测效率与本征全吸收(全能峰)效率。
// 用法: ./T4-4 <每点事件数>   (默认 100，仅验证可运行；正式计算建议 1e5+)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4Gamma.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "QGSP_BIC_HP.hh"
#include <cstdio>
#include <cstdlib>
#include <vector>

static const G4double kR = 2 * cm, kL = 3 * cm, kE = 1 * MeV, kGap = 2 * cm;
static const G4double kSrcZ = -8 * cm;                     // 源距探测器前表面
static const std::vector<G4double> kDead = {0., 0.1 * mm, 1. * mm, 3. * mm, 10. * mm};
static const G4int kN = (G4int)kDead.size();
static G4int g_cur = 0;                                    // 当前照射的探测器序号
static G4double g_eCry, g_eDead;                           // 本事件灵敏区/死层沉积能量
static std::vector<G4long> g_any(kN, 0), g_full(kN, 0);    // 有信号 / 全能峰 事件数

// 逐事件累计能量(含次级电子沉积)，按体积名区分灵敏区与死层
class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4double ed = step->GetTotalEnergyDeposit();
    if (ed <= 0.) return;
    const G4String n = step->GetPreStepPoint()->GetPhysicalVolume()->GetName();
    if (n == "crystal") g_eCry += ed;                      // 灵敏区(产生信号)
    else if (n == "dead") g_eDead += ed;                   // 死层(不产生信号)
  }
};

// 判选：有信号 = 探测效率；能量几乎全部沉积于灵敏区 = 全能峰(全吸收)
class EventAction : public G4UserEventAction {
public:
  void EndOfEventAction(const G4Event*) override {
    if (g_eCry + g_eDead > 0.) g_any[g_cur]++;
    if (g_eCry > 0.95 * kE && g_eDead < 0.01 * kE) g_full[g_cur]++;
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Definition());
    fGun->SetParticleEnergy(kE);
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* ev) override {
    g_eCry = g_eDead = 0.;
    fGun->SetParticlePosition(G4ThreeVector((g_cur - (kN - 1) / 2.) * kGap, 0, kSrcZ));
    fGun->GeneratePrimaryVertex(ev);
  }
};

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4NistManager* nist = G4NistManager::Instance();
    auto* ge = nist->FindOrBuildMaterial("G4_Ge");         // 死层与灵敏区同材料
    auto* vac = nist->FindOrBuildMaterial("G4_Galactic");
    auto* lvW = new G4LogicalVolume(new G4Box("world", 40 * cm, 20 * cm, 20 * cm), vac, "world");
    auto* pvW = new G4PVPlacement(nullptr, {}, lvW, "world", nullptr, false, 0);
    for (G4int i = 0; i < kN; ++i) {                        // 前死层 + 后灵敏区
      G4double d = kDead[i], cryT = kL - d, x = (i - (kN - 1) / 2.) * kGap, z0 = -kL / 2.;
      auto* crystal = new G4LogicalVolume(new G4Box("sc", kR, kR, cryT / 2.), ge, "crystal");
      new G4PVPlacement(nullptr, G4ThreeVector(x, 0, z0 + d + cryT / 2.), crystal, "crystal",
                        lvW, false, i);
      if (d > 0.) {
        auto* dl = new G4LogicalVolume(new G4Box("sd", kR, kR, d / 2.), ge, "dead");
        new G4PVPlacement(nullptr, G4ThreeVector(x, 0, z0 + d / 2.), dl, "dead", lvW, false, i);
      }
    }
    return pvW;
  }
};

int main(int argc, char** argv) {
  G4long n = (argc > 1) ? std::atol(argv[1]) : 100;   // 每点事件数
  auto* run = new G4RunManager;
  run->SetUserInitialization(new DetectorConstruction);
  run->SetUserInitialization(new QGSP_BIC_HP);
  run->SetUserAction(new PrimaryGeneratorAction);
  run->SetUserAction(new SteppingAction);
  run->SetUserAction(new EventAction);
  run->Initialize();
  for (g_cur = 0; g_cur < kN; ++g_cur) run->BeamOn(n);       // 逐个探测器照射
  std::printf("# Ge %gx%gx%g cm, %g MeV 伽马准直正入射, 源距 %g cm, 每点 %ld 事件\n",
              2 * kR / cm, 2 * kR / cm, kL / cm, kE / MeV, -kSrcZ / cm, n);
  std::printf("# %-10s %-14s %-16s %-12s\n", "死层(mm)", "探测效率(%)", "全能峰效率(%)", "峰/全谱比");
  for (G4int i = 0; i < kN; ++i) {
    G4double a = 100. * g_any[i] / n, f = 100. * g_full[i] / n;
    std::printf("  %-10.2f %-14.2f %-16.2f %-12.3f\n", kDead[i] / mm, a, f,
                g_any[i] ? (G4double)g_full[i] / g_any[i] : 0.);
  }
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

# 蒙特卡洛任务 T4-4 - C组（物理核验器干预）

## 原始需求：
> 探测器前面有一层死层，它的厚度对探测效率影响多大？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// 死层厚度对探测效率的影响：Ge 探测器前表面死层厚度扫描
// 5 个同尺寸探测器并排，1 MeV 伽马沿各自轴线准直正入射(源距 8 cm)，
// 逐事件累计灵敏区/死层沉积能量，给出探测效率与本征全吸收(全能峰)效率。
// 用法: ./T4-4 <每点事件数>   (默认 100，仅验证可运行；正式计算建议 1e5+)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4Gamma.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "QGSP_BIC_HP.hh"
#include <cstdio>
#include <cstdlib>
#include <vector>

static const G4double kR = 2 * cm, kL = 3 * cm, kE = 1 * MeV, kGap = 2 * cm;
static const G4double kSrcZ = -8 * cm;                     // 源距探测器前表面
static const std::vector<G4double> kDead = {0., 0.1 * mm, 1. * mm, 3. * mm, 10. * mm};
static const G4int kN = (G4int)kDead.size();
static G4int g_cur = 0;                                    // 当前照射的探测器序号
static G4double g_eCry, g_eDead;                           // 本事件灵敏区/死层沉积能量
static std::vector<G4long> g_any(kN, 0), g_full(kN, 0);    // 有信号 / 全能峰 事件数

// 逐事件累计能量(含次级电子沉积)，按体积名区分灵敏区与死层
class SteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4double ed = step->GetTotalEnergyDeposit();
    if (ed <= 0.) return;
    const G4String n = step->GetPreStepPoint()->GetPhysicalVolume()->GetName();
    if (n == "crystal") g_eCry += ed;                      // 灵敏区(产生信号)
    else if (n == "dead") g_eDead += ed;                   // 死层(不产生信号)
  }
};

// 判选：有信号 = 探测效率；能量几乎全部沉积于灵敏区 = 全能峰(全吸收)
class EventAction : public G4UserEventAction {
public:
  void EndOfEventAction(const G4Event*) override {
    if (g_eCry + g_eDead > 0.) g_any[g_cur]++;
    if (g_eCry > 0.95 * kE && g_eDead < 0.01 * kE) g_full[g_cur]++;
  }
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  PrimaryGeneratorAction() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Definition());
    fGun->SetParticleEnergy(kE);
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* ev) override {
    g_eCry = g_eDead = 0.;
    fGun->SetParticlePosition(G4ThreeVector((g_cur - (kN - 1) / 2.) * kGap, 0, kSrcZ));
    fGun->GeneratePrimaryVertex(ev);
  }
};

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4NistManager* nist = G4NistManager::Instance();
    auto* ge = nist->FindOrBuildMaterial("G4_Ge");         // 死层与灵敏区同材料
    auto* vac = nist->FindOrBuildMaterial("G4_Galactic");
    auto* lvW = new G4LogicalVolume(new G4Box("world", 40 * cm, 20 * cm, 20 * cm), vac, "world");
    auto* pvW = new G4PVPlacement(nullptr, {}, lvW, "world", nullptr, false, 0);
    for (G4int i = 0; i < kN; ++i) {                        // 前死层 + 后灵敏区
      G4double d = kDead[i], cryT = kL - d, x = (i - (kN - 1) / 2.) * kGap, z0 = -kL / 2.;
      auto* crystal = new G4LogicalVolume(new G4Box("sc", kR, kR, cryT / 2.), ge, "crystal");
      new G4PVPlacement(nullptr, G4ThreeVector(x, 0, z0 + d + cryT / 2.), crystal, "crystal",
                        lvW, false, i);
      if (d > 0.) {
        auto* dl = new G4LogicalVolume(new G4Box("sd", kR, kR, d / 2.), ge, "dead");
        new G4PVPlacement(nullptr, G4ThreeVector(x, 0, z0 + d / 2.), dl, "dead", lvW, false, i);
      }
    }
    return pvW;
  }
};

int main(int argc, char** argv) {
  G4long n = (argc > 1) ? std::atol(argv[1]) : 100;   // 每点事件数
  auto* run = new G4RunManager;
  run->SetUserInitialization(new DetectorConstruction);
  run->SetUserInitialization(new QGSP_BIC_HP);
  run->SetUserAction(new PrimaryGeneratorAction);
  run->SetUserAction(new SteppingAction);
  run->SetUserAction(new EventAction);
  run->Initialize();
  for (g_cur = 0; g_cur < kN; ++g_cur) run->BeamOn(n);       // 逐个探测器照射
  std::printf("# Ge %gx%gx%g cm, %g MeV 伽马准直正入射, 源距 %g cm, 每点 %ld 事件\n",
              2 * kR / cm, 2 * kR / cm, kL / cm, kE / MeV, -kSrcZ / cm, n);
  std::printf("# %-10s %-14s %-16s %-12s\n", "死层(mm)", "探测效率(%)", "全能峰效率(%)", "峰/全谱比");
  for (G4int i = 0; i < kN; ++i) {
    G4double a = 100. * g_any[i] / n, f = 100. * g_full[i] / n;
    std::printf("  %-10.2f %-14.2f %-16.2f %-12.3f\n", kDead[i] / mm, a, f,
                g_any[i] ? (G4double)g_full[i] / g_any[i] : 0.);
  }
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

