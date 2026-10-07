# T1-3 测试提示词

## A组：直接生成（原始提问）

> 屏蔽体是铁 5 厘米，后面再接水 10 厘米。伽马穿过这两层之后还剩多少剂量？Geant4 算一下。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T1-3 - B组（通用自查重审）

## 原始需求：
> 屏蔽体是铁 5 厘米，后面再接水 10 厘米。伽马穿过这两层之后还剩多少剂量？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T1-3  5 cm 铁 + 10 cm 水，1 MeV 伽马正入射 -> 水层背面出口剂量
// 剂量用水层背面紧贴的 1 mm 水薄层(计分层)沉积能量算出
// 编译 g++ -O2 -std=c++17 code.cc -o t13 $(geant4-config --cflags) $(geant4-config --libs)
// 运行 ./t13 [源粒子数, 默认 1e6]
#include <cstdlib>
#include "G4RunManagerFactory.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4UserSteppingAction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "G4VPhysicalVolume.hh"

static G4double gEdep = 0.;                // 计分层沉积能量 (MeV)
static G4double gMass = 0.;                // 计分层质量 (kg)
static const G4double kScorer = 1.0 * mm;  // 计分层厚度
// 几何(z 为束流方向, 20x20 cm^2)：z=0~5 铁; 5~15 水; 15~15.1 cm 1 mm 水计分层
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* fe = nist->FindOrBuildMaterial("G4_Fe");
    auto* h2o = nist->FindOrBuildMaterial("G4_WATER");
    const G4double hx = 10. * cm;   // 横向半宽 -> 20 x 20 cm^2

    auto* worldL = new G4LogicalVolume(new G4Box("World", hx + 5 * cm, hx + 5 * cm, 20 * cm), air, "World");
    auto* worldP = new G4PVPlacement(nullptr, {}, worldL, "World", nullptr, false, 0);

    auto* feL = new G4LogicalVolume(new G4Box("Fe", hx, hx, 2.5 * cm), fe, "Fe");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 2.5 * cm), feL, "Fe", worldL, false, 0);   // z=0~5 cm

    auto* wL = new G4LogicalVolume(new G4Box("Water", hx, hx, 5. * cm), h2o, "Water");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 10. * cm), wL, "Water", worldL, false, 0); // z=5~15 cm

    auto* scL = new G4LogicalVolume(new G4Box("Scorer", hx, hx, 0.5 * kScorer), h2o, "Scorer");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 15. * cm + 0.5 * kScorer), scL, "Scorer", worldL, false, 0);

    gMass = scL->GetMass() / kg;   // Geant4 质量单位 -> kg
    return worldP;
  }
};

class Physics : public G4VModularPhysicsList {   // 1 MeV 伽马只需电磁过程
public:
  Physics() { RegisterPhysics(new G4EmStandardPhysics()); }
  void SetCuts() override {
    SetDefaultCutValue(0.1 * mm);                // 薄层能量沉积需小 production cut
    SetCutsWithDefault();
  }
};
class Gun : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Gun() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    fGun->SetParticleEnergy(1.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -1.0 * cm));   // 空气中正对铁层
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~Gun() override { delete fGun; }
  void GeneratePrimaries(G4Event* evt) override { fGun->GeneratePrimaryVertex(evt); }
};
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetPhysicalVolume()->GetName() == "Scorer")
      gEdep += step->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  const G4int n = (argc > 1) ? std::atoi(argv[1]) : 1000000;
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new Detector());
  rm->SetUserInitialization(new Physics());
  rm->SetUserAction(new Gun());
  rm->SetUserAction(new Stepping());
  rm->Initialize();
  rm->BeamOn(n);

  const G4double edepPerPrimary = gEdep / n;                                  // MeV
  const G4double dosePerPrimary = edepPerPrimary * (1.602176634e-13 / gMass); // MeV->J, /kg -> Gy
  G4cout << "\n===== T1-3  5 cm 铁 + 10 cm 水, 1 MeV 伽马正入射 =====\n"
         << "源粒子数                  : " << n << "\n"
         << "计分层(1 mm 水)质量       : " << gMass << " kg\n"
         << "出口层沉积能量 / 源粒子   : " << edepPerPrimary << " MeV\n"
         << "水层背面出口剂量 / 源粒子 : " << dosePerPrimary << " Gy\n"
         << "（剂量与源粒子数成正比, 可线性缩放）\n" << G4endl;
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

# 蒙特卡洛任务 T1-3 - C组（物理核验器干预）

## 原始需求：
> 屏蔽体是铁 5 厘米，后面再接水 10 厘米。伽马穿过这两层之后还剩多少剂量？Geant4 算一下。

## 你此前初步生成的 Geant4 代码：
```cpp
// T1-3  5 cm 铁 + 10 cm 水，1 MeV 伽马正入射 -> 水层背面出口剂量
// 剂量用水层背面紧贴的 1 mm 水薄层(计分层)沉积能量算出
// 编译 g++ -O2 -std=c++17 code.cc -o t13 $(geant4-config --cflags) $(geant4-config --libs)
// 运行 ./t13 [源粒子数, 默认 1e6]
#include <cstdlib>
#include "G4RunManagerFactory.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4UserSteppingAction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Step.hh"
#include "G4Event.hh"
#include "G4VPhysicalVolume.hh"

static G4double gEdep = 0.;                // 计分层沉积能量 (MeV)
static G4double gMass = 0.;                // 计分层质量 (kg)
static const G4double kScorer = 1.0 * mm;  // 计分层厚度
// 几何(z 为束流方向, 20x20 cm^2)：z=0~5 铁; 5~15 水; 15~15.1 cm 1 mm 水计分层
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* fe = nist->FindOrBuildMaterial("G4_Fe");
    auto* h2o = nist->FindOrBuildMaterial("G4_WATER");
    const G4double hx = 10. * cm;   // 横向半宽 -> 20 x 20 cm^2

    auto* worldL = new G4LogicalVolume(new G4Box("World", hx + 5 * cm, hx + 5 * cm, 20 * cm), air, "World");
    auto* worldP = new G4PVPlacement(nullptr, {}, worldL, "World", nullptr, false, 0);

    auto* feL = new G4LogicalVolume(new G4Box("Fe", hx, hx, 2.5 * cm), fe, "Fe");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 2.5 * cm), feL, "Fe", worldL, false, 0);   // z=0~5 cm

    auto* wL = new G4LogicalVolume(new G4Box("Water", hx, hx, 5. * cm), h2o, "Water");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 10. * cm), wL, "Water", worldL, false, 0); // z=5~15 cm

    auto* scL = new G4LogicalVolume(new G4Box("Scorer", hx, hx, 0.5 * kScorer), h2o, "Scorer");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 15. * cm + 0.5 * kScorer), scL, "Scorer", worldL, false, 0);

    gMass = scL->GetMass() / kg;   // Geant4 质量单位 -> kg
    return worldP;
  }
};

class Physics : public G4VModularPhysicsList {   // 1 MeV 伽马只需电磁过程
public:
  Physics() { RegisterPhysics(new G4EmStandardPhysics()); }
  void SetCuts() override {
    SetDefaultCutValue(0.1 * mm);                // 薄层能量沉积需小 production cut
    SetCutsWithDefault();
  }
};
class Gun : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Gun() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("gamma"));
    fGun->SetParticleEnergy(1.0 * MeV);
    fGun->SetParticlePosition(G4ThreeVector(0, 0, -1.0 * cm));   // 空气中正对铁层
    fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  }
  ~Gun() override { delete fGun; }
  void GeneratePrimaries(G4Event* evt) override { fGun->GeneratePrimaryVertex(evt); }
};
class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetPhysicalVolume()->GetName() == "Scorer")
      gEdep += step->GetTotalEnergyDeposit();
  }
};
int main(int argc, char** argv) {
  const G4int n = (argc > 1) ? std::atoi(argv[1]) : 1000000;
  auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial);
  rm->SetUserInitialization(new Detector());
  rm->SetUserInitialization(new Physics());
  rm->SetUserAction(new Gun());
  rm->SetUserAction(new Stepping());
  rm->Initialize();
  rm->BeamOn(n);

  const G4double edepPerPrimary = gEdep / n;                                  // MeV
  const G4double dosePerPrimary = edepPerPrimary * (1.602176634e-13 / gMass); // MeV->J, /kg -> Gy
  G4cout << "\n===== T1-3  5 cm 铁 + 10 cm 水, 1 MeV 伽马正入射 =====\n"
         << "源粒子数                  : " << n << "\n"
         << "计分层(1 mm 水)质量       : " << gMass << " kg\n"
         << "出口层沉积能量 / 源粒子   : " << edepPerPrimary << " MeV\n"
         << "水层背面出口剂量 / 源粒子 : " << dosePerPrimary << " Gy\n"
         << "（剂量与源粒子数成正比, 可线性缩放）\n" << G4endl;
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

