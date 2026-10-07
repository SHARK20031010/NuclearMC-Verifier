# T6-2 测试提示词

## A组：直接生成（原始提问）

> 质子束打厚靶，中子的产额和角分布是多少？用 Geant4 算。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T6-2 - B组（通用自查重审）

## 原始需求：
> 质子束打厚靶，中子的产额和角分布是多少？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T6-2: 质子束打厚靶 -> 中子产额与角分布 (Geant4 11.2.2, 单文件)
#include "G4RunManagerFactory.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4PVPlacement.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "QGSP_BIC_HP.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserRunAction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ios.hh"
#include <set>
static const G4int NB = 12;                  // 角度分箱: 0-180 deg, 15 deg/箱
static const G4double R0 = 50*cm;            // 记录球面半径
static G4double gYield = 0., gAng[NB] = {0};
static std::set<G4int> gSeen;
// 中子每次穿过半径 R0 球面记一次, 按出射极角分箱
class StepAct : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    G4Track* t = s->GetTrack();
    if (t->GetDefinition()->GetParticleName() != "neutron") return;
    if (t->GetPosition().mag() < R0) return;               // 尚未到达球面
    if (!gSeen.insert(t->GetTrackID()).second) return;     // 同一中子只记一次
    G4int b = (G4int)(t->GetMomentumDirection().theta()/deg/15.);
    gYield += 1.; gAng[b > NB-1 ? NB-1 : b] += 1.;
  }
};
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* W = nist->FindOrBuildMaterial("G4_W");          // 钨靶, 19.3 g/cm3
    auto* world = new G4LogicalVolume(new G4Box("world", 60*cm, 60*cm, 60*cm),
                                      nist->FindOrBuildMaterial("G4_AIR"), "world");
    auto* pw = new G4PVPlacement(0, G4ThreeVector(), world, "world", 0, false, 0);
    auto* wl = new G4LogicalVolume(new G4Tubs("target", 0, 5*cm, 10*cm, 0, 360*deg), W, "target");
    new G4PVPlacement(0, G4ThreeVector(), wl, "target", world, false, 0);  // 厚 20 cm
    return pw;
  }
};
class Gun : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun{1};
public:
  Gun() {
    fGun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    fGun.SetParticleEnergy(1*GeV);                        // 质子动能
    fGun.SetParticlePosition(G4ThreeVector(0, 0, -12*cm));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun.GeneratePrimaryVertex(e); }
};
class RunAct : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    gYield = 0.; gSeen.clear();
    for (G4int i = 0; i < NB; i++) gAng[i] = 0.;
  }
  void EndOfRunAction(const G4Run* r) override {
    G4double tot = 0.; for (G4int i = 0; i < NB; i++) tot += gAng[i];
    G4cout << "\n==== T6-2 : 1 GeV 质子轰击 20 cm 厚钨靶 ====" << G4endl;
    G4cout << "初级质子数 = " << r->GetNumberOfEvent() << G4endl;
    G4cout << "中子产额 = " << gYield << "  (每质子 " << gYield/r->GetNumberOfEvent()
           << " 个中子)" << G4endl;
    G4cout << " 角度区间[deg]   中子数     归一化角分布" << G4endl;
    for (G4int i = 0; i < NB; i++)
      G4cout << "  " << i*15 << "-" << (i+1)*15 << "\t\t" << gAng[i] << "\t"
             << (tot > 0. ? gAng[i]/tot : 0.) << G4endl;
  }
};
class ActInit : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Gun); SetUserAction(new RunAct); SetUserAction(new StepAct);
  }
};
int main() {
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new Det);
  rm->SetUserInitialization(new QGSP_BIC_HP);   // Bertini 级联 + 高精度中子输运
  rm->SetUserInitialization(new ActInit);
  rm->Initialize();
  rm->BeamOn(100);                              // 验证用 100 个质子; 正式统计请加大
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

# 蒙特卡洛任务 T6-2 - C组（物理核验器干预）

## 原始需求：
> 质子束打厚靶，中子的产额和角分布是多少？用 Geant4 算。

## 你此前初步生成的 Geant4 代码：
```cpp
// T6-2: 质子束打厚靶 -> 中子产额与角分布 (Geant4 11.2.2, 单文件)
#include "G4RunManagerFactory.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4PVPlacement.hh"
#include "G4LogicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "QGSP_BIC_HP.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserRunAction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Run.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ios.hh"
#include <set>
static const G4int NB = 12;                  // 角度分箱: 0-180 deg, 15 deg/箱
static const G4double R0 = 50*cm;            // 记录球面半径
static G4double gYield = 0., gAng[NB] = {0};
static std::set<G4int> gSeen;
// 中子每次穿过半径 R0 球面记一次, 按出射极角分箱
class StepAct : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* s) override {
    G4Track* t = s->GetTrack();
    if (t->GetDefinition()->GetParticleName() != "neutron") return;
    if (t->GetPosition().mag() < R0) return;               // 尚未到达球面
    if (!gSeen.insert(t->GetTrackID()).second) return;     // 同一中子只记一次
    G4int b = (G4int)(t->GetMomentumDirection().theta()/deg/15.);
    gYield += 1.; gAng[b > NB-1 ? NB-1 : b] += 1.;
  }
};
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* W = nist->FindOrBuildMaterial("G4_W");          // 钨靶, 19.3 g/cm3
    auto* world = new G4LogicalVolume(new G4Box("world", 60*cm, 60*cm, 60*cm),
                                      nist->FindOrBuildMaterial("G4_AIR"), "world");
    auto* pw = new G4PVPlacement(0, G4ThreeVector(), world, "world", 0, false, 0);
    auto* wl = new G4LogicalVolume(new G4Tubs("target", 0, 5*cm, 10*cm, 0, 360*deg), W, "target");
    new G4PVPlacement(0, G4ThreeVector(), wl, "target", world, false, 0);  // 厚 20 cm
    return pw;
  }
};
class Gun : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun{1};
public:
  Gun() {
    fGun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("proton"));
    fGun.SetParticleEnergy(1*GeV);                        // 质子动能
    fGun.SetParticlePosition(G4ThreeVector(0, 0, -12*cm));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  void GeneratePrimaries(G4Event* e) override { fGun.GeneratePrimaryVertex(e); }
};
class RunAct : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    gYield = 0.; gSeen.clear();
    for (G4int i = 0; i < NB; i++) gAng[i] = 0.;
  }
  void EndOfRunAction(const G4Run* r) override {
    G4double tot = 0.; for (G4int i = 0; i < NB; i++) tot += gAng[i];
    G4cout << "\n==== T6-2 : 1 GeV 质子轰击 20 cm 厚钨靶 ====" << G4endl;
    G4cout << "初级质子数 = " << r->GetNumberOfEvent() << G4endl;
    G4cout << "中子产额 = " << gYield << "  (每质子 " << gYield/r->GetNumberOfEvent()
           << " 个中子)" << G4endl;
    G4cout << " 角度区间[deg]   中子数     归一化角分布" << G4endl;
    for (G4int i = 0; i < NB; i++)
      G4cout << "  " << i*15 << "-" << (i+1)*15 << "\t\t" << gAng[i] << "\t"
             << (tot > 0. ? gAng[i]/tot : 0.) << G4endl;
  }
};
class ActInit : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Gun); SetUserAction(new RunAct); SetUserAction(new StepAct);
  }
};
int main() {
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new Det);
  rm->SetUserInitialization(new QGSP_BIC_HP);   // Bertini 级联 + 高精度中子输运
  rm->SetUserInitialization(new ActInit);
  rm->Initialize();
  rm->BeamOn(100);                              // 验证用 100 个质子; 正式统计请加大
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

