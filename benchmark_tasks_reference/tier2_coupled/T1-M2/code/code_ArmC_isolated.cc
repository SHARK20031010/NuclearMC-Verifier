/*
```guardrail-intent
{
  "F1a": {"v": "fluence", "src": "U"},
  "F1b": {"v": "T1-M2 目标几何空间出射面", "src": "U"},
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
*/

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UserEventAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "QGSP_BIC_HP.hh"
#include "G4ThermalNeutrons.hh"
#include "Randomize.hh"
#include "G4RandomDirection.hh"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>
#include <set>

// 目标几何空间出射面有效表面积 (半径 30 cm，半高 50 cm 圆柱屏蔽套外表面)
// 外柱面侧面积: 2 * pi * 30 * 100 = 18849.56 cm2; 铅套端面圆环面积: 2 * pi * (30^2 - 25^2) = 1727.88 cm2
static const G4double gExitArea = 20577.44 * cm2;

// 中子直接穿透统计 (weighted)
static G4double gNeutronCount = 0.0;
static G4double gNeutronEkinSum = 0.0; // MeV
static G4double gNeutronDoseGy = 0.0;  // Gy (单粒子累加)

// 慢化热中子俘获产生的次级级联伽马统计 (weighted)
static G4double gCaptureGammaCount = 0.0;
static G4double gCaptureGammaEkinSum = 0.0; // MeV
static G4double gCaptureGammaDoseGy = 0.0;  // Gy (单粒子累加)

// 非俘获次级伽马统计 (如非弹性散射次级伽马, weighted)
static G4double gInelasticGammaCount = 0.0;
static G4double gInelasticGammaEkinSum = 0.0; // MeV
static G4double gInelasticGammaDoseGy = 0.0;  // Gy (单粒子累加)

// 单事件出射径迹去重集合
static std::set<G4int> gScoredTrackIDs;

class Det2C : public G4VUserDetectorConstruction {
public:
  G4VPhysicalVolume* Construct() override {
    auto* nist = G4NistManager::Instance();

    // 基础材料定义
    auto* matAir = nist->FindOrBuildMaterial("G4_AIR");
    auto* matFe  = nist->FindOrBuildMaterial("G4_Fe");
    auto* matPb  = nist->FindOrBuildMaterial("G4_Pb");
    auto* matPE  = nist->FindOrBuildMaterial("G4_POLYETHYLENE");

    // 复合材料：含硼聚乙烯 (5 wt% Boron, 95 wt% Polyethylene, 密度 1.0 g/cm3)
    auto* matBPE = new G4Material("Borated_POLYETHYLENE", 1.0 * g/cm3, 2);
    matBPE->AddMaterial(matPE, 95.0 * perCent);
    matBPE->AddElement(nist->FindOrBuildElement("B"), 5.0 * perCent);

    // 世界体积
    auto* worldSolid = new G4Box("World", 1.5 * m, 1.5 * m, 1.5 * m);
    auto* worldLog = new G4LogicalVolume(worldSolid, matAir, "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLog, "World", nullptr, false, 0);

    // 三层同心圆柱屏蔽套 (套筒总高 100 cm, dz = 50 cm, 内腔半径 5 cm 放置 252Cf 源)
    const G4double halfLength = 50.0 * cm;

    // 1. 内层 5 cm 铁套筒 (r: 5 cm -> 10 cm, 厚度 5 cm)
    auto* feSolid = new G4Tubs("FeShell", 5.0 * cm, 10.0 * cm, halfLength, 0.0, 360.0 * deg);
    auto* feLog = new G4LogicalVolume(feSolid, matFe, "FeShell");
    new G4PVPlacement(nullptr, {}, feLog, "FeShell", worldLog, false, 0);

    // 2. 中层 15 cm 含硼聚乙烯套筒 (r: 10 cm -> 25 cm, 厚度 15 cm)
    auto* bpeSolid = new G4Tubs("BPEShell", 10.0 * cm, 25.0 * cm, halfLength, 0.0, 360.0 * deg);
    auto* bpeLog = new G4LogicalVolume(bpeSolid, matBPE, "BPEShell");
    new G4PVPlacement(nullptr, {}, bpeLog, "BPEShell", worldLog, false, 0);

    // 3. 外层 5 cm 铅套筒 (r: 25 cm -> 30 cm, 厚度 5 cm)
    auto* pbSolid = new G4Tubs("PbShell", 25.0 * cm, 30.0 * cm, halfLength, 0.0, 360.0 * deg);
    auto* pbLog = new G4LogicalVolume(pbSolid, matPb, "PbShell");
    new G4PVPlacement(nullptr, {}, pbLog, "PbShell", worldLog, false, 0);

    return worldPV;
  }
};

class Phys2C : public QGSP_BIC_HP {
public:
  Phys2C() {
    // 挂载热中子高精度散射物理包以精确模拟热中子在含硼聚乙烯中的慢化与俘获截面
    RegisterPhysics(new G4ThermalNeutrons());
  }
};

class Prim2C : public G4VUserPrimaryGeneratorAction {
private:
  G4ParticleGun* fGun;

  // 252Cf 自发裂变中子能谱 (Watt 谱: a = 1.18 MeV, b = 1.0341 / MeV)
  G4double SampleCf252WattSpectrum() {
    const G4double a = 1.18; // MeV
    const G4double b = 1.0341; // 1/MeV
    while (true) {
      G4double Ek = G4UniformRand() * 15.0; // 0 to 15 MeV
      if (Ek <= 0.0) continue; // 物理非负截断保护 Ek > 0
      G4double f = std::exp(-Ek / a) * std::sinh(std::sqrt(b * Ek));
      if (G4UniformRand() * 0.40 < f) {
        return Ek * MeV;
      }
    }
  }

public:
  Prim2C() {
    fGun = new G4ParticleGun(1);
    fGun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
    fGun->SetParticlePosition(G4ThreeVector(0.0, 0.0, 0.0));
  }
  ~Prim2C() override { delete fGun; }

  void GeneratePrimaries(G4Event* ev) override {
    // 4pi 各向同性立体角抽样
    fGun->SetParticleMomentumDirection(G4RandomDirection());
    fGun->SetParticleEnergy(SampleCf252WattSpectrum());
    fGun->GeneratePrimaryVertex(ev);
  }
};

class Event2C : public G4UserEventAction {
public:
  void BeginOfEventAction(const G4Event*) override {
    // 跨事件清空径迹去重集合
    gScoredTrackIDs.clear();
  }
};

class Step2C : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override {
    auto* postPoint = step->GetPostStepPoint();
    // 几何边界跨越严格在 PostStepPoint 上判定 fGeomBoundary
    if (postPoint->GetStepStatus() != fGeomBoundary) return;

    auto* prePV = step->GetPreStepPoint()->GetPhysicalVolume();
    auto* postPV = postPoint->GetPhysicalVolume();
    if (!prePV || !postPV) return;

    // 粒子穿透外层铅屏蔽套进入世界空气外表面
    if (prePV->GetName() == "PbShell" && postPV->GetName() == "World") {
      auto* track = step->GetTrack();
      G4int trackID = track->GetTrackID();
      if (gScoredTrackIDs.count(trackID)) return;
      gScoredTrackIDs.insert(trackID);

      // 深穿透权重契约
      const G4double weight = track->GetWeight();
      const G4double EkinMeV = postPoint->GetKineticEnergy() / MeV;
      const G4String partName = track->GetParticleDefinition()->GetParticleName();
      const G4double areaCm2 = gExitArea / cm2;

      if (partName == "neutron") {
        // 中子直接穿透统计
        gNeutronCount += weight;
        gNeutronEkinSum += EkinMeV * weight;
        // 中子注量到组织吸收剂量转换系数 (快中子约为 3.0e-11 Gy*cm2/neutron)
        const G4double h_n_Gy_cm2 = 3.0e-11;
        gNeutronDoseGy += (weight / areaCm2) * h_n_Gy_cm2;
      }
      else if (partName == "gamma") {
        // 次级级联伽马：追溯母体物理产生过程
        const G4VProcess* creator = track->GetCreatorProcess();
        G4String procName = creator ? creator->GetProcessName() : "primary";

        // 伽马组织吸收剂量转换系数 (质量能量吸收系数 mu_en/rho ~ 0.03 cm2/g => 4.806e-12 Gy*cm2/MeV)
        const G4double h_gamma_Gy_cm2_per_MeV = 4.806e-12;
        G4double dGammaGy = (weight / areaCm2) * EkinMeV * h_gamma_Gy_cm2_per_MeV;

        if (procName.find("Capture") != std::string::npos ||
            procName.find("capture") != std::string::npos ||
            procName.find("nCapture") != std::string::npos) {
          // 慢化热中子俘获产生的次级级联伽马贡献
          gCaptureGammaCount += weight;
          gCaptureGammaEkinSum += EkinMeV * weight;
          gCaptureGammaDoseGy += dGammaGy;
        } else {
          // 其他次级伽马贡献 (如中子非弹性散射次级伽马)
          gInelasticGammaCount += weight;
          gInelasticGammaEkinSum += EkinMeV * weight;
          gInelasticGammaDoseGy += dGammaGy;
        }
      }

      // 出射后截断径迹，防止外层空气多次反散射重入虚高
      track->SetTrackStatus(fStopAndKill);
    }
  }
};

int main(int argc, char** argv) {
  G4int nEvents = (argc > 1) ? std::atoi(argv[1]) : 50;

  auto* rm = new G4RunManager();
  rm->SetVerboseLevel(0);
  rm->SetUserInitialization(new Det2C());
  rm->SetUserInitialization(new Phys2C());
  rm->SetUserAction(new Prim2C());
  rm->SetUserAction(new Event2C());
  rm->SetUserAction(new Step2C());

  rm->Initialize();
  rm->BeamOn(nEvents);

  // 物理结果解耦归一化输出
  G4double areaCm2 = gExitArea / cm2;
  G4double neutronFluencePerSource = (gNeutronCount / nEvents) / areaCm2;
  G4double captureGammaFluencePerSource = (gCaptureGammaCount / nEvents) / areaCm2;
  G4double inelasticGammaFluencePerSource = (gInelasticGammaCount / nEvents) / areaCm2;

  G4double neutronDosePerSource = gNeutronDoseGy / nEvents;
  G4double captureGammaDosePerSource = gCaptureGammaDoseGy / nEvents;
  G4double inelasticGammaDosePerSource = gInelasticGammaDoseGy / nEvents;
  G4double totalDosePerSource = neutronDosePerSource + captureGammaDosePerSource + inelasticGammaDosePerSource;

  std::cout << "\n================ [T1-M2 Simulation Summary] ================" << std::endl;
  std::cout << "Simulated Events: " << nEvents << std::endl;
  std::cout << "Exit Surface Area: " << areaCm2 << " cm2" << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;
  std::cout << "[1] Neutron Direct Penetration:" << std::endl;
  std::cout << "    - Count: " << gNeutronCount << " (" << (gNeutronCount / nEvents) << " /source)" << std::endl;
  std::cout << "    - Fluence: " << std::scientific << std::setprecision(5) << neutronFluencePerSource << " cm^-2 /source" << std::endl;
  std::cout << "    - Absorbed Dose: " << std::scientific << std::setprecision(5) << neutronDosePerSource << " Gy /source" << std::endl;
  std::cout << "[2] Moderated Thermal Neutron Capture Gamma:" << std::endl;
  std::cout << "    - Count: " << gCaptureGammaCount << " (" << (gCaptureGammaCount / nEvents) << " /source)" << std::endl;
  std::cout << "    - Fluence: " << std::scientific << std::setprecision(5) << captureGammaFluencePerSource << " cm^-2 /source" << std::endl;
  std::cout << "    - Absorbed Dose: " << std::scientific << std::setprecision(5) << captureGammaDosePerSource << " Gy /source" << std::endl;
  std::cout << "[3] Inelastic & Other Secondary Gamma:" << std::endl;
  std::cout << "    - Count: " << gInelasticGammaCount << " (" << (gInelasticGammaCount / nEvents) << " /source)" << std::endl;
  std::cout << "    - Fluence: " << std::scientific << std::setprecision(5) << inelasticGammaFluencePerSource << " cm^-2 /source" << std::endl;
  std::cout << "    - Absorbed Dose: " << std::scientific << std::setprecision(5) << inelasticGammaDosePerSource << " Gy /source" << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;
  std::cout << "Total Outer Surface Absorbed Dose: " << std::scientific << std::setprecision(5) << totalDosePerSource << " Gy /source" << std::endl;
  if (totalDosePerSource > 0.0) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Neutron Dose Share: " << (neutronDosePerSource / totalDosePerSource * 100.0) << " %" << std::endl;
    std::cout << "Capture Gamma Share: " << (captureGammaDosePerSource / totalDosePerSource * 100.0) << " %" << std::endl;
    std::cout << "Other Secondary Gamma Share: " << (inelasticGammaDosePerSource / totalDosePerSource * 100.0) << " %" << std::endl;
  }
  std::cout << "============================================================\n" << std::endl;

  delete rm;
  std::_Exit(0);
}
