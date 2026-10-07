# T4-3 测试提示词

## A组：直接生成（原始提问）

> 塑料闪烁体里，中子和伽马产生的光信号有什么不一样？用 Geant4 看看。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T4-3 - B组（通用自查重审）

## 原始需求：
> 塑料闪烁体里，中子和伽马产生的光信号有什么不一样？用 Geant4 看看。

## 你此前初步生成的 Geant4 代码：
```cpp
// T4-3: 塑料闪烁体 5x5x5 cm 中 1 MeV 伽马 vs 1 MeV 中子 (Geant4 11.2.2, 单文件自带 main())
// 伽马=Compton/光电子 -> 低沉积、单脉冲、0.5 ns 快峰; 中子=质子反冲+2.2 MeV 俘获伽马 -> 高沉积、宽延迟谱(~4.5 ns 峰+长尾)
// 输出: 每类 50 事件的 <Edep>/<Npe>/相对涨落/最大沉积 + 1 ns 道闪烁光子发射时刻分布
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalPhoton.hh"
#include "G4PVPlacement.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysListFactory.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserSteppingAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
static const G4int NEV = 50, NB = 20;   // 事件数 / 时间道数(1 ns)
namespace T43 {
struct Tally { G4int cur = 0, nsc[2] = {0, 0}, bin[2][NB] = {{0}}; G4double edep[2] = {0, 0}; };
thread_local Tally* tp = nullptr;
inline Tally* T() { if (!tp) tp = new Tally(); return tp; }
}
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* sc = new G4Material("PlasticScint", 1.032 * g / cm3, 2);           // 聚苯乙烯 C10H11
    sc->AddElement(nist->FindOrBuildElement("C"), 10); sc->AddElement(nist->FindOrBuildElement("H"), 11);
    const G4int N = 2; G4double en[N] = {1.0 * eV, 6.0 * eV}, rind[N] = {1.58, 1.58}, comp[N] = {1.0, 1.0};
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", en, rind, N); mpt->AddProperty("SCINTILLATIONCOMPONENT1", en, comp, N);  // 缺它则零光子
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV); mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0 * ns);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    sc->SetMaterialPropertiesTable(mpt);
    auto* world = new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("world", 20*cm, 20*cm, 20*cm),
        nist->FindOrBuildMaterial("G4_AIR"), "world"), "world", nullptr, false, 0);
    new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("sc", 2.5*cm, 2.5*cm, 2.5*cm), sc, "sc"), "sc",
        world->GetLogicalVolume(), false, 0);
    return world;
  }
};
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* tr = step->GetTrack(); auto* t = T43::T();
    if (tr->GetParticleDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) t->edep[t->cur] += step->GetTotalEnergyDeposit();
    else if (tr->GetCreatorProcess() && tr->GetCreatorProcess()->GetProcessName() == "Scintillation") {
      G4int b = (G4int)(tr->GetGlobalTime() / ns); t->nsc[t->cur]++;
      if (b >= 0 && b < NB) t->bin[t->cur][b]++;
    }
  }
};
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun gun;
public:
  Primary() : gun(1) {
    gun.SetParticleEnergy(1.0 * MeV); gun.SetParticlePosition({0, 0, -5 * cm}); gun.SetParticleMomentumDirection({0, 0, 1}); SetType(0);
  }
  void SetType(G4int k) {   // 0: 1 MeV 伽马, 1: 1 MeV 中子 (同能量, 只换粒子种类)
    T43::T()->cur = k; gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(k ? "neutron" : "gamma"));
  }
  void GeneratePrimaries(G4Event* ev) override { gun.GeneratePrimaryVertex(ev); }
};
class Action : public G4VUserActionInitialization {
public:
  void Build() const override { SetUserAction(new Primary); SetUserAction(new Stepping); }
};
int main() {
  auto* rm = new G4RunManager; rm->SetUserInitialization(new Detector);
  auto* fac = new G4PhysListFactory; auto* pl = (G4VModularPhysicsList*)fac->GetReferencePhysList("QGSP_BIC_HP");  // 含中子 HP
  pl->RegisterPhysics(new G4OpticalPhysics);                               // 闪烁发光 + 光学光子输运
  rm->SetUserInitialization(pl); rm->SetUserInitialization(new Action); rm->Initialize();
  auto* gen = (Primary*)rm->GetUserPrimaryGeneratorAction(); auto* t = T43::T();
  G4double p2[2] = {0, 0}, mx[2] = {0, 0};                                 // 光子数平方和 / 最大单事件沉积
  for (G4int k = 0; k < 2; ++k) { gen->SetType(k);
    for (G4int e = 0; e < NEV; ++e) {
      G4double e0 = t->edep[k], n0 = t->nsc[k]; rm->BeamOn(1);
      G4double ed = t->edep[k] - e0, nn = t->nsc[k] - n0; p2[k] += nn * nn; if (ed > mx[k]) mx[k] = ed;
    }
  }
  const char* tag[2] = {"gamma 1MeV", "neutron 1MeV"};
  G4cout << "\n===== 塑料闪烁体 5x5x5 cm, 各 " << NEV << " 事件 =====" << G4endl;
  for (G4int k = 0; k < 2; ++k) { G4double av = t->nsc[k] / (G4double)NEV;
    G4cout << tag[k] << ": <Edep>=" << t->edep[k]/NEV/MeV << " MeV, <Npe>=" << av << " 光子, 相对涨落="
           << std::sqrt(p2[k]/NEV - av*av)/av << ", 最大单事件沉积=" << mx[k]/MeV << " MeV" << G4endl; }
  G4cout << "\n闪烁光子发射时刻分布 (1 ns 道, 计数/事件):\n t(ns)\t" << tag[0] << "\t" << tag[1] << G4endl;
  for (G4int b = 0; b < NB; ++b)
    G4cout << " " << b + 0.5 << "\t" << t->bin[0][b]/(G4double)NEV << "\t" << t->bin[1][b]/(G4double)NEV << G4endl;
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

# 蒙特卡洛任务 T4-3 - C组（物理核验器干预）

## 原始需求：
> 塑料闪烁体里，中子和伽马产生的光信号有什么不一样？用 Geant4 看看。

## 你此前初步生成的 Geant4 代码：
```cpp
// T4-3: 塑料闪烁体 5x5x5 cm 中 1 MeV 伽马 vs 1 MeV 中子 (Geant4 11.2.2, 单文件自带 main())
// 伽马=Compton/光电子 -> 低沉积、单脉冲、0.5 ns 快峰; 中子=质子反冲+2.2 MeV 俘获伽马 -> 高沉积、宽延迟谱(~4.5 ns 峰+长尾)
// 输出: 每类 50 事件的 <Edep>/<Npe>/相对涨落/最大沉积 + 1 ns 道闪烁光子发射时刻分布
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4NistManager.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalPhoton.hh"
#include "G4PVPlacement.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4PhysListFactory.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserSteppingAction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
static const G4int NEV = 50, NB = 20;   // 事件数 / 时间道数(1 ns)
namespace T43 {
struct Tally { G4int cur = 0, nsc[2] = {0, 0}, bin[2][NB] = {{0}}; G4double edep[2] = {0, 0}; };
thread_local Tally* tp = nullptr;
inline Tally* T() { if (!tp) tp = new Tally(); return tp; }
}
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* sc = new G4Material("PlasticScint", 1.032 * g / cm3, 2);           // 聚苯乙烯 C10H11
    sc->AddElement(nist->FindOrBuildElement("C"), 10); sc->AddElement(nist->FindOrBuildElement("H"), 11);
    const G4int N = 2; G4double en[N] = {1.0 * eV, 6.0 * eV}, rind[N] = {1.58, 1.58}, comp[N] = {1.0, 1.0};
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", en, rind, N); mpt->AddProperty("SCINTILLATIONCOMPONENT1", en, comp, N);  // 缺它则零光子
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV); mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0 * ns);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    sc->SetMaterialPropertiesTable(mpt);
    auto* world = new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("world", 20*cm, 20*cm, 20*cm),
        nist->FindOrBuildMaterial("G4_AIR"), "world"), "world", nullptr, false, 0);
    new G4PVPlacement(nullptr, {}, new G4LogicalVolume(new G4Box("sc", 2.5*cm, 2.5*cm, 2.5*cm), sc, "sc"), "sc",
        world->GetLogicalVolume(), false, 0);
    return world;
  }
};
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* tr = step->GetTrack(); auto* t = T43::T();
    if (tr->GetParticleDefinition() != G4OpticalPhoton::OpticalPhotonDefinition()) t->edep[t->cur] += step->GetTotalEnergyDeposit();
    else if (tr->GetCreatorProcess() && tr->GetCreatorProcess()->GetProcessName() == "Scintillation") {
      G4int b = (G4int)(tr->GetGlobalTime() / ns); t->nsc[t->cur]++;
      if (b >= 0 && b < NB) t->bin[t->cur][b]++;
    }
  }
};
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun gun;
public:
  Primary() : gun(1) {
    gun.SetParticleEnergy(1.0 * MeV); gun.SetParticlePosition({0, 0, -5 * cm}); gun.SetParticleMomentumDirection({0, 0, 1}); SetType(0);
  }
  void SetType(G4int k) {   // 0: 1 MeV 伽马, 1: 1 MeV 中子 (同能量, 只换粒子种类)
    T43::T()->cur = k; gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle(k ? "neutron" : "gamma"));
  }
  void GeneratePrimaries(G4Event* ev) override { gun.GeneratePrimaryVertex(ev); }
};
class Action : public G4VUserActionInitialization {
public:
  void Build() const override { SetUserAction(new Primary); SetUserAction(new Stepping); }
};
int main() {
  auto* rm = new G4RunManager; rm->SetUserInitialization(new Detector);
  auto* fac = new G4PhysListFactory; auto* pl = (G4VModularPhysicsList*)fac->GetReferencePhysList("QGSP_BIC_HP");  // 含中子 HP
  pl->RegisterPhysics(new G4OpticalPhysics);                               // 闪烁发光 + 光学光子输运
  rm->SetUserInitialization(pl); rm->SetUserInitialization(new Action); rm->Initialize();
  auto* gen = (Primary*)rm->GetUserPrimaryGeneratorAction(); auto* t = T43::T();
  G4double p2[2] = {0, 0}, mx[2] = {0, 0};                                 // 光子数平方和 / 最大单事件沉积
  for (G4int k = 0; k < 2; ++k) { gen->SetType(k);
    for (G4int e = 0; e < NEV; ++e) {
      G4double e0 = t->edep[k], n0 = t->nsc[k]; rm->BeamOn(1);
      G4double ed = t->edep[k] - e0, nn = t->nsc[k] - n0; p2[k] += nn * nn; if (ed > mx[k]) mx[k] = ed;
    }
  }
  const char* tag[2] = {"gamma 1MeV", "neutron 1MeV"};
  G4cout << "\n===== 塑料闪烁体 5x5x5 cm, 各 " << NEV << " 事件 =====" << G4endl;
  for (G4int k = 0; k < 2; ++k) { G4double av = t->nsc[k] / (G4double)NEV;
    G4cout << tag[k] << ": <Edep>=" << t->edep[k]/NEV/MeV << " MeV, <Npe>=" << av << " 光子, 相对涨落="
           << std::sqrt(p2[k]/NEV - av*av)/av << ", 最大单事件沉积=" << mx[k]/MeV << " MeV" << G4endl; }
  G4cout << "\n闪烁光子发射时刻分布 (1 ns 道, 计数/事件):\n t(ns)\t" << tag[0] << "\t" << tag[1] << G4endl;
  for (G4int b = 0; b < NB; ++b)
    G4cout << " " << b + 0.5 << "\t" << t->bin[0][b]/(G4double)NEV << "\t" << t->bin[1][b]/(G4double)NEV << G4endl;
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

