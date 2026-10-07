# T5-7 测试提示词

## A组：直接生成（原始提问）

> 10 keV 电子在组织里的极细微径迹，末端能量沉积的微观线密度分布是什么样？用 Geant4。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T5-7 - B组（通用自查重审）

## 原始需求：
> 10 keV 电子在组织里的极细微径迹，末端能量沉积的微观线密度分布是什么样？用 Geant4。

## 你此前初步生成的 Geant4 代码：
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
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "G4UserLimits.hh"
#include "G4StepLimiterPhysics.hh"
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 5*um, 5*um, 5*um), nist->FindOrBuildMaterial("G4_WATER"), "W");
    world->SetUserLimits(new G4UserLimits(50*nm)); // StepLimiter
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};
class Phys : public G4VModularPhysicsList {
public:
  Phys() { RegisterPhysics(new G4EmStandardPhysics_option4()); RegisterPhysics(new G4StepLimiterPhysics()); }
};
class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(10 * keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-2*um));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det()); rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim());
  rm->Initialize(); rm->BeamOn(argc>1?std::atoi(argv[1]):100);
  std::cout << "T5-7 Low-energy electron run completed" << std::endl;
  delete rm; return 0;
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

# 蒙特卡洛任务 T5-7 - C组（物理核验器干预）

## 原始需求：
> 10 keV 电子在组织里的极细微径迹，末端能量沉积的微观线密度分布是什么样？用 Geant4。

## 你此前初步生成的 Geant4 代码：
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
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "G4UserLimits.hh"
#include "G4StepLimiterPhysics.hh"
class Det : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* world = new G4LogicalVolume(new G4Box("W", 5*um, 5*um, 5*um), nist->FindOrBuildMaterial("G4_WATER"), "W");
    world->SetUserLimits(new G4UserLimits(50*nm)); // StepLimiter
    return new G4PVPlacement(nullptr, {}, world, "W", nullptr, false, 0);
  }
};
class Phys : public G4VModularPhysicsList {
public:
  Phys() { RegisterPhysics(new G4EmStandardPhysics_option4()); RegisterPhysics(new G4StepLimiterPhysics()); }
};
class Prim : public G4VUserPrimaryGeneratorAction {
public:
  void GeneratePrimaries(G4Event* ev) override {
    G4ParticleGun gun(1);
    gun.SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("e-"));
    gun.SetParticleEnergy(10 * keV);
    gun.SetParticlePosition(G4ThreeVector(0,0,-2*um));
    gun.SetParticleMomentumDirection(G4ThreeVector(0,0,1));
    gun.GeneratePrimaryVertex(ev);
  }
};
int main(int argc, char** argv) {
  auto* rm = new G4RunManager();
  rm->SetUserInitialization(new Det()); rm->SetUserInitialization(new Phys());
  rm->SetUserAction(new Prim());
  rm->Initialize(); rm->BeamOn(argc>1?std::atoi(argv[1]):100);
  std::cout << "T5-7 Low-energy electron run completed" << std::endl;
  delete rm; return 0;
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

