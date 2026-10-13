#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tests/test_golden_physics_benchmarks.py —— 真实物理黄金基准自动化测试集
对 8 组经典粒子输运任务（16 份真实 Geant4 C++ 源码）进行自动化回归验证，
确保所有缺陷组 (Arm A) 100% 被拦截，所有黄金合规组 (Arm C) 100% 放行。
"""

import pytest
from pathlib import Path
from guardrail.ir import PSTBuilder, PSTVerifier
from scripts.eval_pst import BENCHMARK_SUITE

ROOT = Path(__file__).resolve().parent.parent


@pytest.fixture(scope="module")
def verifier_tools():
    builder = PSTBuilder()
    verifier = PSTVerifier()
    return builder, verifier


@pytest.mark.parametrize("task", BENCHMARK_SUITE, ids=lambda t: t["task_id"])
def test_golden_benchmark_blocks_defect_code(verifier_tools, task):
    """验证缺陷代码 Arm A 必须被精确拦截并触发预期规则"""
    builder, verifier = verifier_tools
    arm_a_p = ROOT / task["arm_a"]
    assert arm_a_p.exists(), f"Defect source file missing: {arm_a_p}"

    graph = builder.build_from_file(str(arm_a_p))
    report = verifier.verify(graph)

    # 必须拦截
    assert not report.passed, f"Task {task['task_id']} Arm A should be blocked, but passed!"
    # 必须命中预期的守恒违约规则
    expected_rule = task["expected_rule"]
    matched = any(expected_rule in v.rule_id for v in report.violations)
    assert matched, f"Task {task['task_id']} expected {expected_rule}, but got {[v.rule_id for v in report.violations]}"


@pytest.mark.parametrize("task", BENCHMARK_SUITE, ids=lambda t: t["task_id"])
def test_golden_benchmark_passes_compliant_code(verifier_tools, task):
    """验证黄金合规代码 Arm C 必须零误报顺利放行"""
    builder, verifier = verifier_tools
    arm_c_p = ROOT / task["arm_c"]
    assert arm_c_p.exists(), f"Compliant source file missing: {arm_c_p}"

    graph = builder.build_from_file(str(arm_c_p))
    report = verifier.verify(graph)

    # 必须放行
    assert report.passed, f"Task {task['task_id']} Arm C should pass, but was blocked by {[v.rule_id for v in report.violations]}!"
