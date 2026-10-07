# T2-4 测试提示词

## A组：直接生成（原始提问）

> 1 毫米厚的组织等效薄片放在伽马场里，它的剂量是多少？Geant4 算一下。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T2-4 - B组（通用自查重审）

## 原始需求：
> 1 毫米厚的组织等效薄片放在伽马场里，它的剂量是多少？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T2-4: 1 mm 厚组织等效薄片在伽马场中的吸收剂量 (Geant4 11.2.2, 单文件)
// 源: 1 MeV 伽马, 沿 +z 正入射垂直薄片; 薄片 2x2x0.1 cm 组织等效材料 (ICRU-44 近似)
// 用法: ./code [事件数] [伽马能量 MeV]; 默认 1e5 个 1 MeV 伽马
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "FTFP_BERT.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include <cstdio>
#include <cstdlib>

static G4LogicalVolume* g_slab = nullptr;
static G4double g_edep = 0.;        // 薄片内总沉积能量 (MeV)
static long long g_steps = 0;       // 薄片内产生能量沉积的 step 数

// 组织等效材料 (ICRU-44 近似: H/C/N/O 质量分数, 1.06 g/cm3)
static G4Material* MakeTissue() {
  auto* e = G4NistManager::Instance();
  auto* mat = new G4Material("TissueEquiv", 1.06 * g / cm3, 4);
  mat->AddElement(e->FindOrBuildElement("H"), 0.101);  mat->AddElement(e->FindOrBuildElement("C"), 0.111);
  mat->AddElement(e->FindOrBuildElement("N"), 0.026);  mat->AddElement(e->FindOrBuildElement("O"), 0.762);
  return mat;
}

class SlabSD : public G4VSensitiveDetector {          // 灵敏体积: 逐 step 累加薄片内沉积能量
public:
  SlabSD() : G4VSensitiveDetector("slabSD") {}
  G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override {
    G4double edep = step->GetTotalEnergyDeposit();  g_edep += edep;
    if (edep > 0.) { g_steps++; }
    return true;
  }
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override { g_edep = 0.; g_steps = 0; }
  void EndOfRunAction(const G4Run* run) override {
    G4int n = run->GetNumberOfEvent();
    if (!n) return;
    G4double mass = g_slab->GetMass();                 // GetMass 内部单位 = kg
    G4double dose = g_edep / n / mass;                 // 每入射伽马的吸收剂量 (MeV/kg -> Gy)
    printf("\n--- MC: %d 个事件, 薄片内 %lld 个沉积 step, 薄片质量 %.4e g ---\n", n, g_steps, mass / g);
    printf("沉积能量 %.6e MeV | 吸收剂量 D (每入射伽马) %.6e Gy | 总剂量 %.4e Gy\n",
           g_edep / MeV, dose / gray, dose / gray * n);
  }
};

class PrimaryAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun;
public:
  explicit PrimaryAction(G4double e) : fGun(1) {
    fGun.SetParticleDefinition(G4Gamma::Gamma());
    fGun.SetParticleEnergy(e);
    fGun.SetParticlePosition(G4ThreeVector(0, 0, -5. * cm));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));   // 正入射, 垂直薄片
  }
  void GeneratePrimaries(G4Event* ev) override { fGun.GeneratePrimaryVertex(ev); }
};

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");   // 世界介质: 空气
    auto* lvWorld = new G4LogicalVolume(new G4Box("world", 5. * cm, 5. * cm, 10. * cm), air, "world");
    auto* pvWorld = new G4PVPlacement(nullptr, {}, lvWorld, "world", nullptr, false, 0);
    // 薄片: 法向沿 z (即束流方向), 中心在原点, 厚 1 mm
    g_slab = new G4LogicalVolume(new G4Box("slab", 1. * cm, 1. * cm, 0.5 * mm), MakeTissue(), "slab");
    new G4PVPlacement(nullptr, {}, g_slab, "slab", lvWorld, false, 0);
    auto* sd = new SlabSD();  G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    g_slab->SetSensitiveDetector(sd);
    return pvWorld;
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? atoi(argv[1]) : 100000;
  G4double energy = (argc > 2) ? atof(argv[2]) * MeV : 1. * MeV;

  // 解析参考 (NIST 组织 1 MeV): mu_en/rho = 2.66e-2 cm2/g
  // 单位注量剂量 D/Phi = (mu_en/rho)*E 是"带电粒子平衡/厚样品"极限;
  // 1 mm 薄片内次级电子(射程~mm)与散射光子会逃逸, MC 结果低于该极限是正常物理效应
  G4double dPhi = 0.0266 * (energy / MeV) * 1.602e-10;   // mu_en/rho * E -> Gy 每 (伽马/cm2)
  printf("--- 解析参考 (NIST 组织): mu_en/rho = 2.66e-2 cm2/g, 1 mm 片单次穿行作用概率 %.4e ---\n",
         0.0266 * 1.06 * 0.1);
  printf("CPE 极限 D/Phi = %.4e Gy*cm2 (厚样品/带电粒子平衡; 1 mm 薄片因次级粒子逃逸会偏低)\n", dPhi);

  auto* run = new G4RunManager;                     // 几何 + 物理列表 + 源与统计
  run->SetUserInitialization(new Det);
  run->SetUserInitialization(new FTFP_BERT);
  run->SetUserAction(new PrimaryAction(energy));  run->SetUserAction(new RunAction);
  run->Initialize();  run->BeamOn(nEvents);
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

# 蒙特卡洛任务 T2-4 - C组（物理核验器干预）

## 原始需求：
> 1 毫米厚的组织等效薄片放在伽马场里，它的剂量是多少？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T2-4: 1 mm 厚组织等效薄片在伽马场中的吸收剂量 (Geant4 11.2.2, 单文件)
// 源: 1 MeV 伽马, 沿 +z 正入射垂直薄片; 薄片 2x2x0.1 cm 组织等效材料 (ICRU-44 近似)
// 用法: ./code [事件数] [伽马能量 MeV]; 默认 1e5 个 1 MeV 伽马
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "FTFP_BERT.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include <cstdio>
#include <cstdlib>

static G4LogicalVolume* g_slab = nullptr;
static G4double g_edep = 0.;        // 薄片内总沉积能量 (MeV)
static long long g_steps = 0;       // 薄片内产生能量沉积的 step 数

// 组织等效材料 (ICRU-44 近似: H/C/N/O 质量分数, 1.06 g/cm3)
static G4Material* MakeTissue() {
  auto* e = G4NistManager::Instance();
  auto* mat = new G4Material("TissueEquiv", 1.06 * g / cm3, 4);
  mat->AddElement(e->FindOrBuildElement("H"), 0.101);  mat->AddElement(e->FindOrBuildElement("C"), 0.111);
  mat->AddElement(e->FindOrBuildElement("N"), 0.026);  mat->AddElement(e->FindOrBuildElement("O"), 0.762);
  return mat;
}

class SlabSD : public G4VSensitiveDetector {          // 灵敏体积: 逐 step 累加薄片内沉积能量
public:
  SlabSD() : G4VSensitiveDetector("slabSD") {}
  G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override {
    G4double edep = step->GetTotalEnergyDeposit();  g_edep += edep;
    if (edep > 0.) { g_steps++; }
    return true;
  }
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override { g_edep = 0.; g_steps = 0; }
  void EndOfRunAction(const G4Run* run) override {
    G4int n = run->GetNumberOfEvent();
    if (!n) return;
    G4double mass = g_slab->GetMass();                 // GetMass 内部单位 = kg
    G4double dose = g_edep / n / mass;                 // 每入射伽马的吸收剂量 (MeV/kg -> Gy)
    printf("\n--- MC: %d 个事件, 薄片内 %lld 个沉积 step, 薄片质量 %.4e g ---\n", n, g_steps, mass / g);
    printf("沉积能量 %.6e MeV | 吸收剂量 D (每入射伽马) %.6e Gy | 总剂量 %.4e Gy\n",
           g_edep / MeV, dose / gray, dose / gray * n);
  }
};

class PrimaryAction : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun fGun;
public:
  explicit PrimaryAction(G4double e) : fGun(1) {
    fGun.SetParticleDefinition(G4Gamma::Gamma());
    fGun.SetParticleEnergy(e);
    fGun.SetParticlePosition(G4ThreeVector(0, 0, -5. * cm));
    fGun.SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));   // 正入射, 垂直薄片
  }
  void GeneratePrimaries(G4Event* ev) override { fGun.GeneratePrimaryVertex(ev); }
};

class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");   // 世界介质: 空气
    auto* lvWorld = new G4LogicalVolume(new G4Box("world", 5. * cm, 5. * cm, 10. * cm), air, "world");
    auto* pvWorld = new G4PVPlacement(nullptr, {}, lvWorld, "world", nullptr, false, 0);
    // 薄片: 法向沿 z (即束流方向), 中心在原点, 厚 1 mm
    g_slab = new G4LogicalVolume(new G4Box("slab", 1. * cm, 1. * cm, 0.5 * mm), MakeTissue(), "slab");
    new G4PVPlacement(nullptr, {}, g_slab, "slab", lvWorld, false, 0);
    auto* sd = new SlabSD();  G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    g_slab->SetSensitiveDetector(sd);
    return pvWorld;
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? atoi(argv[1]) : 100000;
  G4double energy = (argc > 2) ? atof(argv[2]) * MeV : 1. * MeV;

  // 解析参考 (NIST 组织 1 MeV): mu_en/rho = 2.66e-2 cm2/g
  // 单位注量剂量 D/Phi = (mu_en/rho)*E 是"带电粒子平衡/厚样品"极限;
  // 1 mm 薄片内次级电子(射程~mm)与散射光子会逃逸, MC 结果低于该极限是正常物理效应
  G4double dPhi = 0.0266 * (energy / MeV) * 1.602e-10;   // mu_en/rho * E -> Gy 每 (伽马/cm2)
  printf("--- 解析参考 (NIST 组织): mu_en/rho = 2.66e-2 cm2/g, 1 mm 片单次穿行作用概率 %.4e ---\n",
         0.0266 * 1.06 * 0.1);
  printf("CPE 极限 D/Phi = %.4e Gy*cm2 (厚样品/带电粒子平衡; 1 mm 薄片因次级粒子逃逸会偏低)\n", dPhi);

  auto* run = new G4RunManager;                     // 几何 + 物理列表 + 源与统计
  run->SetUserInitialization(new Det);
  run->SetUserInitialization(new FTFP_BERT);
  run->SetUserAction(new PrimaryAction(energy));  run->SetUserAction(new RunAction);
  run->Initialize();  run->BeamOn(nEvents);
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

