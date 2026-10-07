# T5-1 测试提示词

## A组：直接生成（原始提问）

> 组织等效正比计数器里测到的线能谱是什么样？用 Geant4 模拟。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T5-1 - B组（通用自查重审）

## 原始需求：
> 组织等效正比计数器里测到的线能谱是什么样？用 Geant4 模拟。

## 你此前初步生成的 Geant4 代码：
```cpp
// T5-1: TEPC lineal-energy spectrum, 1 um tissue-equivalent site
// A TEPC counts single events in a ~1 um tissue-equivalent cavity. That cavity
// is far too small to be hit by a 1 MeV neutron directly, so the charged
// particles the neutron field liberates in the wall are generated instead
// (recoil protons from n-p scattering, dN/dEp flat over 0..En) and tracked.
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4NistManager.hh"
#include "G4Orb.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "QGSP_BIC_HP.hh"
#include <cmath>

static const int    kNBin = 40;   // 0-200 keV/um, 5 keV/um per bin
static const double kBinW = 5.;   // keV/um
static double fSpectrum[kNBin];
static long   fNhit = 0;
static double fSumY = 0., fEdep = 0.;   // fEdep: cavity deposit, current event

// close out the event just finished; y = E_dep / mean chord = E_dep / 1 um
static void FlushEvent() {
  if (fEdep > 0.) {
    double y = fEdep / keV;
    fNhit++;  fSumY += y;
    int ib = (int)(y / kBinW);
    if (ib >= kNBin) ib = kNBin - 1;
    fSpectrum[ib] += 1.;
  }
  fEdep = 0.;
}

// 1 um diameter tissue-equivalent sphere (the TEPC site) in bulk TE material
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4Material* te = G4NistManager::Instance()
                       ->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
    auto* lWorld = new G4LogicalVolume(new G4Orb("World", 50.*um), te, "World");
    auto* pWorld = new G4PVPlacement(nullptr, {}, lWorld, "World", nullptr, false, 0);
    auto* lCav = new G4LogicalVolume(new G4Orb("Cavity", 0.5*um), te, "Cavity");
    new G4PVPlacement(nullptr, {}, lCav, "Cavity", lWorld, false, 0);
    return pWorld;
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4VPhysicalVolume* pv = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv || pv->GetName() != "Cavity") return;
    G4double e = step->GetTotalEnergyDeposit();
    if (e > 0.) fEdep += e;
  }
};

// primary: recoil proton from n-p scattering in the wall, crossing the site
class Primary : public G4VUserPrimaryGeneratorAction {
public:
  Primary() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(
        G4ParticleTable::GetParticleTable()->FindParticle("proton"));
  }
  void GeneratePrimaries(G4Event* ev) override {
    FlushEvent();
    double th = std::acos(2.*G4UniformRand() - 1.);     // isotropic
    double ph = CLHEP::twopi * G4UniformRand();
    fGun->SetParticleEnergy(G4UniformRand()*1.*MeV + 1.*eV);  // flat 0..En
    fGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
    fGun->SetParticleMomentumDirection(
        G4ThreeVector(std::sin(th)*std::cos(ph), std::sin(th)*std::sin(ph),
                      std::cos(th)));
    fGun->GeneratePrimaryVertex(ev);
  }
private:
  G4ParticleGun* fGun;
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    fEdep = 0.; fNhit = 0; fSumY = 0.;
    for (int i = 0; i < kNBin; ++i) fSpectrum[i] = 0.;
  }
  void EndOfRunAction(const G4Run*) override {
    FlushEvent();
    G4cout << "\n# TEPC lineal-energy spectrum: 1 MeV neutron field, 1 um TE site\n"
           << "# y_mid[keV/um]   counts\n";
    for (int i = 0; i < kNBin; ++i)
      G4cout << (i + 0.5) * kBinW << "   " << fSpectrum[i] << G4endl;
    G4cout << "# hit events = " << fNhit
           << "   mean y = " << (fNhit ? fSumY / fNhit : 0.) << " keV/um" << G4endl;
  }
};

int main() {
  G4Random::setTheSeed(20240);
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new RunAction);
  rm->SetUserAction(new Stepping);
  rm->Initialize();
  G4UImanager::GetUIpointer()->ApplyCommand("/run/beamOn 100");
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

# 蒙特卡洛任务 T5-1 - C组（物理核验器干预）

## 原始需求：
> 组织等效正比计数器里测到的线能谱是什么样？用 Geant4 模拟。

## 你此前初步生成的 Geant4 代码：
```cpp
// T5-1: TEPC lineal-energy spectrum, 1 um tissue-equivalent site
// A TEPC counts single events in a ~1 um tissue-equivalent cavity. That cavity
// is far too small to be hit by a 1 MeV neutron directly, so the charged
// particles the neutron field liberates in the wall are generated instead
// (recoil protons from n-p scattering, dN/dEp flat over 0..En) and tracked.
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4NistManager.hh"
#include "G4Orb.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserRunAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "QGSP_BIC_HP.hh"
#include <cmath>

static const int    kNBin = 40;   // 0-200 keV/um, 5 keV/um per bin
static const double kBinW = 5.;   // keV/um
static double fSpectrum[kNBin];
static long   fNhit = 0;
static double fSumY = 0., fEdep = 0.;   // fEdep: cavity deposit, current event

// close out the event just finished; y = E_dep / mean chord = E_dep / 1 um
static void FlushEvent() {
  if (fEdep > 0.) {
    double y = fEdep / keV;
    fNhit++;  fSumY += y;
    int ib = (int)(y / kBinW);
    if (ib >= kNBin) ib = kNBin - 1;
    fSpectrum[ib] += 1.;
  }
  fEdep = 0.;
}

// 1 um diameter tissue-equivalent sphere (the TEPC site) in bulk TE material
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    G4Material* te = G4NistManager::Instance()
                       ->FindOrBuildMaterial("G4_TISSUE_SOFT_ICRP");
    auto* lWorld = new G4LogicalVolume(new G4Orb("World", 50.*um), te, "World");
    auto* pWorld = new G4PVPlacement(nullptr, {}, lWorld, "World", nullptr, false, 0);
    auto* lCav = new G4LogicalVolume(new G4Orb("Cavity", 0.5*um), te, "Cavity");
    new G4PVPlacement(nullptr, {}, lCav, "Cavity", lWorld, false, 0);
    return pWorld;
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    const G4VPhysicalVolume* pv = step->GetPreStepPoint()->GetPhysicalVolume();
    if (!pv || pv->GetName() != "Cavity") return;
    G4double e = step->GetTotalEnergyDeposit();
    if (e > 0.) fEdep += e;
  }
};

// primary: recoil proton from n-p scattering in the wall, crossing the site
class Primary : public G4VUserPrimaryGeneratorAction {
public:
  Primary() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(
        G4ParticleTable::GetParticleTable()->FindParticle("proton"));
  }
  void GeneratePrimaries(G4Event* ev) override {
    FlushEvent();
    double th = std::acos(2.*G4UniformRand() - 1.);     // isotropic
    double ph = CLHEP::twopi * G4UniformRand();
    fGun->SetParticleEnergy(G4UniformRand()*1.*MeV + 1.*eV);  // flat 0..En
    fGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
    fGun->SetParticleMomentumDirection(
        G4ThreeVector(std::sin(th)*std::cos(ph), std::sin(th)*std::sin(ph),
                      std::cos(th)));
    fGun->GeneratePrimaryVertex(ev);
  }
private:
  G4ParticleGun* fGun;
};

class RunAction : public G4UserRunAction {
public:
  void BeginOfRunAction(const G4Run*) override {
    fEdep = 0.; fNhit = 0; fSumY = 0.;
    for (int i = 0; i < kNBin; ++i) fSpectrum[i] = 0.;
  }
  void EndOfRunAction(const G4Run*) override {
    FlushEvent();
    G4cout << "\n# TEPC lineal-energy spectrum: 1 MeV neutron field, 1 um TE site\n"
           << "# y_mid[keV/um]   counts\n";
    for (int i = 0; i < kNBin; ++i)
      G4cout << (i + 0.5) * kBinW << "   " << fSpectrum[i] << G4endl;
    G4cout << "# hit events = " << fNhit
           << "   mean y = " << (fNhit ? fSumY / fNhit : 0.) << " keV/um" << G4endl;
  }
};

int main() {
  G4Random::setTheSeed(20240);
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(new QGSP_BIC_HP);
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new RunAction);
  rm->SetUserAction(new Stepping);
  rm->Initialize();
  G4UImanager::GetUIpointer()->ApplyCommand("/run/beamOn 100");
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

