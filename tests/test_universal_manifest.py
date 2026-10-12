#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tests/test_universal_manifest.py —— 蒙卡代码实参显式清单与深层物理特性透视测试套件
验证多能点物性数组曲线 f(E)、分子结构与热中子晶格散射核 S(α, β)、探测器全景建模角色的泛用提取与物理合理性校验。
"""

import pytest
from pathlib import Path
from guardrail.ir.code_manifest import CodeManifestExtractor, CodeParameterManifest


@pytest.fixture
def extractor():
    return CodeManifestExtractor()


def test_property_curves_extraction_compliant(extractor, tmp_path):
    """验证合规多能点连续物性数组曲线的提取 (自变量/因变量跨度、单调性校验)"""
    code = """
    #include "G4Material.hh"
    #include "G4MaterialPropertiesTable.hh"
    #include "G4NistManager.hh"
    #include "G4SystemOfUnits.hh"

    void SetupMaterial() {
        auto* nist = G4NistManager::Instance();
        auto* water = nist->FindOrBuildMaterial("G4_WATER");
        auto* mpt = new G4MaterialPropertiesTable();

        std::vector<G4double> photonE = {1.5 * eV, 3.0 * eV, 6.0 * eV};
        std::vector<G4double> rIndex  = {1.333, 1.335, 1.340};
        mpt->AddProperty("RINDEX", photonE, rIndex);

        water->SetMaterialPropertiesTable(mpt);
    }
    """
    f = tmp_path / "water_optics.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.property_curves) == 1
    curve = manifest.property_curves[0]

    assert curve.material_name == "G4_WATER"
    assert curve.property_name == "RINDEX"
    assert curve.num_points == 3
    assert curve.x_range == (1.5, 6.0)
    assert curve.x_unit == "eV"
    assert curve.y_range == (1.333, 1.340)
    assert curve.is_monotonic_increasing is True
    assert curve.is_physically_sound is True
    assert len(manifest.drift_warnings) == 0


def test_property_curves_detects_non_monotonic_and_unphysical_rindex(extractor, tmp_path):
    """验证物性数组能量非单调乱序与因果律破缺 (n < 1.0) 的自动捕获与报警"""
    code = """
    #include "G4Material.hh"
    #include "G4MaterialPropertiesTable.hh"
    #include "G4NistManager.hh"
    #include "G4SystemOfUnits.hh"

    void SetupFlawedMaterial() {
        auto* nist = G4NistManager::Instance();
        auto* mat = nist->FindOrBuildMaterial("G4_GLASS");
        auto* mpt = new G4MaterialPropertiesTable();

        // 能量倒序: 6.0 eV -> 1.5 eV (违反 Geant4 单调递增公理)
        // 折射率包含 0.85 (实部小于 1.0，违反因果律介电响应)
        std::vector<G4double> photonE = {6.0 * eV, 1.5 * eV};
        std::vector<G4double> rIndex  = {0.85, 1.50};
        mpt->AddProperty("RINDEX", photonE, rIndex);

        mat->SetMaterialPropertiesTable(mpt);
    }
    """
    f = tmp_path / "flawed_optics.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.property_curves) == 1
    curve = manifest.property_curves[0]

    assert curve.is_monotonic_increasing is False
    assert curve.is_physically_sound is False
    assert "小于 1.0" in curve.soundness_remark or "因果律" in curve.soundness_remark

    # 验证告警已注入 drift_warnings
    warn_text = " ".join(manifest.drift_warnings)
    assert "因果律破缺" in warn_text
    assert "物性网格非单调违约" in warn_text


def test_thermal_scattering_compliant_activation(extractor, tmp_path):
    """验证热中子晶格散射核 S(α, β) 完备使能自洽性判断"""
    code = """
    #include "G4NistManager.hh"
    #include "G4Box.hh"
    #include "G4LogicalVolume.hh"
    #include "G4PVPlacement.hh"
    #include "G4VModularPhysicsList.hh"
    #include "G4ThermalNeutrons.hh"
    #include "G4ParticleGun.hh"
    #include "G4SystemOfUnits.hh"

    class Det {
    public:
        void Build() {
            auto* nist = G4NistManager::Instance();
            auto* water = nist->FindOrBuildMaterial("G4_WATER");
            auto* box = new G4Box("WaterTank", 20*cm, 20*cm, 20*cm);
            auto* log = new G4LogicalVolume(box, water, "WaterTank");
            new G4PVPlacement(nullptr, {}, log, "WaterTank", nullptr, false, 0);
        }
    };

    class Phys : public G4VModularPhysicsList {
    public:
        Phys() {
            RegisterPhysics(new G4ThermalNeutrons());
        }
    };

    void SetupSource(G4ParticleGun* gun) {
        gun->SetParticleEnergy(0.025 * eV);
    }
    """
    f = tmp_path / "thermal_compliant.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.thermal_scattering) >= 1
    ts = manifest.thermal_scattering[0]

    assert "WATER" in ts.material_name.upper()
    assert ts.tsl_state == "TS_H_of_Water"
    assert ts.is_physics_registered is True
    assert ts.is_material_bound is True
    assert "合规激活" in ts.cross_check_status


def test_thermal_scattering_detects_missing_physics(extractor, tmp_path):
    """验证慢化介质中输运热中子但未注册 G4ThermalNeutrons 时发出明确物理警报"""
    code = """
    #include "G4NistManager.hh"
    #include "G4Box.hh"
    #include "G4LogicalVolume.hh"
    #include "G4PVPlacement.hh"
    #include "G4VModularPhysicsList.hh"
    #include "G4ParticleGun.hh"
    #include "G4ParticleTable.hh"
    #include "G4SystemOfUnits.hh"

    class Det {
    public:
        void Build() {
            auto* nist = G4NistManager::Instance();
            auto* water = nist->BuildMaterialWithNewDensity("H2O", "G4_WATER", 1.0 * g/cm3);
            auto* box = new G4Box("Moderator", 30*cm, 30*cm, 30*cm);
            auto* log = new G4LogicalVolume(box, water, "Moderator");
            new G4PVPlacement(nullptr, {}, log, "Moderator", nullptr, false, 0);
        }
    };

    class Phys : public G4VModularPhysicsList {
    public:
        Phys() {
            // 缺失: 未注册 G4ThermalNeutrons
        }
    };

    void SetupSource(G4ParticleGun* gun) {
        gun->SetParticleDefinition(G4ParticleTable::GetParticleTable()->FindParticle("neutron"));
        gun->SetParticleEnergy(0.025 * eV);
    }
    """
    f = tmp_path / "thermal_flaw.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.thermal_scattering) >= 1
    ts = manifest.thermal_scattering[0]

    assert ts.is_physics_registered is False
    assert "缺失物理过程" in ts.cross_check_status

    warn_text = " ".join(manifest.drift_warnings)
    assert "热中子晶格动力学缺失" in warn_text


def test_detector_roles_classification(extractor, tmp_path):
    """验证探测器组件全景结构分类 (灵敏晶体 / 死层 / 薄入射窗 / 屏蔽体 / 环境容器)"""
    code = """
    #include "G4Box.hh"
    #include "G4Tubs.hh"
    #include "G4LogicalVolume.hh"
    #include "G4PVPlacement.hh"
    #include "G4NistManager.hh"

    void ConstructDetector() {
        auto* nist = G4NistManager::Instance();
        auto* air = nist->FindOrBuildMaterial("G4_AIR");
        auto* ge = nist->FindOrBuildMaterial("G4_Ge");
        auto* pb = nist->FindOrBuildMaterial("G4_Pb");
        auto* be = nist->FindOrBuildMaterial("G4_Be");

        // 1. World
        auto* solidWorld = new G4Box("World", 100, 100, 100);
        auto* logWorld = new G4LogicalVolume(solidWorld, air, "World");

        // 2. Active Crystal
        auto* solidCrystal = new G4Tubs("HPGeCrystal", 0, 30, 30, 0, 360);
        auto* logCrystal = new G4LogicalVolume(solidCrystal, ge, "HPGeCrystal");

        // 3. Dead Layer
        auto* solidDead = new G4Tubs("DeadLayerContact", 0, 30, 0.5, 0, 360);
        auto* logDead = new G4LogicalVolume(solidDead, ge, "DeadLayerContact");

        // 4. Entrance Window
        auto* solidWin = new G4Tubs("BerylliumWindow", 0, 35, 0.2, 0, 360);
        auto* logWin = new G4LogicalVolume(solidWin, be, "BerylliumWindow");

        // 5. Shielding
        auto* solidShield = new G4Box("LeadShieldBox", 50, 50, 50);
        auto* logShield = new G4LogicalVolume(solidShield, pb, "LeadShieldBox");
    }
    """
    f = tmp_path / "hpge_detector.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    roles = {g.solid_name: g.role for g in manifest.geometry}

    assert "World 容器" in roles["World"]
    assert "灵敏探测晶体" in roles["HPGeCrystal"]
    assert "非活性死层" in roles["DeadLayerContact"]
    assert "薄入射窗" in roles["BerylliumWindow"]
    assert "屏蔽" in roles["LeadShieldBox"]


def test_extraction_latency_performance(extractor):
    """确保提取引擎维持微秒至毫秒级 (单文件 < 30ms) 静态高吞吐性能"""
    target = "benchmarks/code_C/T4-3/code.cc"
    manifest = extractor.extract(target)
    assert manifest.extraction_latency_ms < 30.0
    assert len(manifest.geometry) >= 2
    assert len(manifest.property_curves) >= 2
    assert manifest.physics.birks_constant == "0.126 * mm / MeV"
