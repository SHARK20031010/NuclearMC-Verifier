# T3-3 测试提示词

## A组：直接生成（原始提问）

> 样品照射完冷却一段时间，剩下的剂量率是多少？Geant4 算一下。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T3-3 - B组（通用自查重审）

## 原始需求：
> 样品照射完冷却一段时间，剩下的剂量率是多少？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T3-3: residual dose rate of an irradiated sample after a cooling time t_cool.
// Sample = activated point source (Na-24: E_g = 1.37 MeV, T1/2 = 15 h, A0 = 1 GBq at
// the sample itself = point source at the centre).  Air kerma at R = 10 cm from the
// K = sum(L*E*mu_en/rho)/V in a shell 9.5-10.5 cm; D_dot(t) = K*A0*exp(-lam*t).
// mu_en/rho = 0.0266 cm^2/g (air, ~1 MeV).  Usage: ./T3-3 [nevents]
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4EmStandardPhysics.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const G4double kR = 10.0*cm, kT = 0.5*cm, kE = 1.37*MeV, kMu = 0.0266; // cm^2/g
static const G4double kA0 = 1.0e9;                 // activity at end of irradiation [Bq]
static const G4double kT12 = 14.997*3600.;         // half-life [s] (Na-24)
static G4double gK = 0.;                           // track-length kerma sum [MeV*cm^3/g]
class Sens : public G4VSensitiveDetector {         // track-length kerma scorer
public:
  Sens() : G4VSensitiveDetector("shell") {}
  G4bool ProcessHits(G4Step* st, G4TouchableHistory*) override {
    gK += (st->GetStepLength()/cm) * st->GetTrack()->GetKineticEnergy()/MeV * kMu;
    return true;
  }
};
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4Material* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    G4LogicalVolume* lw = new G4LogicalVolume(new G4Box("w", 30*cm, 30*cm, 30*cm), air, "world");
    G4LogicalVolume* ld = new G4LogicalVolume(new G4Sphere("sD", kR-kT, kR+kT, 0, 360*deg, 0, 180*deg), air, "doseShell");
    new G4PVPlacement(0, G4ThreeVector(), ld, "pD", lw, false, 0);
    G4VSensitiveDetector* sd = new Sens;
    G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    ld->SetSensitiveDetector(sd);
    return new G4PVPlacement(0, G4ThreeVector(), lw, "physWorld", 0, false, 0);
  }
};
class Prim : public G4VUserPrimaryGeneratorAction { // isotropic gammas from the sample
  G4ParticleGun* fGun;
public:
  Prim() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticleEnergy(kE);
    fGun->SetParticlePosition(G4ThreeVector());
  }
  void GeneratePrimaries(G4Event* ev) override {
    G4double cz = 2.*G4UniformRand() - 1., rho = std::sqrt(1. - cz*cz), ph = 2.*M_PI*G4UniformRand();
    fGun->SetParticleMomentumDirection(G4ThreeVector(rho*std::cos(ph), rho*std::sin(ph), cz));
    fGun->GeneratePrimaryVertex(ev);
  }
};
class RunAct : public G4UserRunAction {             // per cooling time: reset + report
  G4double fTc;
public:
  RunAct() : fTc(0.) {}
  void SetTcool(G4double t) { fTc = t; }
  void BeginOfRunAction(const G4Run*) override { gK = 0.; }
  void EndOfRunAction(const G4Run* run) override {
    G4double vol = (4./3.)*M_PI*(std::pow((kR+kT)/cm, 3) - std::pow((kR-kT)/cm, 3)); // cm^3
    G4double lam = std::log(2.)/kT12;
    G4double A = kA0*std::exp(-lam*fTc);                              // Bq at t_cool
    G4double kerma = gK/vol*1.602176634e-10/run->GetNumberOfEvent();  // Gy per primary
    G4double rate = kerma*A;                                          // Gy/s
    std::printf("t_cool=%10.4e s (%9.4f d)  A(t)=%9.3e Bq  D_dot=%9.3e Gy/s = %9.3f uGy/h\n",
                fTc, fTc/86400., A, rate, rate*3600./1e-6);
  }
};
int main(int argc, char** argv) {
  G4int nev = (argc > 1) ? atoi(argv[1]) : 20000;
  G4RunManager* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  G4VModularPhysicsList* pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics());
  pl->SetVerboseLevel(0);
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());       // G4RunManager API: no ActionInitialization needed
  RunAct* ra = new RunAct;
  rm->SetUserAction(ra);
  rm->Initialize();
  std::printf("--- T3-3 residual dose rate: A0=%.3g Bq, E=%.3f MeV, T1/2=%.4f h, kerma at R=%.1f+-%.1f cm, %d primaries ---\n",
              kA0, kE/MeV, kT12/3600., kR/cm, kT/cm, nev);
  const G4double tc[5] = {0., 3600., 6*3600., 86400., 7*86400.};
  for (G4int i = 0; i < 5; i++) { ra->SetTcool(tc[i]); rm->BeamOn(nev); }
  delete rm;                                   // clean shutdown (silences store warnings)
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

# 蒙特卡洛任务 T3-3 - C组（物理核验器干预）

## 原始需求：
> 样品照射完冷却一段时间，剩下的剂量率是多少？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T3-3: residual dose rate of an irradiated sample after a cooling time t_cool.
// Sample = activated point source (Na-24: E_g = 1.37 MeV, T1/2 = 15 h, A0 = 1 GBq at
// the sample itself = point source at the centre).  Air kerma at R = 10 cm from the
// K = sum(L*E*mu_en/rho)/V in a shell 9.5-10.5 cm; D_dot(t) = K*A0*exp(-lam*t).
// mu_en/rho = 0.0266 cm^2/g (air, ~1 MeV).  Usage: ./T3-3 [nevents]
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4EmStandardPhysics.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const G4double kR = 10.0*cm, kT = 0.5*cm, kE = 1.37*MeV, kMu = 0.0266; // cm^2/g
static const G4double kA0 = 1.0e9;                 // activity at end of irradiation [Bq]
static const G4double kT12 = 14.997*3600.;         // half-life [s] (Na-24)
static G4double gK = 0.;                           // track-length kerma sum [MeV*cm^3/g]
class Sens : public G4VSensitiveDetector {         // track-length kerma scorer
public:
  Sens() : G4VSensitiveDetector("shell") {}
  G4bool ProcessHits(G4Step* st, G4TouchableHistory*) override {
    gK += (st->GetStepLength()/cm) * st->GetTrack()->GetKineticEnergy()/MeV * kMu;
    return true;
  }
};
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4Material* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    G4LogicalVolume* lw = new G4LogicalVolume(new G4Box("w", 30*cm, 30*cm, 30*cm), air, "world");
    G4LogicalVolume* ld = new G4LogicalVolume(new G4Sphere("sD", kR-kT, kR+kT, 0, 360*deg, 0, 180*deg), air, "doseShell");
    new G4PVPlacement(0, G4ThreeVector(), ld, "pD", lw, false, 0);
    G4VSensitiveDetector* sd = new Sens;
    G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    ld->SetSensitiveDetector(sd);
    return new G4PVPlacement(0, G4ThreeVector(), lw, "physWorld", 0, false, 0);
  }
};
class Prim : public G4VUserPrimaryGeneratorAction { // isotropic gammas from the sample
  G4ParticleGun* fGun;
public:
  Prim() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(G4Gamma::Gamma());
    fGun->SetParticleEnergy(kE);
    fGun->SetParticlePosition(G4ThreeVector());
  }
  void GeneratePrimaries(G4Event* ev) override {
    G4double cz = 2.*G4UniformRand() - 1., rho = std::sqrt(1. - cz*cz), ph = 2.*M_PI*G4UniformRand();
    fGun->SetParticleMomentumDirection(G4ThreeVector(rho*std::cos(ph), rho*std::sin(ph), cz));
    fGun->GeneratePrimaryVertex(ev);
  }
};
class RunAct : public G4UserRunAction {             // per cooling time: reset + report
  G4double fTc;
public:
  RunAct() : fTc(0.) {}
  void SetTcool(G4double t) { fTc = t; }
  void BeginOfRunAction(const G4Run*) override { gK = 0.; }
  void EndOfRunAction(const G4Run* run) override {
    G4double vol = (4./3.)*M_PI*(std::pow((kR+kT)/cm, 3) - std::pow((kR-kT)/cm, 3)); // cm^3
    G4double lam = std::log(2.)/kT12;
    G4double A = kA0*std::exp(-lam*fTc);                              // Bq at t_cool
    G4double kerma = gK/vol*1.602176634e-10/run->GetNumberOfEvent();  // Gy per primary
    G4double rate = kerma*A;                                          // Gy/s
    std::printf("t_cool=%10.4e s (%9.4f d)  A(t)=%9.3e Bq  D_dot=%9.3e Gy/s = %9.3f uGy/h\n",
                fTc, fTc/86400., A, rate, rate*3600./1e-6);
  }
};
int main(int argc, char** argv) {
  G4int nev = (argc > 1) ? atoi(argv[1]) : 20000;
  G4RunManager* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  G4VModularPhysicsList* pl = new G4VModularPhysicsList;
  pl->RegisterPhysics(new G4EmStandardPhysics());
  pl->SetVerboseLevel(0);
  rm->SetUserInitialization(pl);
  rm->SetUserAction(new Prim());       // G4RunManager API: no ActionInitialization needed
  RunAct* ra = new RunAct;
  rm->SetUserAction(ra);
  rm->Initialize();
  std::printf("--- T3-3 residual dose rate: A0=%.3g Bq, E=%.3f MeV, T1/2=%.4f h, kerma at R=%.1f+-%.1f cm, %d primaries ---\n",
              kA0, kE/MeV, kT12/3600., kR/cm, kT/cm, nev);
  const G4double tc[5] = {0., 3600., 6*3600., 86400., 7*86400.};
  for (G4int i = 0; i < 5; i++) { ra->SetTcool(tc[i]); rm->BeamOn(nev); }
  delete rm;                                   // clean shutdown (silences store warnings)
  return 0;
}

```

## 认知护栏 / 老师傅插件诊断报告与质询：
【第一性原理认知护栏 · 静态物理守恒与测度不变性审计结果】：
🚨 状态：系统检测到当前代码存在未闭环的物理守恒律破损、相空间测度畸变或时空生命周期不变量违背。

## 第一性原理「五大守恒与不变性」纯抽象物理质询：
请依据以下不依赖任何具体经验公式或框架实现的普遍性第一性原理，逐项倒查代码中的数学建模与微观物理逻辑：

1. 【微分相空间测度不变性（几何与分布抽象）】
   - 相空间体积元守恒：空间坐标、动量方向或能谱相空间从连续物理真实向离散抽样映射时，微分微元是否在坐标变换下保持测度不变？是否存在非线性变换导致的测度畸变（如极坐标、柱坐标或曲面映射下的虚假空间聚集/发散）？

2. 【因果律与绝对时钟单调性（时间基准抽象）】
   - 物理时间单调性：飞行时间、符合时序或时间响应所依赖的物理时间标尺，是否满足全局因果单调递增律？是否存在因局部几何界面切换或输运步进截断而导致的参考系非物理原点重置？

3. 【时空产生奇点与连续输运线积分的生命周期隔离（拓扑与测度抽象）】
   - 产生点 vs 输运态：物理相互作用导致的粒子/核素创生事件，是属于时空微观反应的局部脉冲奇点（点事件），还是粒子在介质中持续滑移的自由程积分？当前计分生命周期是将单次创生奇点与后续的连续输运状态混为一谈导致了高阶多重计数，还是严格实现了产生与输运的拓扑隔离？

4. 【概率测度守恒与估计量期望无偏性（蒙特卡洛权重流抽象）】
   - 期望无偏性：在非模拟（Non-analog）输运、空间区域分裂、轮盘赌或多重方差缩减作用下，可观测量的统计累加是否维持数学期望无偏？每次相空间密度的人工调整是否在权重流上进行了严格守恒的代数补偿？

5. 【微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）】
   - 能量响应非线性：微观能量沉积转化为宏观观测信号时，是否隐含假设了完全理想的线性比例关系？高电离激发密度下的非辐射耗散、微观载流子产生的亚泊松离散相关性、以及介质微观热态/晶格散射通道是否完备闭环？

请对照上述五大第一性原理抽象质询，对代码中的数理逻辑与物理建模进行全局重审与自主修复。

## 修正要求：
请仔细阅读上述认知护栏/老师傅插件给出的检测提示与质询问题，针对报告中指出的具体可疑槽位与物理缺陷进行核实与修改。

请严格遵照以下外壳规范交付修正后的最终代码：
1. 目标平台 Geant4 11.2.2，单个 .cc 文件，自带 main()，能直接 g++ 编译运行，不依赖外部宏文件。
2. 代码尽量短：控制在 100~150 行以内，不要复杂几何。
3. 输出一份完整可编译且物理修正后的单一 .cc C++ 源码。

