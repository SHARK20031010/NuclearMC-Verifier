# T2-1 测试提示词

## A组：直接生成（原始提问）

> Cs-137 点源，活度 1 毫居里，离它 1 米远的空气里吸收剂量率是多少？写个 Geant4 程序。

---

## B组：模型自查（让模型自己复核）

# 蒙特卡洛任务 T2-1 - B组（通用自查重审）

## 原始需求：
> Cs-137 点源，活度 1 毫居里，离它 1 米远的空气里吸收剂量率是多少？写个 Geant4 程序。

## 你此前初步生成的 Geant4 代码：
```cpp
// Cs-137 point source (1 mCi) -> absorbed dose rate in air at 1 m.
// Method: track-length fluence estimator in a 10 cm thick air shell centred on
// r = 1 m, converted to air kerma by (mu_en/rho) = 0.0293 cm2/g at 662 keV.
// (Secondary electrons range ~1 m in air, so local edep is not usable here.)
// build: g++ -O2 -std=c++17 code.cc -o run $(geant4-config --cflags) $(geant4-config --libs)
// run:   ./run [nEvents]     (default 10000; the estimator converges within ~1e3)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserPhysicsList.hh"
#include "G4VModularPhysicsList.hh"
#include "G4PhysListFactory.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4Run.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ios.hh"
#include <cstdlib>
#include <cmath>

static G4LogicalVolume* gShell = nullptr;  // air shell around r = 1 m
static G4double gVol = 0.;                 // shell volume
static G4double gTE = 0., gLen = 0.;       // sum of tracklength*energy, and tracklength
static G4long gEdep = 0;                   // energy-deposit steps in shell (diagnostic)

// world = air ; scoring shell = air shell from r = 95 cm to r = 105 cm
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    auto world = new G4LogicalVolume(new G4Box("World", 1.5*m, 1.5*m, 1.5*m), air, "World");
    auto wp = new G4PVPlacement(nullptr, G4ThreeVector(), world, "World", nullptr, false, 0);
    gShell = new G4LogicalVolume(new G4Sphere("Shell", 95.*cm, 105.*cm, 0., twopi, 0., pi), air, "Shell");
    new G4PVPlacement(nullptr, G4ThreeVector(), gShell, "Shell", world, false, 0);
    gVol = gShell->GetSolid()->GetCubicVolume();
    return wp;
  }
};

// isotropic 661.657 keV gamma (Ba-137m) from a point source at the origin
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Definition());
    fGun->SetParticleEnergy(661.657*keV);
    fGun->SetParticlePosition(G4ThreeVector());
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    G4double c = 2.*G4UniformRand() - 1., p = twopi*G4UniformRand(), s = std::sqrt(1. - c*c);
    fGun->SetParticleMomentumDirection(G4ThreeVector(s*std::cos(p), s*std::sin(p), c));
    fGun->GeneratePrimaryVertex(ev);
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    if (st->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() != gShell) return;
    if (st->GetTrack()->GetDefinition() != G4Gamma::Definition()) return;
    gTE += st->GetStepLength() * st->GetPreStepPoint()->GetKineticEnergy();
    gLen += st->GetStepLength();
    if (st->GetTotalEnergyDeposit() > 0.) gEdep++;
  }
};

class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    G4double n = r->GetNumberOfEvent();
    G4double psi = (gTE/n)/gVol/(MeV/cm2);      // energy fluence per gamma, MeV/cm2
    // air kerma = psi * (mu_en/rho) ;  1 MeV/g = 1.602176634e-10 Gy
    G4double gyPerGamma = psi * 0.0293 * 1.602176634e-10;
    G4double rate = gyPerGamma * 3.7e7 * 0.851; // Gy/s for 1 mCi (85.1% per decay)
    G4cout << "\n=== Cs-137 1 mCi point source: absorbed dose rate in air at 1 m ===\n"
           << "  primaries             : " << n << "\n"
           << "  energy fluence at 1 m : " << psi << " MeV/cm2 per gamma\n"
           << "  air kerma (662 keV)   : " << gyPerGamma << " Gy per gamma\n"
           << "  dose rate             : " << rate*1.e6 << " uGy/s = "
           << rate*3.6e9 << " uGy/h\n"
           << "  mean chord in shell   : " << (gLen/n)/cm << " cm   (edep steps " << gEdep << ")\n";
  }
};

int main(int argc, char** argv) {
  G4int n = (argc > 1) ? std::atoi(argv[1]) : 10000;
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(G4PhysListFactory().GetReferencePhysList("QGSP_BIC_HP"));
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new Run);
  rm->SetUserAction(new Stepping);
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

# 蒙特卡洛任务 T2-1 - C组（物理核验器干预）

## 原始需求：
> Cs-137 点源，活度 1 毫居里，离它 1 米远的空气里吸收剂量率是多少？写个 Geant4 程序。

## 你此前初步生成的 Geant4 代码：
```cpp
// Cs-137 point source (1 mCi) -> absorbed dose rate in air at 1 m.
// Method: track-length fluence estimator in a 10 cm thick air shell centred on
// r = 1 m, converted to air kerma by (mu_en/rho) = 0.0293 cm2/g at 662 keV.
// (Secondary electrons range ~1 m in air, so local edep is not usable here.)
// build: g++ -O2 -std=c++17 code.cc -o run $(geant4-config --cflags) $(geant4-config --libs)
// run:   ./run [nEvents]     (default 10000; the estimator converges within ~1e3)
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserPhysicsList.hh"
#include "G4VModularPhysicsList.hh"
#include "G4PhysListFactory.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserRunAction.hh"
#include "G4ParticleGun.hh"
#include "G4Gamma.hh"
#include "G4Run.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ios.hh"
#include <cstdlib>
#include <cmath>

static G4LogicalVolume* gShell = nullptr;  // air shell around r = 1 m
static G4double gVol = 0.;                 // shell volume
static G4double gTE = 0., gLen = 0.;       // sum of tracklength*energy, and tracklength
static G4long gEdep = 0;                   // energy-deposit steps in shell (diagnostic)

// world = air ; scoring shell = air shell from r = 95 cm to r = 105 cm
class Detector : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto air = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    auto world = new G4LogicalVolume(new G4Box("World", 1.5*m, 1.5*m, 1.5*m), air, "World");
    auto wp = new G4PVPlacement(nullptr, G4ThreeVector(), world, "World", nullptr, false, 0);
    gShell = new G4LogicalVolume(new G4Sphere("Shell", 95.*cm, 105.*cm, 0., twopi, 0., pi), air, "Shell");
    new G4PVPlacement(nullptr, G4ThreeVector(), gShell, "Shell", world, false, 0);
    gVol = gShell->GetSolid()->GetCubicVolume();
    return wp;
  }
};

// isotropic 661.657 keV gamma (Ba-137m) from a point source at the origin
class Primary : public G4VUserPrimaryGeneratorAction {
  G4ParticleGun* fGun;
public:
  Primary() : fGun(new G4ParticleGun(1)) {
    fGun->SetParticleDefinition(G4Gamma::Definition());
    fGun->SetParticleEnergy(661.657*keV);
    fGun->SetParticlePosition(G4ThreeVector());
  }
  ~Primary() override { delete fGun; }
  void GeneratePrimaries(G4Event* ev) override {
    G4double c = 2.*G4UniformRand() - 1., p = twopi*G4UniformRand(), s = std::sqrt(1. - c*c);
    fGun->SetParticleMomentumDirection(G4ThreeVector(s*std::cos(p), s*std::sin(p), c));
    fGun->GeneratePrimaryVertex(ev);
  }
};

class Stepping : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* st) override {
    if (st->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume() != gShell) return;
    if (st->GetTrack()->GetDefinition() != G4Gamma::Definition()) return;
    gTE += st->GetStepLength() * st->GetPreStepPoint()->GetKineticEnergy();
    gLen += st->GetStepLength();
    if (st->GetTotalEnergyDeposit() > 0.) gEdep++;
  }
};

class Run : public G4UserRunAction {
public:
  void EndOfRunAction(const G4Run* r) override {
    G4double n = r->GetNumberOfEvent();
    G4double psi = (gTE/n)/gVol/(MeV/cm2);      // energy fluence per gamma, MeV/cm2
    // air kerma = psi * (mu_en/rho) ;  1 MeV/g = 1.602176634e-10 Gy
    G4double gyPerGamma = psi * 0.0293 * 1.602176634e-10;
    G4double rate = gyPerGamma * 3.7e7 * 0.851; // Gy/s for 1 mCi (85.1% per decay)
    G4cout << "\n=== Cs-137 1 mCi point source: absorbed dose rate in air at 1 m ===\n"
           << "  primaries             : " << n << "\n"
           << "  energy fluence at 1 m : " << psi << " MeV/cm2 per gamma\n"
           << "  air kerma (662 keV)   : " << gyPerGamma << " Gy per gamma\n"
           << "  dose rate             : " << rate*1.e6 << " uGy/s = "
           << rate*3.6e9 << " uGy/h\n"
           << "  mean chord in shell   : " << (gLen/n)/cm << " cm   (edep steps " << gEdep << ")\n";
  }
};

int main(int argc, char** argv) {
  G4int n = (argc > 1) ? std::atoi(argv[1]) : 10000;
  auto* rm = new G4RunManager;
  rm->SetUserInitialization(new Detector);
  rm->SetUserInitialization(G4PhysListFactory().GetReferencePhysList("QGSP_BIC_HP"));
  rm->SetUserAction(new Primary);
  rm->SetUserAction(new Run);
  rm->SetUserAction(new Stepping);
  rm->Initialize();
  rm->BeamOn(n);
  delete rm;
  return 0;
}

```

## 认知护栏 / 老师傅插件诊断报告与质询：
【认知护栏检测结果】：
✓ 物理守恒与相空间测度不变性核验通过 （自动识别核素：Cs-137）。

## 修正要求：
请仔细阅读上述认知护栏/老师傅插件给出的检测提示与质询问题，针对报告中指出的具体可疑槽位与物理缺陷进行核实与修改。

请严格遵照以下外壳规范交付修正后的最终代码：
1. 目标平台 Geant4 11.2.2，单个 .cc 文件，自带 main()，能直接 g++ 编译运行，不依赖外部宏文件。
2. 代码尽量短：控制在 100~150 行以内，不要复杂几何。
3. 输出一份完整可编译且物理修正后的单一 .cc C++ 源码。

