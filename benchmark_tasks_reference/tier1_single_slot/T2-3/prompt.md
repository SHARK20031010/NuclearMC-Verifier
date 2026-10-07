# T2-3 测试提示词

## A组：直接生成（原始提问）

> Am-Be 中子源照一块组织等效材料板，算板里的吸收剂量。用 Geant4。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T2-3 - B组（通用自查重审）

## 原始需求：
> Am-Be 中子源照一块组织等效材料板，算板里的吸收剂量。用 Geant4。

## 你此前初步生成的 Geant4 代码：
```cpp
// T2-3: Am-Be neutron source irradiating a tissue-equivalent slab -> absorbed dose in the slab.
// Build: g++ -O2 -std=c++17 code.cc -o dose $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManager.hh"
#include "G4PhysListFactory.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserSteppingAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <cstdio>

static G4LogicalVolume* gSlab = nullptr;   // tissue slab
static G4double gEdep = 0.;                 // total energy deposited in the slab [MeV]

// --- geometry: 30 x 30 x 5 cm tissue-equivalent (ICRU-44 soft tissue) slab, front face at z=10 cm
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* tissue = nist->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
    auto* world = new G4Box("world", 60 * cm, 60 * cm, 60 * cm);
    auto* lworld = new G4LogicalVolume(world, air, "world");
    auto* pworld = new G4PVPlacement(nullptr, {}, lworld, "world", nullptr, false, 0);
    auto* slab = new G4Box("slab", 15 * cm, 15 * cm, 2.5 * cm);
    gSlab = new G4LogicalVolume(slab, tissue, "slab");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 12.5 * cm), gSlab, "slab", lworld, false, 0);
    return pworld;
  }
};

// --- primary: Am-Be neutron spectrum (11 x 1 MeV bins, ISO 8529-1 like), isotropic from a point at origin
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
  G4double fSpectrum[11] = {0.02, 0.10, 0.20, 0.31, 0.38, 0.34, 0.22, 0.12, 0.05, 0.015, 0.005};

public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));
  }
  void GeneratePrimaries(G4Event* ev) override {
    static CLHEP::RandGeneral sample(fSpectrum, 11, 0);   // linear interpolation over 0-11 MeV
    fGun->SetParticleEnergy(sample.shoot() * 11. * MeV);
    fGun->SetParticleMomentumDirection(G4RandomDirection());
    fGun->GeneratePrimaryVertex(ev);
  }
};

// --- score the energy deposited inside the tissue slab
class Scoring : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() == gSlab)
      gEdep += step->GetTotalEnergyDeposit();
  }
};

class Action : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Primary);
    SetUserAction(new Scoring);
  }
};

int main() {
  const G4int nEvents = 100;
  auto* runManager = new G4RunManager;
  runManager->SetUserInitialization(new Detector);
  G4PhysListFactory factory;
  runManager->SetUserInitialization(factory.GetReferencePhysList("QGSP_BIC_HP"));  // neutron HP
  runManager->SetUserInitialization(new Action);
  runManager->Initialize();
  runManager->BeamOn(nEvents);

  G4double rho = gSlab->GetMaterial()->GetDensity() / (g / cm3);   // g/cm3
  G4double mass = rho * 4500. * 1e-3;                              // kg  (30x30x5 cm3)
  G4double dose = (gEdep / MeV) * 1.602176634e-13 / mass;          // Gy
  G4cout << "\n=== Am-Be neutrons on tissue-equivalent slab (30x30x5 cm) ===" << G4endl;
  G4cout << "events                : " << nEvents << G4endl;
  G4cout << "density / mass        : " << rho << " g/cm3 / " << mass << " kg" << G4endl;
  G4cout << "energy deposited      : " << gEdep / MeV << " MeV" << G4endl;
  G4cout << "absorbed dose (total) : " << dose << " Gy" << G4endl;
  G4cout << "dose per source n     : " << dose / nEvents << " Gy/neutron" << G4endl;
  delete runManager;
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

# 蒙特卡洛任务 T2-3 - C组（物理核验器干预）

## 原始需求：
> Am-Be 中子源照一块组织等效材料板，算板里的吸收剂量。用 Geant4。

## 你此前初步生成的 Geant4 代码：
```cpp
// T2-3: Am-Be neutron source irradiating a tissue-equivalent slab -> absorbed dose in the slab.
// Build: g++ -O2 -std=c++17 code.cc -o dose $(geant4-config --cflags) $(geant4-config --libs)
#include "G4RunManager.hh"
#include "G4PhysListFactory.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserSteppingAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <cstdio>

static G4LogicalVolume* gSlab = nullptr;   // tissue slab
static G4double gEdep = 0.;                 // total energy deposited in the slab [MeV]

// --- geometry: 30 x 30 x 5 cm tissue-equivalent (ICRU-44 soft tissue) slab, front face at z=10 cm
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* tissue = nist->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
    auto* world = new G4Box("world", 60 * cm, 60 * cm, 60 * cm);
    auto* lworld = new G4LogicalVolume(world, air, "world");
    auto* pworld = new G4PVPlacement(nullptr, {}, lworld, "world", nullptr, false, 0);
    auto* slab = new G4Box("slab", 15 * cm, 15 * cm, 2.5 * cm);
    gSlab = new G4LogicalVolume(slab, tissue, "slab");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 12.5 * cm), gSlab, "slab", lworld, false, 0);
    return pworld;
  }
};

// --- primary: Am-Be neutron spectrum (11 x 1 MeV bins, ISO 8529-1 like), isotropic from a point at origin
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
  G4double fSpectrum[11] = {0.02, 0.10, 0.20, 0.31, 0.38, 0.34, 0.22, 0.12, 0.05, 0.015, 0.005};

public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticlePosition(G4ThreeVector(0, 0, 0));
  }
  void GeneratePrimaries(G4Event* ev) override {
    static CLHEP::RandGeneral sample(fSpectrum, 11, 0);   // linear interpolation over 0-11 MeV
    fGun->SetParticleEnergy(sample.shoot() * 11. * MeV);
    fGun->SetParticleMomentumDirection(G4RandomDirection());
    fGun->GeneratePrimaryVertex(ev);
  }
};

// --- score the energy deposited inside the tissue slab
class Scoring : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    if (step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() == gSlab)
      gEdep += step->GetTotalEnergyDeposit();
  }
};

class Action : public G4VUserActionInitialization {
public:
  void Build() const override {
    SetUserAction(new Primary);
    SetUserAction(new Scoring);
  }
};

int main() {
  const G4int nEvents = 100;
  auto* runManager = new G4RunManager;
  runManager->SetUserInitialization(new Detector);
  G4PhysListFactory factory;
  runManager->SetUserInitialization(factory.GetReferencePhysList("QGSP_BIC_HP"));  // neutron HP
  runManager->SetUserInitialization(new Action);
  runManager->Initialize();
  runManager->BeamOn(nEvents);

  G4double rho = gSlab->GetMaterial()->GetDensity() / (g / cm3);   // g/cm3
  G4double mass = rho * 4500. * 1e-3;                              // kg  (30x30x5 cm3)
  G4double dose = (gEdep / MeV) * 1.602176634e-13 / mass;          // Gy
  G4cout << "\n=== Am-Be neutrons on tissue-equivalent slab (30x30x5 cm) ===" << G4endl;
  G4cout << "events                : " << nEvents << G4endl;
  G4cout << "density / mass        : " << rho << " g/cm3 / " << mass << " kg" << G4endl;
  G4cout << "energy deposited      : " << gEdep / MeV << " MeV" << G4endl;
  G4cout << "absorbed dose (total) : " << dose << " Gy" << G4endl;
  G4cout << "dose per source n     : " << dose / nEvents << " Gy/neutron" << G4endl;
  delete runManager;
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

