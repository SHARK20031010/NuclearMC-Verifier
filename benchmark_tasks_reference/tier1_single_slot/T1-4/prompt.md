# T1-4 测试提示词

## A组：直接生成（原始提问）

> 中子加伽马的混合场，屏蔽是含硼聚乙烯外面包一层铅。漏出来的粒子能谱是什么样？用 Geant4 算。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T1-4 - B组（通用自查重审）

## 原始需求：
> 中子加伽马的混合场，屏蔽是含硼聚乙烯外面包一层铅。漏出来的粒子能谱是什么样？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T1-4  Neutron+gamma mixed field leaking through borated polyethylene (inner) + lead (outer);
// source at centre (14 MeV neutron / 1.25 MeV gamma, alternating), spectra scored at r=13 cm.
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ios.hh"
#include "QGSP_BIC_HP.hh"
static const G4int NBin = 60; static const G4double EMax = 15 * MeV;
static G4double gNE[NBin] = {0}, gNG[NBin] = {0};
static G4double gTotN = 0, gTotG = 0;
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    G4Material* pe = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
    G4Material* bpe = new G4Material("BPE", 0.95 * g / cm3, 2);
    bpe->AddMaterial(pe, 95 * perCent);
    bpe->AddElement(nist->FindOrBuildElement("B"), 5 * perCent);
    G4Material* pb = nist->FindOrBuildMaterial("G4_Pb");
    auto worldL = new G4LogicalVolume(new G4Box("World", 30 * cm, 30 * cm, 30 * cm),
                                      nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto worldP = new G4PVPlacement(0, {}, worldL, "World", 0, false, 0);
    new G4PVPlacement(0, {}, new G4LogicalVolume(
        new G4Sphere("BPE", 0, 10 * cm, 0, 360 * deg, 0, 180 * deg), bpe, "BPE"),
        "BPE", worldL, false, 0);
    new G4PVPlacement(0, {}, new G4LogicalVolume(
        new G4Sphere("Pb", 10 * cm, 13 * cm, 0, 360 * deg, 0, 180 * deg), pb, "Pb"),
        "Pb", worldL, false, 0);
    return worldP;
  }
};
class Prim : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Prim() : fGun(new G4ParticleGun(1)) {}
  void GeneratePrimaries(G4Event* ev) override {
    auto pt = G4ParticleTable::GetParticleTable();
    G4bool isN = (ev->GetEventID() % 2 == 0);
    fGun->SetParticleDefinition(pt->FindParticle(isN ? "neutron" : "gamma"));
    fGun->SetParticleEnergy(isN ? 14 * MeV : 1.25 * MeV);
    fGun->SetParticlePosition({0, 0, 0});
    fGun->SetParticleMomentumDirection({0, 0, 1});
    fGun->GeneratePrimaryVertex(ev);
  }
};
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    auto pre = st->GetPreStepPoint(); auto post = st->GetPostStepPoint();
    if (pre->GetStepStatus() != fGeomBoundary || !pre->GetPhysicalVolume()) return;
    if (pre->GetPhysicalVolume()->GetName() != "Pb") return;
    if (post->GetPhysicalVolume() && post->GetPhysicalVolume()->GetName() != "World") return;
    G4double e = pre->GetKineticEnergy();
    G4int b = (G4int)(e / EMax * NBin);
    if (b < 0 || b >= NBin) return;
    G4String p = st->GetTrack()->GetDefinition()->GetParticleName();
    if (p == "neutron") { gNE[b]++; gTotN++; }
    else if (p == "gamma") { gNG[b]++; gTotG++; }
  }
};
class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    G4cout << "\nT1-4  events=" << r->GetNumberOfEvent()
           << "   leaked neutrons=" << gTotN << "   leaked gammas=" << gTotG << G4endl;
    G4cout << "  E(MeV)   n_leak   g_leak" << G4endl;
    for (G4int i = 0; i < NBin; i++)
      if (gNE[i] + gNG[i] > 0)
        G4cout << "  " << (i + 0.5) * EMax / NBin / MeV << "   " << gNE[i] << "   " << gNG[i] << G4endl;
  }
};
int main(int argc, char** argv) {
  G4int n = (argc > 1) ? atoi(argv[1]) : 100;
  auto rm = new G4RunManager;
  rm->SetUserInitialization(new Det);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Prim);
  rm->SetUserAction(new Run);
  rm->SetUserAction(new Step);
  rm->Initialize();
  rm->BeamOn(n);
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

# 蒙特卡洛任务 T1-4 - C组（物理核验器干预）

## 原始需求：
> 中子加伽马的混合场，屏蔽是含硼聚乙烯外面包一层铅。漏出来的粒子能谱是什么样？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T1-4  Neutron+gamma mixed field leaking through borated polyethylene (inner) + lead (outer);
// source at centre (14 MeV neutron / 1.25 MeV gamma, alternating), spectra scored at r=13 cm.
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ios.hh"
#include "QGSP_BIC_HP.hh"
static const G4int NBin = 60; static const G4double EMax = 15 * MeV;
static G4double gNE[NBin] = {0}, gNG[NBin] = {0};
static G4double gTotN = 0, gTotG = 0;
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto nist = G4NistManager::Instance();
    G4Material* pe = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
    G4Material* bpe = new G4Material("BPE", 0.95 * g / cm3, 2);
    bpe->AddMaterial(pe, 95 * perCent);
    bpe->AddElement(nist->FindOrBuildElement("B"), 5 * perCent);
    G4Material* pb = nist->FindOrBuildMaterial("G4_Pb");
    auto worldL = new G4LogicalVolume(new G4Box("World", 30 * cm, 30 * cm, 30 * cm),
                                      nist->FindOrBuildMaterial("G4_Galactic"), "World");
    auto worldP = new G4PVPlacement(0, {}, worldL, "World", 0, false, 0);
    new G4PVPlacement(0, {}, new G4LogicalVolume(
        new G4Sphere("BPE", 0, 10 * cm, 0, 360 * deg, 0, 180 * deg), bpe, "BPE"),
        "BPE", worldL, false, 0);
    new G4PVPlacement(0, {}, new G4LogicalVolume(
        new G4Sphere("Pb", 10 * cm, 13 * cm, 0, 360 * deg, 0, 180 * deg), pb, "Pb"),
        "Pb", worldL, false, 0);
    return worldP;
  }
};
class Prim : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Prim() : fGun(new G4ParticleGun(1)) {}
  void GeneratePrimaries(G4Event* ev) override {
    auto pt = G4ParticleTable::GetParticleTable();
    G4bool isN = (ev->GetEventID() % 2 == 0);
    fGun->SetParticleDefinition(pt->FindParticle(isN ? "neutron" : "gamma"));
    fGun->SetParticleEnergy(isN ? 14 * MeV : 1.25 * MeV);
    fGun->SetParticlePosition({0, 0, 0});
    fGun->SetParticleMomentumDirection({0, 0, 1});
    fGun->GeneratePrimaryVertex(ev);
  }
};
class Step : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    auto pre = st->GetPreStepPoint(); auto post = st->GetPostStepPoint();
    if (pre->GetStepStatus() != fGeomBoundary || !pre->GetPhysicalVolume()) return;
    if (pre->GetPhysicalVolume()->GetName() != "Pb") return;
    if (post->GetPhysicalVolume() && post->GetPhysicalVolume()->GetName() != "World") return;
    G4double e = pre->GetKineticEnergy();
    G4int b = (G4int)(e / EMax * NBin);
    if (b < 0 || b >= NBin) return;
    G4String p = st->GetTrack()->GetDefinition()->GetParticleName();
    if (p == "neutron") { gNE[b]++; gTotN++; }
    else if (p == "gamma") { gNG[b]++; gTotG++; }
  }
};
class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    G4cout << "\nT1-4  events=" << r->GetNumberOfEvent()
           << "   leaked neutrons=" << gTotN << "   leaked gammas=" << gTotG << G4endl;
    G4cout << "  E(MeV)   n_leak   g_leak" << G4endl;
    for (G4int i = 0; i < NBin; i++)
      if (gNE[i] + gNG[i] > 0)
        G4cout << "  " << (i + 0.5) * EMax / NBin / MeV << "   " << gNE[i] << "   " << gNG[i] << G4endl;
  }
};
int main(int argc, char** argv) {
  G4int n = (argc > 1) ? atoi(argv[1]) : 100;
  auto rm = new G4RunManager;
  rm->SetUserInitialization(new Det);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Prim);
  rm->SetUserAction(new Run);
  rm->SetUserAction(new Step);
  rm->Initialize();
  rm->BeamOn(n);
  delete rm;
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

