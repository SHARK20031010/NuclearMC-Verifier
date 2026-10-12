#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tests/test_dynamic_loops_and_files.py
专门验证：
1. 动态循环与公式计算（Procedural Loops & Math Formulas，如柯西色散公式计算折射率）
2. 外部数据文件读取（File I/O，如 std::ifstream 读取 CSV/DAT 数据）
的自动静态展开、数值提取与守恒律核验。
"""

import pytest
from pathlib import Path
from guardrail.ir.code_manifest import CodeManifestExtractor


@pytest.fixture
def extractor():
    return CodeManifestExtractor()


def test_procedural_loop_cauchy_formula_evaluation(extractor, tmp_path):
    """验证动态循环与柯西色散公式计算的自动求值与物性抽取"""
    code = """
    #include "G4Material.hh"
    #include "G4MaterialPropertiesTable.hh"
    #include "G4NistManager.hh"
    #include "G4SystemOfUnits.hh"
    #include <vector>

    void ConstructMaterial() {
        auto* nist = G4NistManager::Instance();
        auto* water = nist->FindOrBuildMaterial("G4_WATER");
        auto* mpt = new G4MaterialPropertiesTable();

        std::vector<G4double> photonE;
        std::vector<G4double> rIndex;

        // 柯西色散经验公式: n(E) = 1.333 + 0.05 / (e * e)
        for (double e = 1.5; e <= 6.0; e += 0.5) {
            photonE.push_back(e * eV);
            double n = 1.333 + 0.05 / (e * e);
            rIndex.push_back(n);
        }

        mpt->AddProperty("RINDEX", photonE, rIndex);
        water->SetMaterialPropertiesTable(mpt);
    }
    """
    f = tmp_path / "cauchy_optics.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.property_curves) == 1
    curve = manifest.property_curves[0]

    assert curve.material_name == "G4_WATER"
    assert curve.property_name == "RINDEX"
    # 1.5 到 6.0 步长 0.5 共 10 个采样点
    assert curve.num_points == 10
    assert curve.x_range == (1.5, 6.0)
    assert curve.x_unit == "eV"
    # 极小值约 1.3344，极大值约 1.3552
    assert 1.330 <= curve.y_range[0] <= 1.336
    assert 1.350 <= curve.y_range[1] <= 1.360
    assert curve.is_monotonic_increasing is True
    assert curve.is_physically_sound is True
    assert "动态循环公式生成" in curve.data_origin


def test_external_data_file_loading(extractor, tmp_path):
    """验证从外部 CSV/DAT 数据文件读取物性数组的自动探测与加载"""
    # 1. 准备物理数据文件
    csv_file = tmp_path / "water_abslength.csv"
    csv_file.write_text("""
# Energy(eV), AbsLength(cm)
1.5, 50.0
2.0, 45.0
3.0, 30.0
4.0, 20.0
6.0, 10.0
""", encoding="utf-8")

    # 2. 准备 C++ 读取源码
    code = f"""
    #include "G4Material.hh"
    #include "G4MaterialPropertiesTable.hh"
    #include "G4NistManager.hh"
    #include "G4SystemOfUnits.hh"
    #include <fstream>
    #include <vector>

    void LoadAbsLength() {{
        auto* nist = G4NistManager::Instance();
        auto* water = nist->FindOrBuildMaterial("G4_WATER");
        auto* mpt = new G4MaterialPropertiesTable();

        std::vector<G4double> en_vec;
        std::vector<G4double> abs_vec;

        std::ifstream fin("water_abslength.csv");
        double e, abs_val;
        while (fin >> e >> abs_val) {{
            en_vec.push_back(e * eV);
            abs_vec.push_back(abs_val * cm);
        }}

        mpt->AddProperty("ABSLENGTH", en_vec, abs_vec);
        water->SetMaterialPropertiesTable(mpt);
    }}
    """
    f = tmp_path / "load_file.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.property_curves) == 1
    curve = manifest.property_curves[0]

    assert curve.material_name == "G4_WATER"
    assert curve.property_name == "ABSLENGTH"
    assert curve.num_points == 5
    assert curve.x_range == (1.5, 6.0)
    assert curve.x_unit == "eV"
    assert curve.y_range == (10.0, 50.0)
    assert curve.y_unit == "cm"
    assert curve.is_monotonic_increasing is True
    assert "外部文件读取" in curve.data_origin
    assert "water_abslength.csv" in curve.data_origin


def test_external_data_file_missing_graceful_handling(extractor, tmp_path):
    """验证外部数据文件暂未落盘时的优雅处理 (不报错且标记未就绪)"""
    code = """
    #include "G4MaterialPropertiesTable.hh"
    #include <fstream>
    #include <vector>

    void LoadMissing() {
        auto* mpt = new G4MaterialPropertiesTable();
        std::vector<G4double> e_vec;
        std::vector<G4double> r_vec;
        std::ifstream fin("non_existent_data.dat");
        double e, r;
        while (fin >> e >> r) {
            e_vec.push_back(e);
            r_vec.push_back(r);
        }
        mpt->AddProperty("RINDEX", e_vec, r_vec);
    }
    """
    f = tmp_path / "missing_file.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.property_curves) == 1
    curve = manifest.property_curves[0]
    assert "外部文件未就绪" in curve.data_origin
    assert "non_existent_data.dat" in curve.data_origin


def test_procedural_loop_detects_unphysical_formula(extractor, tmp_path):
    """验证动态循环公式计算出非物理值 (如 n < 1.0) 时的自动守恒拦截"""
    code = """
    #include "G4MaterialPropertiesTable.hh"
    #include <vector>

    void ConstructBadOptics() {
        auto* mpt = new G4MaterialPropertiesTable();
        std::vector<G4double> photonE;
        std::vector<G4double> rIndex;

        // 公式缺陷: 算出的折射率小于 1.0 (0.85 ~ 0.40)
        for (double e = 1.0; e <= 5.0; e += 1.0) {
            photonE.push_back(e * eV);
            double n = 0.95 - 0.1 * e;
            rIndex.push_back(n);
        }

        mpt->AddProperty("RINDEX", photonE, rIndex);
    }
    """
    f = tmp_path / "bad_formula.cc"
    f.write_text(code, encoding="utf-8")

    manifest = extractor.extract(str(f))
    assert len(manifest.property_curves) == 1
    curve = manifest.property_curves[0]

    assert curve.is_physically_sound is False
    warn_text = " ".join(manifest.drift_warnings)
    assert "因果律破缺" in warn_text
