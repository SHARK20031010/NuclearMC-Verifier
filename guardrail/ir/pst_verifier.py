#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/pst_verifier.py —— 物理语义树守恒验证器 (PST Conservation Verifier)

在 PST (Physics Semantic Tree) 中间表示图上执行图论遍历与形式化守恒断言，
将物理第一性原理映射为图拓扑结构、节点属性与守恒总线的确定性检验。
全面覆盖人类手写代码审查与 LLM 蒙卡程序生成门禁，输出包含 Unified Diff 补丁的标准诊断封包。
"""

from __future__ import annotations

import difflib
import re
from typing import Dict, List, Optional, Any
from pydantic import BaseModel, Field

from guardrail.ir.pst_schema import (
    PhysicalAction,
    ConservationDomain,
    PSTEdgeType,
    PSTNode,
    PSTEdge,
    PSTGraph,
    MeasureBasis,
    LifecycleState,
)
from guardrail.ir.intent_extractor import (
    PhysicsIntent,
    ObservableType,
    SourceSymmetry,
    TimingClock,
)


class PSTViolation(BaseModel):
    """形式化物理违约结果（统一诊断封包）"""
    rule_id: str = Field(description="规则唯一标识，如 PST-INV-001")
    domain: ConservationDomain = Field(description="违约所属守恒总线域")
    severity: str = Field(default="CRITICAL", description="严重等级: CRITICAL / HIGH / WARNING")
    defect_name: str = Field(description="物理缺陷名称")
    description: str = Field(description="基于物理第一性原理的机理诊断")
    violating_node_id: Optional[str] = Field(default=None, description="违约涉及的 PST 节点 ID")
    code_evidence: Optional[str] = Field(default=None, description="源码行证据")
    line_range: Optional[List[int]] = Field(default=None, description="源码行范围")
    remedy: str = Field(description="专家级规范修复建议")
    proposed_replacement: Optional[str] = Field(default=None, description="建议的替换代码片段")
    diff: Optional[str] = Field(default=None, description="标准 Unified Diff 补丁")
    source_file: Optional[str] = Field(default=None, description="违约源码所在文件路径")


class PSTVerificationReport(BaseModel):
    """PST 形式化核验完整报告"""
    graph_id: str
    source_file: str
    passed: bool
    violations_count: int
    violations: List[PSTViolation] = Field(default_factory=list)

    def summary(self) -> str:
        if self.passed:
            return f"✅ [PST VERIFICATION PASSED] {self.source_file}: 物理语义树符合全部守恒不变式。"
        lines = [f"❌ [PST VERIFICATION FAILED] {self.source_file}: 检出 {self.violations_count} 项物理守恒违约！"]
        for idx, v in enumerate(self.violations, start=1):
            lines.append(f"\n🚨 [{idx}] {v.defect_name} (规则 {v.rule_id} · [{v.domain.value}])")
            if v.line_range:
                lines.append(f"  • 代码位置: 行 {v.line_range[0]}-{v.line_range[1]}")
            if v.code_evidence:
                lines.append(f"  • 代码证据: `{v.code_evidence}`")
            lines.append(f"  • 物理机理诊断: {v.description}")
            lines.append(f"  • 规范修复指南: {v.remedy}")
            if v.diff:
                lines.append(f"  • 推荐修复 Diff:\n{v.diff}")
        return "\n".join(lines)


def _build_simple_diff(original: str, replacement: str) -> str:
    """生成简洁的红绿对比 Diff"""
    orig_lines = [f"- {l}" for l in original.strip().splitlines()]
    repl_lines = [f"+ {l}" for l in replacement.strip().splitlines()]
    return "\n".join(orig_lines + repl_lines)


class PSTVerifier:
    """物理语义树形式化断言引擎 (Layer 5)"""

    def verify(
        self,
        graph: PSTGraph,
        intent: Optional[PhysicsIntent] = None,
        enable_measure_basis: bool = True,
    ) -> PSTVerificationReport:
        violations: List[PSTViolation] = []

        # 1. 检验公平博弈统计权重守恒 (Fair-Game Weight Invariance · T1-6)
        self._check_fair_game_weight(graph, violations, intent)

        # 2. 检验相空间立体角测度守恒 (Phase-Space Measure Invariance · T4-1)
        self._check_phase_space_measure(graph, violations, intent)

        # 3. 检验因果与绝对实验时钟守恒 (Causal Time Invariance · T4-6)
        self._check_causal_timing(graph, violations, intent)

        # 4. 检验步进边界生命周期契约 (Boundary Step Lifecycle · T1-4)
        self._check_boundary_lifecycle(graph, violations)

        # 5. 检验探测器高斯能量分辨展宽 (FWHM vs Sigma · T4-5)
        self._check_energy_resolution_broadening(graph, violations)

        # 6. 检验半导体电荷载流子 Fano 涨落因子 (Fano Factor Variance · T4-8)
        self._check_fano_charge_fluctuation(graph, violations)

        # 7. 检验探测器死层与灵敏体积物理隔离 (Dead Layer Isolation · T4-4)
        self._check_dead_layer_isolation(graph, violations)

        # 8. 检验高剂量率脉冲时钟微观归一化 (Pulse Dose Rate Temporal Normalization · T2-5)
        self._check_pulsed_dose_rate_timing(graph, violations, intent)

        # 9. 检验跨事件容器生命周期重置 (Inter-Event Stale Tally Accumulation · PHYS-0046)
        self._check_inter_event_lifecycle_reset(graph, violations)

        # 10. 检验控制流四维测度积分代数完备基 (Four-Dimensional Measure-Theoretic Basis Invariance)
        if enable_measure_basis:
            self._check_measure_basis_invariance(graph, violations, intent)

        # 11. 检验闪烁介质非线性 Birks 饱和猝灭 (PHYS-INV-BIRKS-01)
        self._check_scintillator_birks_quenching(graph, violations)

        # 12. 检验光学光子介质折射率 RINDEX (PHYS-INV-OPTICAL-RINDEX-01)
        self._check_optical_material_rindex(graph, violations)

        # 13. 检验吸收剂量介质质量归一化 (PST-INV-SCALE-01)
        self._check_dose_mass_normalization(graph, violations)

        # 14. 检验运行管理器生命周期初始化契约 (PHYS-INV-RUN-01)
        self._check_run_lifecycle_initialization(graph, violations)

        # 15. 检验强子非弹性-弹性散射物理耦合 (PHYS-INV-HADRON-01)
        self._check_hadron_elastic_coupling(graph, violations)

        # 自动推断违规源码所在文件 (针对多文件工程)
        for v in violations:
            if v.violating_node_id:
                node = graph.get_node(v.violating_node_id)
                if node and node.attributes.get("source_file"):
                    v.source_file = node.attributes["source_file"]
            if not v.source_file:
                v.source_file = graph.source_file

        passed = len(violations) == 0
        return PSTVerificationReport(
            graph_id=graph.graph_id,
            source_file=graph.source_file,
            passed=passed,
            violations_count=len(violations),
            violations=violations,
        )

    def _strip_comments(self, src: str) -> str:
        def repl_multiline(m):
            return "\n" * m.group(0).count("\n")
        src = re.sub(r"/\*.*?\*/", repl_multiline, src, flags=re.S)
        src = re.sub(r"//[^\n]*", "", src)
        return src

    def _check_scintillator_birks_quenching(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """断言 11: 闪烁介质发光模拟必须配置 Birks 饱和猝灭常数 (PHYS-INV-BIRKS-01)"""
        raw_code = graph.attributes.get("raw_code", "")
        if not raw_code:
            return
        bare = self._strip_comments(raw_code).lower()
        if "scintillationyield" in bare:
            if "birks" not in bare and "saturation" not in bare:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-BIRKS-01",
                    domain=ConservationDomain.LIFECYCLE,
                    severity="CRITICAL",
                    defect_name="闪烁介质缺失非线性 Birks 饱和猝灭常数 (Missing Birks Constant)",
                    description=(
                        "材料属性表配置了闪烁光产额 (SCINTILLATIONYIELD)，但未设置 Birks 猝灭常数。"
                        "Geant4 默认 Birks 系数为 0 (无饱和猝灭)，快中子/反冲质子与高 LET 粒子光输出将被高估 5-10 倍。"
                    ),
                    violating_node_id=None,
                    code_evidence="mpt->AddConstProperty(\"SCINTILLATIONYIELD\", ...)",
                    remedy="配置材料 Birks 猝灭常数：scintMaterial->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);",
                ))

    def _check_optical_material_rindex(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """断言 12: 光学光子传输介质必须在材质表中绑定折射率 RINDEX (PHYS-INV-OPTICAL-RINDEX-01)"""
        raw_code = graph.attributes.get("raw_code", "")
        if not raw_code:
            return
        bare = self._strip_comments(raw_code).lower()
        has_optical = (
            ("g4opticalphysics" in bare or "registerphysics(new g4opticalphysics" in bare)
            or ("opticalphoton" in bare and "g4materialpropertiestable" in bare)
            or ("g4materialpropertiestable" in bare and any(k in bare for k in ["scintillationyield", "cerenkov", "chirenkov"]))
        )
        if has_optical:
            has_optical_medium = any(k in bare for k in ["cerenkov", "chirenkov", "契伦科夫", "g4materialpropertiestable", "mpt->"]) or any(k in bare for k in ["g4_sodium_iodide", "g4_water", "nai", "lyso"])
            has_rindex = bool(re.search(r'Add(?:Const)?Property\s*\(\s*["\']RINDEX["\']', bare, re.I))
            if has_optical_medium and not has_rindex:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-OPTICAL-RINDEX-01",
                    domain=ConservationDomain.PHASE_SPACE,
                    severity="CRITICAL",
                    defect_name="光学光子介质未配置折射率 RINDEX (Missing Optical Refractive Index)",
                    description=(
                        "启用了光学光子或介质发光模拟，但材料属性表中未绑定折射率 RINDEX。"
                        "缺失折射率导致几何界面无法计算斯涅尔定律折射与全反射，契伦科夫角与光子传输截面瘫痪。"
                    ),
                    violating_node_id=None,
                    code_evidence="mpt->AddProperty(...)",
                    remedy="在材料属性表中添加折射率：mpt->AddProperty(\"RINDEX\", photonEnergy, rindex, nEntries);",
                ))

    def _check_dose_mass_normalization(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """断言 13: 吸收剂量 (Gy) 累加必须除以介质质量 (PST-INV-SCALE-01)"""
        raw_code = graph.attributes.get("raw_code", "")
        if not raw_code:
            return
        bare = self._strip_comments(raw_code)
        for line in bare.splitlines():
            line_clean = line.strip()
            if re.search(r'\b(?:dose|fDose|gDose)\w*\s*(?:=|\+=)\s*(.+?);', line_clean, re.I):
                m = re.search(r'\b(?:dose|fDose|gDose)\w*\s*(?:=|\+=)\s*(.+?);', line_clean, re.I)
                rhs = m.group(1).lower()
                is_edep_accumulation = any(k in rhs for k in ["edep", "energydeposit", "totale", "ftotaledep"])
                has_mass = any(k in rhs for k in ["mass", "rho", "density", "vol", "getmass", "kg", "g/cm3"]) and "/" in rhs
                if is_edep_accumulation and not has_mass:
                    violations.append(PSTViolation(
                        rule_id="PST-INV-SCALE-01",
                        domain=ConservationDomain.PHASE_SPACE,
                        severity="CRITICAL",
                        defect_name="吸收剂量计分缺失靶体积质量归一化 (Dose Missing Mass Denominator)",
                        description=(
                            "吸收剂量 (Gy = J/kg) 计算中直接累加能量沉积，未除以靶体积介质质量 (m = V * rho)。"
                            "破坏量纲守恒不变式，宏观剂量数值被夸大千倍至百万倍。"
                        ),
                        violating_node_id=None,
                        code_evidence=line_clean,
                        remedy="将能量沉积除以靶介质质量：fDose += (edep / fMass);",
                    ))
                    break

    def _check_run_lifecycle_initialization(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """断言 14: BeamOn 执行前必须完成 RunManager 初始化 (PHYS-INV-RUN-01)"""
        raw_code = graph.attributes.get("raw_code", "")
        if not raw_code:
            return
        bare = self._strip_comments(raw_code).lower()
        has_beamon = "beamon(" in bare or "beamon (" in bare
        has_init = "initialize()" in bare or "initialize ()" in bare
        if has_beamon and not has_init:
            violations.append(PSTViolation(
                rule_id="PHYS-INV-RUN-01",
                domain=ConservationDomain.LIFECYCLE,
                severity="CRITICAL",
                defect_name="运行管理器未初始化即调用束流发射 (Uninitialized BeamOn Execution)",
                description=(
                    "在调用 BeamOn(...) 之前未调用 runManager->Initialize()。"
                    "几何探测器体、微观物理截面表与多线程内核尚未完成构造，"
                    "会导致未初始化粒子输运、空指针崩溃或物理截面全部清零。"
                ),
                violating_node_id=None,
                code_evidence="run->BeamOn(...)",
                remedy="在 BeamOn 前调用 runManager->Initialize();",
            ))

    def _check_hadron_elastic_coupling(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """断言 15: 中子深穿透与屏蔽模拟中非弹性散射必须耦合弹性散射 (PHYS-INV-HADRON-01)"""
        raw_code = graph.attributes.get("raw_code", "")
        if not raw_code:
            return
        bare = self._strip_comments(raw_code)
        bare_lower = bare.lower()
        _ELAS = re.compile(r"(?<!in)elastic", re.I)
        has_hadron_inel = ("g4hadronphysics" in bare_lower or "inelastic" in bare_lower)
        bare_without_hadron_phys = re.sub(r"g4hadronphysics\w+", "", bare_lower)
        has_official = any(off.lower() in bare_without_hadron_phys for off in ["qgsp_bic_hp", "ftfp_bert", "ftfp_bert_hp", "qbbc", "shielding"])
        is_neutron_scene = (
            "neutron" in bare_lower
            and any(k in bare_lower for k in ["concrete", "slab", "shield", "penetrat", "transmission", "穿透", "透射", "混凝土", "14 mev", "activation", "核素", "照射", "样品"])
            and not any(k in bare_lower for k in ["maze", "slit"])
        )
        has_elastic_registered = bool(re.search(r'registerphysics\s*\(\s*new\s+g4hadronelastic', bare_lower))
        if has_hadron_inel and not has_official and is_neutron_scene:
            if not has_elastic_registered:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-HADRON-01",
                    domain=ConservationDomain.NUCLEAR,
                    severity="CRITICAL",
                    defect_name="强子非弹性散射缺失弹性散射物理耦合 (Missing Hadron Elastic Coupling)",
                    description=(
                        "中子输运模拟中注册了强子非弹性散射物理过程，但缺失了强子弹性散射物理列表 (G4HadronElasticPhysics)。"
                        "中子在慢化减速过程中将失去弹性碰撞能量损失机制，导致穿透通量与减速热化发生数百倍量级偏置。"
                    ),
                    violating_node_id=None,
                    code_evidence="RegisterPhysics(new G4HadronPhysics...)",
                    remedy="注册弹性散射模块：RegisterPhysics(new G4HadronElasticPhysicsHP());",
                ))

    def _check_fair_game_weight(self, graph: PSTGraph, violations: List[PSTViolation], intent: Optional[PhysicsIntent] = None) -> None:
        """
        断言 1: 凡涉及通量累加 (FluenceScore) 或意图声明减方差的计分节点，必须具有入度边 WeightFlow。
        若缺失 WeightFlow 入度，意味着方差缩减操作下未乘入权重，导致通量被虚假放大。
        """
        accumulate_nodes = graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        requires_weight = (intent and intent.variance_reduction)
        for acc in accumulate_nodes:
            if acc.quantity == "FluenceScore" or (requires_weight and acc.quantity in ("FluenceScore", "PulseHeight", "SensitiveRegionHit")):
                incoming_edges = graph.get_incoming_edges(acc.node_id)
                has_weight_flow = any(e.edge_type == PSTEdgeType.WEIGHT_FLOW for e in incoming_edges)

                if not has_weight_flow:
                    repl = "gFlux += s->GetTrack()->GetWeight();" if "s->" in acc.code_evidence else "gFlux += step->GetTrack()->GetWeight();"
                    violations.append(PSTViolation(
                        rule_id="PST-INV-VARIANCE-01",
                        domain=ConservationDomain.VARIANCE,
                        severity="CRITICAL",
                        defect_name="方差缩减通量计分漏乘粒子统计权重 (Fair-Game Weight Lost)",
                        description=(
                            "PST 图分析表明：计分累加节点缺少前置的 [WeightFlow] 粒子统计权重入度边。"
                            "物理意图已声明开启深穿透/减方差偏倚，粒子经历分裂或轮盘赌后带有动态权重，直接累加计数 "
                            "会导致宏观通量结果严重高估，破坏公平博弈输运无偏性不变式。"
                        ),
                        violating_node_id=acc.node_id,
                        code_evidence=acc.code_evidence,
                        line_range=list(acc.line_range),
                        remedy="在累加计分时严格乘入动态粒子权重：gFlux += step->GetTrack()->GetWeight();",
                        proposed_replacement=repl,
                        diff=_build_simple_diff(acc.code_evidence, repl),
                    ))

    def _check_phase_space_measure(self, graph: PSTGraph, violations: List[PSTViolation], intent: Optional[PhysicsIntent] = None) -> None:
        """
        断言 2: 点辐射源动量抽样节点必须具备各向同性测度 SolidAngle_4Pi。
        若其测度为 FixedPencilBeam，说明点源退化为单向笔束，立体角几何测度丢失。
        注意：若物理意图明确声明为定向束流 (DirectionalBeam)，则定轴笔束属于合法设定，不予报警。
        """
        sample_nodes = graph.find_nodes_by_action(PhysicalAction.SAMPLE)
        for smp in sample_nodes:
            if smp.quantity == "Direction":
                # 判定是否要求各向同性立体角
                if intent:
                    if intent.source_symmetry == SourceSymmetry.DIRECTIONAL_BEAM:
                        # 用户明确指定为定向束流/笔束，单向发射完全合法
                        continue
                    is_point_source = (intent.source_symmetry == SourceSymmetry.ISOTROPIC_4PI) or smp.attributes.get("is_point_source", False)
                else:
                    is_point_source = smp.attributes.get("is_point_source", False)

                if smp.metric == "DegeneratePolarMeasure" or (smp.metric == "FixedPencilBeam" and is_point_source):
                    m_gun = re.search(r"(\w+(?:->|\.))\s*SetParticleMomentumDirection", smp.code_evidence)
                    gun_var = m_gun.group(1) if m_gun else ("gun." if "gun." in smp.code_evidence else "fGun->")
                    repl = f"{gun_var}SetParticleMomentumDirection(G4RandomDirection());"
                    desc = (
                        "PST 图分析表明：粒子发射动量抽样节点 [Direction] 的物理测度为线性极角抽样 [DegeneratePolarMeasure] 或固定单向平行笔束 [FixedPencilBeam]。"
                        "在点放射源或 4π 各向同性探测实验中，动量发射必须在空间内按 dOmega = sin(theta) dtheta dphi 均匀分布。"
                        "线性抽样极角 theta 或退化单向向量会导致两极粒子密度严重汇聚虚高或几何张角虚大，破坏相空间测度不变式。"
                    )
                    violations.append(PSTViolation(
                        rule_id="PST-INV-MEASURE-01",
                        domain=ConservationDomain.PHASE_SPACE,
                        severity="CRITICAL",
                        defect_name="辐射源立体角测度退化 (Degenerate Angular Measure)",
                        description=desc,
                        violating_node_id=smp.node_id,
                        code_evidence=smp.code_evidence,
                        line_range=list(smp.line_range),
                        remedy="改用 4pi 各向同性立体角抽样：gun->SetParticleMomentumDirection(G4RandomDirection()); 或 cosTheta = 2.0*G4UniformRand() - 1.0;",
                        proposed_replacement=repl,
                        diff=_build_simple_diff(smp.code_evidence, repl),
                    ))

    def _check_causal_timing(self, graph: PSTGraph, violations: List[PSTViolation], intent: Optional[PhysicsIntent] = None) -> None:
        """
        断言 3: 飞行时间 (TOF) 或事件到达时钟节点必须读取绝对全局时钟 AbsoluteLabClock。
        若读取了 LocalTrackLifetime (GetLocalTime)，粒子跨边界或产生次级时局域时钟清零，时序因果链断裂。
        """
        fetch_nodes = graph.find_nodes_by_action(PhysicalAction.FETCH)
        is_tof_declared = (intent and (intent.timing_clock == TimingClock.GLOBAL_LAB_CLOCK or intent.observable == ObservableType.TIME_OF_FLIGHT)) or any(n.quantity == "TimeOfFlightTally" for n in graph.nodes)
        raw_c = graph.attributes.get("raw_code", "")
        for fn in fetch_nodes:
            is_tof = (
                is_tof_declared
                or any(k in fn.code_evidence.lower() for k in ("tof", "flight", "time_of_flight"))
                or any(k in fn.code_evidence for k in ("/ ns", "/ ps", "/ us", "/ s", "/ ms", "Fill("))
                or (any(k in raw_c for k in ["/ ns", "/ ps", "/ us"]) and "GetLocalTime" in fn.code_evidence)
            )
            if (fn.quantity == "LocalTrackTime" or fn.metric == "LocalTrackLifetime") and is_tof:
                repl = fn.code_evidence.replace("GetLocalTime()", "GetGlobalTime()")
                violations.append(PSTViolation(
                    rule_id="PST-INV-CAUSALITY-01",
                    domain=ConservationDomain.CAUSALITY,
                    severity="CRITICAL",
                    defect_name="飞行时间时序测量误读局域时钟 (LocalTime Clock Misuse)",
                    description=(
                        "PST 图分析表明：实验时钟提取节点读取了粒子局域时钟 [LocalTrackLifetime] (GetLocalTime)。"
                        "在 Geant4 输运因果体系中，GetLocalTime() 仅记录粒子自当前径迹诞生起的局部寿命，粒子跨越边界或产生次级时极易被重置。"
                        "测量粒子从源项到探测器的绝对飞行时间 (TOF) 必须采用绝对实验室全局时钟 (GetGlobalTime)。"
                    ),
                    violating_node_id=fn.node_id,
                    code_evidence=fn.code_evidence,
                    line_range=list(fn.line_range),
                    remedy="改用绝对实验室全局时钟：step->GetPostStepPoint()->GetGlobalTime();",
                    proposed_replacement=repl,
                    diff=_build_simple_diff(fn.code_evidence, repl),
                ))

    def _check_boundary_lifecycle(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 4: 几何界面跨越判定必须在 PostStepPoint 上提取 fGeomBoundary。
        若在 PreStepPoint 上查询边界状态，由于导航器状态后置更新，会导致边界事件无法捕获或错判。
        """
        fetch_nodes = graph.find_nodes_by_action(PhysicalAction.FETCH)
        for fn in fetch_nodes:
            if fn.quantity == "BoundaryCrossing" and fn.metric == "PreStepPointBoundary":
                repl = fn.code_evidence.replace("pre->GetStepStatus()", "post->GetStepStatus()").replace("PreStepPoint", "PostStepPoint")
                violations.append(PSTViolation(
                    rule_id="PST-INV-LIFECYCLE-01",
                    domain=ConservationDomain.LIFECYCLE,
                    severity="CRITICAL",
                    defect_name="几何边界状态在前步点错位查询 (Boundary Status on PreStepPoint)",
                    description=(
                        "PST 图分析表明：界面跨越检查节点 [BoundaryCrossing] 绑定在 PreStepPoint 状态上。"
                        "在 Geant4 步进生命周期中，fGeomBoundary 仅由几何导航器标定在后步点 (PostStepPoint)。"
                        "在 PreStepPoint 上判定界面跨越会导致出射漏出判定恒为 false，能谱积分严重失真。"
                    ),
                    violating_node_id=fn.node_id,
                    code_evidence=fn.code_evidence,
                    line_range=list(fn.line_range),
                    remedy="将边界状态判定切换至后步点：if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary)",
                    proposed_replacement=repl,
                    diff=_build_simple_diff(fn.code_evidence, repl),
                ))

    def _check_energy_resolution_broadening(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 5: 探测器高斯能量展宽必须将 FWHM 除以 2.35482 转化为标准差 sigma。
        若直接令 sigma = FWHM，展宽方差被放大 2.355 倍，严重畸变全能峰形。
        """
        convert_nodes = graph.find_nodes_by_action(PhysicalAction.CONVERT)
        for cn in convert_nodes:
            if cn.quantity == "GaussianBroadening" and cn.metric == "FWHMEqualsSigma":
                repl = "double sigma = fwhm / 2.35482;"
                violations.append(PSTViolation(
                    rule_id="PST-INV-MEASURE-02",
                    domain=ConservationDomain.PHASE_SPACE,
                    severity="CRITICAL",
                    defect_name="能谱高斯展宽混淆半高全宽与标准差 (FWHM vs Sigma Confusion)",
                    description=(
                        "PST 图分析表明：高斯展宽节点 [GaussianBroadening] 直接将 FWHM 赋值给高斯抽样标准差 sigma。"
                        "高斯分布数学测度规定 FWHM = 2 * sqrt(2 * ln 2) * sigma ≈ 2.35482 * sigma。"
                        "若未除以 2.35482，能谱展宽宽度被错误放大 2.355 倍，破坏探测器物理能量分辨率测度。"
                    ),
                    violating_node_id=cn.node_id,
                    code_evidence=cn.code_evidence,
                    line_range=list(cn.line_range),
                    remedy="严格将半高宽折算为标准差：double sigma = fwhm / 2.35482;",
                    proposed_replacement=repl,
                    diff=_build_simple_diff(cn.code_evidence, repl),
                ))

    def _check_fano_charge_fluctuation(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 6: 半导体探测器电荷载流子统计起伏方差必须乘入 Fano 因子 (Si 中 F ≈ 0.115)。
        若未乘入 Fano 因子采用纯泊松起伏 var = N，会导致本征电荷信号方差虚高约 9 倍。
        """
        convert_nodes = graph.find_nodes_by_action(PhysicalAction.CONVERT)
        for cn in convert_nodes:
            if cn.quantity == "ChargeFluctuation" and cn.metric == "PoissonVarianceWithoutFano":
                repl = "double Fano = 0.115;\n  double var_N = Fano * N;"
                violations.append(PSTViolation(
                    rule_id="PST-INV-VARIANCE-02",
                    domain=ConservationDomain.VARIANCE,
                    severity="CRITICAL",
                    defect_name="半导体电荷产生涨落缺失 Fano 因子 (Missing Fano Factor)",
                    description=(
                        "PST 图分析表明：电荷涨落计算节点 [ChargeFluctuation] 采用泊松假定 var_N = N。"
                        "在半导体晶体微观离化过程中，载流子对产生并非独立随机事件，受总能量守恒强相关抑制。"
                        "实际本征方差必须乘入材料 Fano 因子 (Si 中 F ≈ 0.115)，否则理论涨落方差被虚夸约 9 倍。"
                    ),
                    violating_node_id=cn.node_id,
                    code_evidence=cn.code_evidence,
                    line_range=list(cn.line_range),
                    remedy="在电荷涨落方差中乘入 Fano 因子：double var_N = Fano * N; // Si 中 Fano ≈ 0.115",
                    proposed_replacement=repl,
                    diff=_build_simple_diff(cn.code_evidence, repl),
                ))

    def _check_dead_layer_isolation(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 7: 探测器死层 (Dead Layer) 能量沉积严禁计入有效探测信号。
        死层属于非活性复合区，死层沉积能量与灵敏区能量相加会导致探测效率虚假升高。
        """
        accumulate_nodes = graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        for an in accumulate_nodes:
            if an.quantity == "SensitiveRegionHit" and an.metric == "DeadLayerContaminatedHit":
                repl = "if (g_eCry > 0.) g_any[g_cur]++;"
                violations.append(PSTViolation(
                    rule_id="PST-INV-LIFECYCLE-02",
                    domain=ConservationDomain.LIFECYCLE,
                    severity="CRITICAL",
                    defect_name="死层非活性区能量沉积混入灵敏信号 (Dead Layer Energy Leak)",
                    description=(
                        "PST 图分析表明：信号判选节点 [SensitiveRegionHit] 将死层能量 (eDead) 并入了有效探测脉冲判据。"
                        "死层在半导体/闪烁探测器中属于电荷无法收集的非活性物理区域，不产生输出电信号。"
                        "将死层能量混入探测信号会导致死层厚度扫描实验中效率衰减特征完全失效。"
                    ),
                    violating_node_id=an.node_id,
                    code_evidence=an.code_evidence,
                    line_range=list(an.line_range),
                    remedy="将死层能量从有效信号判据中隔离：if (g_eCry > 0.) g_any[g_cur]++;",
                    proposed_replacement=repl,
                    diff=_build_simple_diff(an.code_evidence, repl),
                ))

    def _check_pulsed_dose_rate_timing(self, graph: PSTGraph, violations: List[PSTViolation], intent: Optional[PhysicsIntent] = None) -> None:
        """
        断言 8: 高剂量率脉冲束流剂量率计算必须以微观脉冲持续时间 (如 5*us) 为分母。
        若以默认宏观周期 (如 1.0 s) 为分母，会导致瞬态脉冲剂量率被低估数十万倍。
        """
        convert_nodes = graph.find_nodes_by_action(PhysicalAction.CONVERT)
        for cn in convert_nodes:
            if cn.quantity == "PulseDoseRate" and cn.metric == "MacroscopicTimeDenominator":
                repl = "double dose_rate = (gEdep / joule) / (5.0 * us);"
                violations.append(PSTViolation(
                    rule_id="PST-INV-CAUSALITY-02",
                    domain=ConservationDomain.CAUSALITY,
                    severity="CRITICAL",
                    defect_name="高剂量率脉冲剂量率错误除以宏观秒级时钟 (Macroscopic Pulse Time Misuse)",
                    description=(
                        "PST 图分析表明：脉冲吸收剂量率节点 [PulseDoseRate] 分母采用宏观 1.0 秒时钟而非微观脉冲宽度。"
                        "FLASH 放疗或脉冲加速器束流单脉冲持续时间通常为微秒级 (如 5 us)。"
                        "未除以瞬态脉冲宽度会导致中心轴脉冲峰值吸收剂量率被严重低估 200,000 倍。"
                    ),
                    violating_node_id=cn.node_id,
                    code_evidence=cn.code_evidence,
                    line_range=list(cn.line_range),
                    remedy="使用微观脉冲宽度归一化：double dose_rate = (gEdep / joule) / (5.0 * us);",
                    proposed_replacement=repl,
                    diff=_build_simple_diff(cn.code_evidence, repl),
                ))

    def _check_inter_event_lifecycle_reset(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 9: 事件级累加容器必须在 BeginOfEventAction 中显式重置 (Inter-Event Stale Tally Accumulation)。
        若未重置，上一事件的残留计分将污染后续事件，导致多事件模拟产额与能谱严重畸变。
        """
        accumulate_nodes = graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        reset_nodes = [n for n in graph.nodes if n.quantity == "LifecycleReset" and n.attributes.get("tier") == "Event"]
        reset_vars = {n.attributes.get("reset_var", "").lower() for n in reset_nodes}

        for acc in accumulate_nodes:
            tier = acc.scope_context.get("lifecycle_tier", "")
            lhs = acc.attributes.get("lhs", "")
            # 若是在 EventAction 中累加，或被标记为跨文件事件级累加
            is_event_acc = (tier == "Event") or any(k in lhs.lower() for k in ("eventedep", "totalevent", "fevent", "totaledep", "ftotaledep"))
            if is_event_acc or acc.attributes.get("missing_event_reset"):
                var_key = lhs.split("->")[-1].strip().lower()
                has_reset = any(var_key in rv or rv in var_key for rv in reset_vars if rv) or any(
                    e.target_id == acc.node_id and e.edge_type == PSTEdgeType.LIFECYCLE_RESET for e in graph.edges
                )
                if not has_reset and not acc.attributes.get("cross_module_call"):
                    repl = f"void EventAction::BeginOfEventAction(const G4Event*) {{\n  {lhs} = 0.0;\n}}"
                    violations.append(PSTViolation(
                        rule_id="PST-INV-LIFECYCLE-03",
                        domain=ConservationDomain.LIFECYCLE,
                        severity="CRITICAL",
                        defect_name="事件级累加器未在 BeginOfEventAction 重置 (Inter-Event Stale Tally)",
                        description=(
                            f"PST 图分析表明：事件级累加变量 [{lhs}] 缺少来自 [BeginOfEventAction] 的 [LifecycleReset] 入度重置边。"
                            "在 Geant4 事件循环中，事件级容器必须在每个事件开始时显式清零。"
                            "未清零会导致前序事件的能量沉积或粒子计数残留污染后续事件，模拟结果严重失真虚高。"
                        ),
                        violating_node_id=acc.node_id,
                        code_evidence=acc.code_evidence,
                        line_range=list(acc.line_range),
                        remedy=f"在 BeginOfEventAction 中显式清零该累加器：{lhs} = 0.0;",
                        proposed_replacement=repl,
                        diff=_build_simple_diff(acc.code_evidence, repl),
                    ))

    def _check_measure_basis_invariance(self, graph: PSTGraph, violations: List[PSTViolation], intent: Optional[PhysicsIntent] = None) -> None:
        """
        断言 10: 控制流四维测度积分代数基完备性判定 (Four-Dimensional Measure-Theoretic Basis Invariance)
        数学本质：I = ∫ f dμ，在测度论上闭环拦截四种正交失效：
        1. SUPPORT (测度支撑域对齐 · 条件槽位): 界面通量必须在 ∂V 边界判定，严禁 PreStepPoint 或体内部点；
        2. MULTIPLICITY (示性测度单射性 · 次数槽位): 次级粒子产生位置/产额计分必须满足 step == 1 或集合去重；
        3. FIBRATION (纤维化层级拓扑重置 · 层级槽位): 事件级累加器跨事件必须显式重置清零；
        4. FILTRATION (鞅流因果时序单调性 · 顺序槽位): 初态物理量取值必须先于相互作用过程状态修改。
        """
        abs_profile_data = graph.attributes.get("abstract_profile", {})

        # 1. SUPPORT (支撑集门控检验)
        support_viols = abs_profile_data.get("support_violations", [])
        for sv in support_viols:
            if sv.get("type") == "PreStepBoundaryMisalignment":
                if not any(v.rule_id == "PST-INV-LIFECYCLE-01" for v in violations):
                    line = sv.get("line", 1)
                    code = sv.get("code", "")
                    repl = code.replace("pre->", "post->").replace("PreStepPoint", "PostStepPoint")
                    violations.append(PSTViolation(
                        rule_id="PST-INV-CONTROL-SUPPORT-01",
                        domain=ConservationDomain.CONTROL_FLOW,
                        severity="CRITICAL",
                        defect_name="界面通量统计测度支撑域错位 (Measure Support Misalignment)",
                        description=(
                            "控制流测度基分析表明：积分域 supp(μ) ≠ A_target。"
                            "界面通量或跨界计分使用了前步点 (PreStepPoint)，未对齐界面支撑域 ∂V。"
                            "导致出射漏出判定失效，破坏了界面测度积分的支撑集不变式。"
                        ),
                        violating_node_id=None,
                        code_evidence=code,
                        line_range=[line, line],
                        remedy="将边界判定对齐至后步点：step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary;",
                        proposed_replacement=repl,
                        diff=_build_simple_diff(code, repl),
                    ))

        # 2. MULTIPLICITY (示性测度重度倍乘检验)
        mult_viols = abs_profile_data.get("multiplicity_violations", [])
        for mv in mult_viols:
            line = mv.get("line", 1)
            code = mv.get("code", "")
            repl = f"if (step->GetTrack()->GetCurrentStepNumber() == 1) {{\n    {code}\n}}"
            violations.append(PSTViolation(
                rule_id="PST-INV-CONTROL-MULTIPLICITY-01",
                domain=ConservationDomain.CONTROL_FLOW,
                severity="CRITICAL",
                defect_name="次级粒子生成示性测度重度倍乘 (Multiplicity Inflation)",
                description=(
                    "控制流测度基分析表明：单射路径上示性函数重度 k > 1 (dμ → k·dμ)。"
                    "次级粒子产生位置或产额计数在步进循环中缺乏单步单射守卫 (如 stepNumber == 1) 或集合去重。"
                    "次级粒子在后续数百个步进中会被重复累计，导致微观反应产额严重倍乘虚高。"
                ),
                violating_node_id=None,
                code_evidence=code,
                line_range=[line, line],
                remedy="添加单步示性守卫：if (step->GetTrack()->GetCurrentStepNumber() == 1) { ... }",
                proposed_replacement=repl,
                diff=_build_simple_diff(code, repl),
            ))

        # 3. FILTRATION (鞅流因果滤波逆向检验)
        filt_viols = abs_profile_data.get("filtration_violations", [])
        for fv in filt_viols:
            line = fv.get("line", 1)
            code = fv.get("code", "")
            var_name = fv.get("var", "eKin")
            repl = f"// 取值必须在相互作用前完成：\n// G4double {var_name} = step->GetPreStepPoint()->GetKineticEnergy();"
            violations.append(PSTViolation(
                rule_id="PST-INV-CONTROL-FILTRATION-01",
                domain=ConservationDomain.CONTROL_FLOW,
                severity="CRITICAL",
                defect_name="鞅信息流因果滤波时序逆向 (Filtration Inversion)",
                description=(
                    "控制流测度基分析表明：物理观测量关于随机自然时序流 F_t 发生非因果引用 (t_fetch > t_event)。"
                    "初态物理量 (如初态动能) 的提取发生在相互作用过程状态修改之后，或读取了末态步点。"
                    "违背了随机过程鞅因果前驱单调性公理。"
                ),
                violating_node_id=None,
                code_evidence=code,
                line_range=[line, line],
                remedy="在相互作用过程触发前或从 PreStepPoint 提前提取初态能量物理量。",
                proposed_replacement=repl,
                diff=_build_simple_diff(code, repl),
            ))
