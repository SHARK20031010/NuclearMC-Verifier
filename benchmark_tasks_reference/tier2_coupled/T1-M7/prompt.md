# T1-M7 测试提示词

## A组：直接生成（原始需求）

# 任务 T1-M7 (Tier 2 中等复杂度 · T1 屏蔽与深穿透)

## 需求描述：
快中子源置于石蜡慢化体中心，外层包裹 1 mm 镉皮（Cadmium）。统计穿过石蜡的热中子能谱，以及穿过镉皮前后的中子能谱变化，验证镉对热中子的截断比。

## 核心物理观测量：
- **目标观测量**：含氢屏蔽慢化热中子在镉皮上的截止吸收效率


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



---

## C组：加核验器（前置参数确认与报错质询）

# 蒙特卡洛任务 T1-M7 - Arm C（Level 3 第一性原理抽象重审 · 含前置参数确认）

## 用户真实工程需求：
# 任务 T1-M7 (Tier 2 中等复杂度 · T1 屏蔽与深穿透)

## 需求描述：
快中子源置于石蜡慢化体中心，外层包裹 1 mm 镉皮（Cadmium）。统计穿过石蜡的热中子能谱，以及穿过镉皮前后的中子能谱变化，验证镉对热中子的截断比。

## 核心物理观测量：
- **目标观测量**：含氢屏蔽慢化热中子在镉皮上的截止吸收效率


### 交付外壳规范：
1. 目标平台 Geant4 11.2.2（本机已装，geant4-config 在 PATH 中）。
2. 单个 .cc 文件，自带 main()，能直接 g++ -O2 编译运行，不依赖外部宏文件或数据文件。
3. 代码规模建议控制在 200~350 行以内，结构严谨，具备完整的物理逻辑与数值计分。



## 前置参数确认 (Ring 0 Intent Confirmation)：
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M7 目标几何空间出射面", "src": "U"},
  "F2":  {"v": "count", "src": "U"},
  "F3":  {"v": "other", "src": "A"},
  "F4":  {"v": "surface_avg", "src": "U"},
  "F5":  {"v": "steady", "src": "A"},
  "F6":  {"v": "per_source", "src": "U"},
  "F7":  {"v": "trend", "src": "A"},
  "F8":  {"v": "other_mc", "src": "A"},
  "F9":  {"v": "N/A", "src": "U"},
  "F10": {"v": "scalar", "src": "U"},
  "warnings": ["per_source_needs_strength"]
}
```

## 你此前初步生成的 Geant4 基线代码 (Arm A 缺陷代码)：
```cpp
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

static G4long gTransNeutrons = 0;
class Det7A : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* worldSolid = new G4Box("World", 2*m, 2*m, 2*m);
    auto* worldLog = new G4LogicalVolume(worldSolid, nist->FindOrBuildMaterial("G4_AIR"), "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    auto* waxSolid = new G4Sphere("ParaffinSphere", 0, 15*cm, 0, 360*deg, 0, 180*deg);
    auto* waxLog = new G4LogicalVolume(waxSolid, nist->FindOrBuildMaterial("G4_PARAFFIN"), "ParaffinSphere");
    new G4PVPlacement(nullptr, {}, waxLog, "ParaffinSphere", worldLog, false, 0);
    return worldPV;
  }
};
class Phys7A : public G4VModularPhysicsList {
public:
  Phys7A() {
    RegisterPhysics(new G4EmStandardPhysics_option4());
    RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
  }
};
class Prim7A : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    gun.SetParticleEnergy(2.0*MeV);
    gun.SetParticlePosition(G4ThreeVector(0,0,0));
    gun.SetParticleMomentumDirection(G4RandomDirection());
    gun.GeneratePrimaryVertex(ev);
  }
};
class Step7A : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    if (s->GetPostStepPoint()->GetPosition().mag() > 20*cm) gTransNeutrons++;
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det7A()); rm->SetUserInitialization(new Phys7A());
  rm->SetUserAction(new Prim7A()); rm->SetUserAction(new Step7A());
  rm->Initialize(); rm->BeamOn(argc > 1 ? std::atoi(argv[1]) : 50);
  std::cout << "[T1-M7 Arm A] Transmitted = " << gTransNeutrons << std::endl;
  delete rm; return 0;
}

```

## 认知护栏守恒总线反馈：
【第一性原理认知护栏 · 静态物理守恒与测度不变性审计结果】：
🚨 状态：系统检测到当前代码存在未闭环的物理守恒律破损、相空间测度畸变或时空生命周期不变量违背。

## 第一性原理「守恒、不变性与意图对齐」纯抽象物理质询：
请依据以下不依赖任何具体经验公式或框架实现的普遍性第一性原理，逐项倒查代码中的数学建模与微观物理逻辑：

0. 【环 0 目标物理量与时空基准对齐（意图与规范抽象）】
   - 目标可观测量定义与量纲：所计算的量是微观能量沉积率还是宏观通量/泄漏率？几何结构是否完整还原用户需求的特定实体构型（如迷宫双弯折、分层套筒、狭缝漏束）？代码中的分母换算是否与用户真实物理目标闭环对齐？

1. 【微分相空间测度不变性（几何与分布抽象）】
   - 相空间体积元守恒：空间坐标、动量方向或能谱相空间从连续物理真实向离散抽样映射时，微分微元是否在坐标变换下保持测度不变？

2. 【因果律与绝对时钟单调性（时间基准抽象）】
   - 物理时间单调性：飞行时间、符合时序或时间响应所依赖的物理时间标尺，是否满足全局因果单调递增律？

3. 【时空产生奇点与连续输运线积分的生命周期隔离（拓扑与测度抽象）】
   - 产生点 vs 输运态：相互作用产生的次级粒子是点创生奇点还是输运线积分？计分生命周期是否混淆了单次创生与连续输运？

4. 【概率测度守恒与估计量期望无偏性（蒙特卡洛权重流抽象）】
   - 期望无偏性：在非模拟输运或空间分裂下，可观测量的统计累加是否维持数学期望无偏？

5. 【微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）】
   - 激发态通道完备性：微观能量沉积转化为信号时，强子弹性、光核反应或激发态自由度是否闭环？

## 任务要求：
1. 请对照用户真实工程需求及上述五大第一性原理抽象质询，对代码中的几何建模、物理列表与计分逻辑进行全局重构与自主修复；
2. 输出完整可独立编译的单文件 C++ 源码（包含 main()），通过 `g++ -O2 code.cc $(geant4-config --cflags --libs)` 编译且真实运行 Exit 0。可在 main 退出前调用 std::_Exit(0) 防止 Geant4 析构段错误。


