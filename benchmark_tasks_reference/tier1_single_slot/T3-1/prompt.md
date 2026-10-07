# T3-1 测试提示词

## A组：直接生成（原始提问）

> 纯钴 Co-59 样品放在中子场里照一段时间，拿出来之后 Co-60 的活度随时间怎么变？写个 Geant4 程序。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T3-1 - B组（通用自查重审）

## 原始需求：
> 纯钴 Co-59 样品放在中子场里照一段时间，拿出来之后 Co-60 的活度随时间怎么变？写个 Geant4 程序。

## 你此前初步生成的 Geant4 代码：
```cpp
// Co-59 纯钴样品放在中子场中辐照 -> 生成 Co-60 -> 停照后 Co-60 活度随时间的变化
// 编译: g++ -O2 -std=c++17 code.cc -o /tmp/T3-1 $(geant4-config --cflags) $(geant4-config --libs)
// 运行: ./T3-1
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4VProcess.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "FTFP_BERT_HP.hh"          // 含高精度中子 + 放射性衰变 (Radioactivation)
#include <cmath>
#include <cstdio>

static G4int gCap = 0;              // Co-59(n,gamma)Co-60 反应计数

// ---------- 几何: 纯钴圆柱样品, 热中子从样品前表面沿 +z 入射 ----------
class CoDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* lvW = new G4LogicalVolume(new G4Box("World", 12 * cm, 12 * cm, 12 * cm),
                                    nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto* pvW = new G4PVPlacement(nullptr, {}, lvW, "World", nullptr, false, 0);
    fR = 2.5 * cm; fT = 1.0 * cm;                       // 半径 2.5 cm, 厚 1 cm
    auto* lvCo = new G4LogicalVolume(new G4Tubs("Co", 0, fR, fT / 2, 0, 360 * deg),
                                     nist->FindOrBuildMaterial("G4_Co"), "Co");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, fT / 2), lvCo, "Co", lvW, false, 0);
    return pvW;
  }
  G4double R() const { return fR; }
  G4double T() const { return fT; }
private:
  G4double fR = 0, fT = 0;
};

class CoGun : public G4VUserPrimaryGeneratorAction {
public:
  CoGun() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(0.025 * eV);                // 热中子
    fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));  // 样品前表面
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~CoGun() override { delete fGun; }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
private:
  G4ParticleGun* fGun;
};

// 统计 Co-59(n,gamma)Co-60 的俘获次数
class CoStepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    const G4VProcess* p = s->GetPostStepPoint()->GetProcessDefinedStep();
    if (p && p->GetProcessName() == "nCapture") ++gCap;
  }
};

int main() {
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new FTFP_BERT_HP);
  auto* det = new CoDetector;
  rm->SetUserInitialization(det);
  rm->SetUserAction(new CoGun);
  rm->SetUserAction(new CoStepping);
  rm->Initialize();                                     // 必须先初始化几何/物理

  const G4int NEVT = 200;
  rm->BeamOn(NEVT);
  const G4double R = det->R(), T = det->T();
  delete rm;

  // ---------- 后处理: 活化产生 -> 停照后按 Co-60 半衰期指数衰变 ----------
  const G4double V     = M_PI * R * R * T / (cm * cm * cm);      // 样品体积 cm3
  const G4double N59   = 9.07e22 * V;                            // Co-59 原子数 (/cm3)
  const G4double sigma = 37.2e-24;                               // 热中子 (n,g) 截面 cm2
  const G4double pCap  = (G4double)gCap / NEVT;                  // 每个源中子的俘获概率
  const G4double phi   = 1e8;                                    // 中子注量率 n/cm2/s (设定)
  const G4double Rp    = phi * pCap / V;                         // 比产生率 /cm3/s
  const G4double lam   = std::log(2.0) / (5.2714 * 365.25 * 86400); // Co-60 衰变常数 /s
  const G4double Tirr  = 30 * 86400;                             // 辐照 30 天
  const G4double A0    = Rp * (1 - std::exp(-lam * Tirr)) / lam;  // 停照时刻比活度

  std::printf("\n=== Co-59(n,g)Co-60 : 活度随时间变化 ===\n");
  std::printf("样品: 纯钴圆柱 R=%.2f cm, T=%.2f cm -> V=%.3f cm3, Co-59 原子数=%.3e\n",
              R / cm, T / cm, V, N59);
  std::printf("MC (%d 热中子): nCapture=%d  ->  俘获概率 p=%.4e /源中子\n", NEVT, gCap, pCap);
  std::printf("校验: sigma*N59/V = %.4f /cm (平均自由程 %.3f cm), 薄样品近似 p~%.4f\n",
              sigma * N59 / V, 1.0 / (sigma * N59 / V), 1 - std::exp(-sigma * N59 / V * T / cm));
  std::printf("辐照: 30 d @ phi=%.1e n/cm2/s -> 停照时刻比活度 A0 = %.4e Bq/cm3\n", phi, A0);
  std::printf("%-10s %-10s %s\n", "冷却时间", "A/A0", "比活度 [Bq/cm3]");
  const G4double tc[] = {0, 1, 7, 30, 180, 365, 1825, 3650};     // 天
  for (G4double d : tc)
    std::printf("%-8.0f d  %-10.4f %.4e\n", d, std::exp(-lam * d * 86400),
                A0 * std::exp(-lam * d * 86400));
  std::printf("Co-60 T1/2 = 5.2714 a; 停照后无新产生, 纯指数衰变 A(t)=A0*exp(-lam*t)\n");
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

# 蒙特卡洛任务 T3-1 - C组（物理核验器干预）

## 原始需求：
> 纯钴 Co-59 样品放在中子场里照一段时间，拿出来之后 Co-60 的活度随时间怎么变？写个 Geant4 程序。

## 你此前初步生成的 Geant4 代码：
```cpp
// Co-59 纯钴样品放在中子场中辐照 -> 生成 Co-60 -> 停照后 Co-60 活度随时间的变化
// 编译: g++ -O2 -std=c++17 code.cc -o /tmp/T3-1 $(geant4-config --cflags) $(geant4-config --libs)
// 运行: ./T3-1
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4VProcess.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "FTFP_BERT_HP.hh"          // 含高精度中子 + 放射性衰变 (Radioactivation)
#include <cmath>
#include <cstdio>

static G4int gCap = 0;              // Co-59(n,gamma)Co-60 反应计数

// ---------- 几何: 纯钴圆柱样品, 热中子从样品前表面沿 +z 入射 ----------
class CoDetector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* lvW = new G4LogicalVolume(new G4Box("World", 12 * cm, 12 * cm, 12 * cm),
                                    nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto* pvW = new G4PVPlacement(nullptr, {}, lvW, "World", nullptr, false, 0);
    fR = 2.5 * cm; fT = 1.0 * cm;                       // 半径 2.5 cm, 厚 1 cm
    auto* lvCo = new G4LogicalVolume(new G4Tubs("Co", 0, fR, fT / 2, 0, 360 * deg),
                                     nist->FindOrBuildMaterial("G4_Co"), "Co");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, fT / 2), lvCo, "Co", lvW, false, 0);
    return pvW;
  }
  G4double R() const { return fR; }
  G4double T() const { return fT; }
private:
  G4double fR = 0, fT = 0;
};

class CoGun : public G4VUserPrimaryGeneratorAction {
public:
  CoGun() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(0.025 * eV);                // 热中子
    fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));  // 样品前表面
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~CoGun() override { delete fGun; }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
private:
  G4ParticleGun* fGun;
};

// 统计 Co-59(n,gamma)Co-60 的俘获次数
class CoStepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    const G4VProcess* p = s->GetPostStepPoint()->GetProcessDefinedStep();
    if (p && p->GetProcessName() == "nCapture") ++gCap;
  }
};

int main() {
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new FTFP_BERT_HP);
  auto* det = new CoDetector;
  rm->SetUserInitialization(det);
  rm->SetUserAction(new CoGun);
  rm->SetUserAction(new CoStepping);
  rm->Initialize();                                     // 必须先初始化几何/物理

  const G4int NEVT = 200;
  rm->BeamOn(NEVT);
  const G4double R = det->R(), T = det->T();
  delete rm;

  // ---------- 后处理: 活化产生 -> 停照后按 Co-60 半衰期指数衰变 ----------
  const G4double V     = M_PI * R * R * T / (cm * cm * cm);      // 样品体积 cm3
  const G4double N59   = 9.07e22 * V;                            // Co-59 原子数 (/cm3)
  const G4double sigma = 37.2e-24;                               // 热中子 (n,g) 截面 cm2
  const G4double pCap  = (G4double)gCap / NEVT;                  // 每个源中子的俘获概率
  const G4double phi   = 1e8;                                    // 中子注量率 n/cm2/s (设定)
  const G4double Rp    = phi * pCap / V;                         // 比产生率 /cm3/s
  const G4double lam   = std::log(2.0) / (5.2714 * 365.25 * 86400); // Co-60 衰变常数 /s
  const G4double Tirr  = 30 * 86400;                             // 辐照 30 天
  const G4double A0    = Rp * (1 - std::exp(-lam * Tirr)) / lam;  // 停照时刻比活度

  std::printf("\n=== Co-59(n,g)Co-60 : 活度随时间变化 ===\n");
  std::printf("样品: 纯钴圆柱 R=%.2f cm, T=%.2f cm -> V=%.3f cm3, Co-59 原子数=%.3e\n",
              R / cm, T / cm, V, N59);
  std::printf("MC (%d 热中子): nCapture=%d  ->  俘获概率 p=%.4e /源中子\n", NEVT, gCap, pCap);
  std::printf("校验: sigma*N59/V = %.4f /cm (平均自由程 %.3f cm), 薄样品近似 p~%.4f\n",
              sigma * N59 / V, 1.0 / (sigma * N59 / V), 1 - std::exp(-sigma * N59 / V * T / cm));
  std::printf("辐照: 30 d @ phi=%.1e n/cm2/s -> 停照时刻比活度 A0 = %.4e Bq/cm3\n", phi, A0);
  std::printf("%-10s %-10s %s\n", "冷却时间", "A/A0", "比活度 [Bq/cm3]");
  const G4double tc[] = {0, 1, 7, 30, 180, 365, 1825, 3650};     // 天
  for (G4double d : tc)
    std::printf("%-8.0f d  %-10.4f %.4e\n", d, std::exp(-lam * d * 86400),
                A0 * std::exp(-lam * d * 86400));
  std::printf("Co-60 T1/2 = 5.2714 a; 停照后无新产生, 纯指数衰变 A(t)=A0*exp(-lam*t)\n");
  return 0;
}

```

## 认知护栏 / 老师傅插件诊断报告与质询：
【认知护栏检测结果】：
✓ 物理守恒与相空间测度不变性核验通过 （自动识别核素：Co-60）。

## 修正要求：
请仔细阅读上述认知护栏/老师傅插件给出的检测提示与质询问题，针对报告中指出的具体可疑槽位与物理缺陷进行核实与修改。

请严格遵照以下外壳规范交付修正后的最终代码：
1. 目标平台 Geant4 11.2.2，单个 .cc 文件，自带 main()，能直接 g++ 编译运行，不依赖外部宏文件。
2. 代码尽量短：控制在 100~150 行以内，不要复杂几何。
3. 输出一份完整可编译且物理修正后的单一 .cc C++ 源码。

