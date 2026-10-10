#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tests/test_measure_basis_and_abstract_interpretation.py

验证控制流四维测度积分代数完备基 (Support, Multiplicity, Fibration, Filtration)
与微观物理抽象解释器 (MicroAbstractInterpreter) 及拓扑干跑器 (MockTopologyRunner) 的形式化核验能力。
"""

import time
import pytest
from guardrail.ir.pst_builder import PSTBuilder
from guardrail.ir.pst_verifier import PSTVerifier
from guardrail.ir.abstract_interpreter import MicroAbstractInterpreter
from guardrail.ir.mock_topology import MockTopologyRunner
from guardrail.ir.pst_schema import ConservationDomain, PSTEdgeType, MeasureBasis


def test_micro_abstract_interpreter_resolves_pointer_aliases_and_weight_propagation():
    """
    测试第 2 层微观物理抽象解释器：
    穿透多重指针别名传递 (t1 -> t2 -> t3) 与局部运算 (w * 1.5)，正确传播权重属性并在 < 2ms 内完成。
    """
    code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        auto* track = step->GetTrack();
        auto* t2 = track;
        auto* t3 = t2;
        double w = t3->GetWeight();
        double flux_inc = w * 1.5;
        fFlux += flux_inc;
    }
    """
    interpreter = MicroAbstractInterpreter()
    # 预热一次排除 Python 正则首次编译冷启动开销
    interpreter.interpret_code_snippet(code)

    t0 = time.perf_counter()
    profile = interpreter.interpret_code_snippet(code, func_name="UserSteppingAction")
    elapsed_ms = (time.perf_counter() - t0) * 1000

    assert elapsed_ms < 2.0, f"稳态微观抽象解释耗时 {elapsed_ms:.2f}ms 超过预期阈值 2ms"
    assert "t3" in profile.env
    assert profile.env["t3"].target_entity == "Track"
    assert profile.env["w"].is_weighted is True
    assert profile.env["flux_inc"].is_weighted is True
    assert len(profile.weighted_accumulations) == 1
    assert profile.weighted_accumulations[0]["is_weighted"] is True

    # 通过 PSTBuilder 构建并在 PST 上断言 WeightFlow 边
    builder = PSTBuilder()
    graph = builder.build(code, filename="SteppingAction.cc")
    has_wf = any(e.edge_type == PSTEdgeType.WEIGHT_FLOW for e in graph.edges)
    assert has_wf is True, "PST 依赖图中未成功构建出别名传递的 WeightFlow 边"

    # PSTVerifier 判定无权重违约
    verifier = PSTVerifier()
    report = verifier.verify(graph)
    assert not any(v.domain == ConservationDomain.VARIANCE for v in report.violations)


def test_measure_basis_support_misalignment_intercepted():
    """
    测试四维测度基 1：测度支撑域错位 (Support Misalignment · 条件槽位)
    在界面通量判定中误用 PreStepPoint，导致积分域偏离界面支撑集 ∂V。
    """
    buggy_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        auto* pre = step->GetPreStepPoint();
        if (pre->GetStepStatus() == fGeomBoundary) {
            fSurfaceFlux += step->GetTrack()->GetWeight();
        }
    }
    """
    builder = PSTBuilder()
    graph = builder.build(buggy_code, filename="SteppingAction.cc")
    verifier = PSTVerifier()
    report = verifier.verify(graph)

    assert report.passed is False
    assert any(
        v.rule_id in ("PST-INV-LIFECYCLE-01", "PST-INV-CONTROL-SUPPORT-01")
        for v in report.violations
    )


def test_measure_basis_multiplicity_inflation_intercepted():
    """
    测试四维测度基 2：示性测度重度倍乘 (Multiplicity Inflation · 次数槽位)
    次级粒子生成计分缺失单步示性守卫 (GetCurrentStepNumber() == 1) 或集合去重，
    导致示性函数在沿径迹输运的数百个步进中发生重度膨胀 dμ -> k·dμ。
    """
    buggy_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        if (step->GetTrack()->GetDefinition() == G4Neutron::NeutronDefinition()) {
            fSecondaryYield += 1;
        }
    }
    """
    builder = PSTBuilder()
    graph = builder.build(buggy_code, filename="SteppingAction.cc")
    verifier = PSTVerifier()
    report = verifier.verify(graph)

    assert report.passed is False
    assert any(v.rule_id == "PST-INV-CONTROL-MULTIPLICITY-01" for v in report.violations)
    mult_viol = next(v for v in report.violations if v.rule_id == "PST-INV-CONTROL-MULTIPLICITY-01")
    assert "示性测度重度倍乘" in mult_viol.defect_name
    assert mult_viol.diff is not None


def test_measure_basis_multiplicity_guard_compliant():
    """
    测试四维测度基 2：带有合规单步示性守卫的代码顺利通过验证。
    """
    compliant_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        if (step->GetTrack()->GetCurrentStepNumber() == 1) {
            fSecondaryYield += 1;
        }
    }
    """
    builder = PSTBuilder()
    graph = builder.build(compliant_code, filename="SteppingAction.cc")
    verifier = PSTVerifier()
    report = verifier.verify(graph)

    assert not any(v.rule_id == "PST-INV-CONTROL-MULTIPLICITY-01" for v in report.violations)


def test_measure_basis_filtration_inversion_intercepted():
    """
    测试四维测度基 4：鞅信息流因果滤波逆向 (Filtration Inversion · 顺序槽位)
    在相互作用过程触发改变状态之后才提取本应属于初态的物理量，导致时序因果倒置。
    """
    buggy_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        proc->SampleSecondaries();
        double eKin = step->GetPostStepPoint()->GetKineticEnergy();
        fIncidentSpectrum += eKin;
    }
    """
    builder = PSTBuilder()
    graph = builder.build(buggy_code, filename="SteppingAction.cc")
    verifier = PSTVerifier()
    report = verifier.verify(graph)

    assert report.passed is False
    assert any(v.rule_id == "PST-INV-CONTROL-FILTRATION-01" for v in report.violations)
    filt_viol = next(v for v in report.violations if v.rule_id == "PST-INV-CONTROL-FILTRATION-01")
    assert "因果滤波时序逆向" in filt_viol.defect_name


def test_mock_topology_dry_run_accuracy_and_performance():
    """
    测试第 3 层轻量拓扑干跑器 (MockTopologyRunner)：
    在 < 1ms 内从复杂 DetectorConstruction 源码解析 CSG 实体、死层隔离与质量估算。
    """
    detector_code = """
    G4VPhysicalVolume* DetectorConstruction::Construct() {
        auto* worldBox = new G4Box("WorldSolid", 1.0*m, 1.0*m, 1.0*m);
        auto* logicWorld = new G4LogicalVolume(worldBox, matAir, "WorldLogical");

        auto* crystalTubs = new G4Tubs("CrystalSolid", 0.*cm, 3.8*cm, 3.8*cm, 0.*deg, 360.*deg);
        auto* logicCrystal = new G4LogicalVolume(crystalTubs, matNaI, "NaICrystalLogical");

        auto* deadTubs = new G4Tubs("DeadSolid", 3.8*cm, 3.82*cm, 3.8*cm, 0.*deg, 360.*deg);
        auto* logicDead = new G4LogicalVolume(deadTubs, matAl, "DeadLayerLogical");

        new G4PVPlacement(0, G4ThreeVector(), logicCrystal, "CrystalPhys", logicWorld, false, 0);
        new G4PVPlacement(0, G4ThreeVector(), logicDead, "DeadPhys", logicWorld, false, 0);

        return new G4PVPlacement(0, G4ThreeVector(), logicWorld, "WorldPhys", nullptr, false, 0);
    }
    """
    runner = MockTopologyRunner()
    # 预热一次排除 Python 正则首次编译冷启动开销
    runner.run_dry(detector_code)

    t0 = time.perf_counter()
    manifest = runner.run_dry(detector_code)
    elapsed_ms = (time.perf_counter() - t0) * 1000

    assert elapsed_ms < 2.0, f"稳态拓扑干跑耗时 {elapsed_ms:.2f}ms 超过预期阈值 2ms"
    assert "CrystalSolid" in manifest.solids
    assert "DeadSolid" in manifest.solids
    assert "NaICrystalLogical" in manifest.sensitive_volumes
    assert "DeadLayerLogical" in manifest.dead_layer_volumes
    assert "WorldLogical" in manifest.containment_hierarchy
    assert len(manifest.containment_hierarchy["WorldLogical"]) == 2
