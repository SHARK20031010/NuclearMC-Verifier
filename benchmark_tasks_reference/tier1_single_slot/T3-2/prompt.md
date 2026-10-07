# T3-2 测试提示词

## A组：直接生成（原始提问）

> 一块纯铁样品被中子照射后，会生成哪些放射性核素，各是多少？用 Geant4 算。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T3-2 - B组（通用自查重审）

## 原始需求：
> 一块纯铁样品被中子照射后，会生成哪些放射性核素，各是多少？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T3-2: 纯铁样品被 14 MeV 中子照射 -> 生成哪些放射性核素、各多少个
// g++ -O2 -std=c++17 code.cc -o /tmp/T3-2 $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Step.hh"
#include "G4VPhysicalVolume.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include <cstdio>
#include <map>
using namespace std;
// 天然铁被中子照射会生成的放射性核素(Z*1000+A), 预声明为稳定, 免得初始化时暂停等输入
static const G4int kRA[] = {26053, 26055, 25054, 25056, 27056, 27057, 27058, 27060,
                            24510, 23048, 23049, 21046, 21047, 21048};
static G4bool isRA(G4int k)
{ for (G4int r : kRA) if (r == k) return true; return false; }
class Det : public G4VUserDetectorConstruction { public:
  G4VPhysicalVolume* Construct() override
  {
    auto* fe = G4NistManager::Instance()->FindOrBuildMaterial("G4_Fe");
    auto* lv = new G4LogicalVolume(new G4Box("Fe", 3*cm, 3*cm, 3*cm), fe, "Fe");
    return new G4PVPlacement(nullptr, {}, lv, "Fe", nullptr, false, 0);   // 纯铁 6x6x6 cm
  }
};
class Phys : public G4VModularPhysicsList { public:
  Phys()
  {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4HadronElasticPhysicsHP());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());   // HP 中子输运到 20 MeV
    RegisterPhysics(new G4RadioactiveDecayPhysics());    // 产物核可继续衰变
    SetVerboseLevel(0);
  }
};
class Gun : public G4VUserPrimaryGeneratorAction { public:
  G4ParticleGun* fGun;
  Gun() : fGun(new G4ParticleGun(1))
  {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(14*MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -3*cm));   // 源在样品前表面
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
};
map<G4int, G4int> gAtoms;   // 各放射性核素生成个数, 对全部入射中子累加
// 判据: 样品内的核, 由中子作用直接产生, 或由光子蒸发/衰变过程留下的残余核
class Step : public G4UserSteppingAction { public:
  void UserSteppingAction(const G4Step* step) override
  {
    auto* tr = step->GetTrack(); auto* def = tr->GetDefinition();
    auto* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc == nullptr || tr->GetTrackID() == 1 || def->GetParticleType() != "nucleus") return;
    if (tr->GetVolume() == nullptr || tr->GetVolume()->GetName() != "Fe") return;
    G4String pn = proc->GetProcessName();
    if (pn == "neutronInelastic" || pn == "nCapture" || pn == "hadElastic" || pn == "nFission"
        || pn == "PhotoEvaporation" || pn == "RadioactiveDecay" || pn == "Decay")
      gAtoms[G4int(def->GetPDGCharge() / eplus + 0.5) * 1000 + def->GetBaryonNumber()]++;
  }
};
class Run : public G4UserRunAction { public:
  void EndOfRunAction(const G4Run* run) override
  {
    printf("\n==== 纯铁 14 MeV 中子照射: 放射性核素生成量 (入射中子 %d 个) ====\n%-8s %10s\n",
           run->GetNumberOfEvent(), "Z-A", "生成个数");
    for (auto& it : gAtoms) {
      char n[16];
      snprintf(n, sizeof n, "%d-%d", it.first / 1000, it.first % 1000);
      if (isRA(it.first) && it.second > 0) printf("%-8s %10d\n", n, it.second);
    }
  }
};
int main()
{
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Gun());
  rm->SetUserAction(new Run()); rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(100);   // 中子数, 调大可得更好统计
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

# 蒙特卡洛任务 T3-2 - C组（物理核验器干预）

## 原始需求：
> 一块纯铁样品被中子照射后，会生成哪些放射性核素，各是多少？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T3-2: 纯铁样品被 14 MeV 中子照射 -> 生成哪些放射性核素、各多少个
// g++ -O2 -std=c++17 code.cc -o /tmp/T3-2 $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManagerFactory.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Step.hh"
#include "G4VPhysicalVolume.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include <cstdio>
#include <map>
using namespace std;
// 天然铁被中子照射会生成的放射性核素(Z*1000+A), 预声明为稳定, 免得初始化时暂停等输入
static const G4int kRA[] = {26053, 26055, 25054, 25056, 27056, 27057, 27058, 27060,
                            24510, 23048, 23049, 21046, 21047, 21048};
static G4bool isRA(G4int k)
{ for (G4int r : kRA) if (r == k) return true; return false; }
class Det : public G4VUserDetectorConstruction { public:
  G4VPhysicalVolume* Construct() override
  {
    auto* fe = G4NistManager::Instance()->FindOrBuildMaterial("G4_Fe");
    auto* lv = new G4LogicalVolume(new G4Box("Fe", 3*cm, 3*cm, 3*cm), fe, "Fe");
    return new G4PVPlacement(nullptr, {}, lv, "Fe", nullptr, false, 0);   // 纯铁 6x6x6 cm
  }
};
class Phys : public G4VModularPhysicsList { public:
  Phys()
  {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4HadronElasticPhysicsHP());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());   // HP 中子输运到 20 MeV
    RegisterPhysics(new G4RadioactiveDecayPhysics());    // 产物核可继续衰变
    SetVerboseLevel(0);
  }
};
class Gun : public G4VUserPrimaryGeneratorAction { public:
  G4ParticleGun* fGun;
  Gun() : fGun(new G4ParticleGun(1))
  {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticleEnergy(14*MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -3*cm));   // 源在样品前表面
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun->GeneratePrimaryVertex(e); }
};
map<G4int, G4int> gAtoms;   // 各放射性核素生成个数, 对全部入射中子累加
// 判据: 样品内的核, 由中子作用直接产生, 或由光子蒸发/衰变过程留下的残余核
class Step : public G4UserSteppingAction { public:
  void UserSteppingAction(const G4Step* step) override
  {
    auto* tr = step->GetTrack(); auto* def = tr->GetDefinition();
    auto* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc == nullptr || tr->GetTrackID() == 1 || def->GetParticleType() != "nucleus") return;
    if (tr->GetVolume() == nullptr || tr->GetVolume()->GetName() != "Fe") return;
    G4String pn = proc->GetProcessName();
    if (pn == "neutronInelastic" || pn == "nCapture" || pn == "hadElastic" || pn == "nFission"
        || pn == "PhotoEvaporation" || pn == "RadioactiveDecay" || pn == "Decay")
      gAtoms[G4int(def->GetPDGCharge() / eplus + 0.5) * 1000 + def->GetBaryonNumber()]++;
  }
};
class Run : public G4UserRunAction { public:
  void EndOfRunAction(const G4Run* run) override
  {
    printf("\n==== 纯铁 14 MeV 中子照射: 放射性核素生成量 (入射中子 %d 个) ====\n%-8s %10s\n",
           run->GetNumberOfEvent(), "Z-A", "生成个数");
    for (auto& it : gAtoms) {
      char n[16];
      snprintf(n, sizeof n, "%d-%d", it.first / 1000, it.first % 1000);
      if (isRA(it.first) && it.second > 0) printf("%-8s %10d\n", n, it.second);
    }
  }
};
int main()
{
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly);
  rm->SetUserInitialization(new Det());
  rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Gun());
  rm->SetUserAction(new Run()); rm->SetUserAction(new Step());
  rm->Initialize();
  rm->BeamOn(100);   // 中子数, 调大可得更好统计
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

