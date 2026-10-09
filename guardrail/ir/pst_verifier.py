#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/pst_verifier.py —— 物理语义树守恒验证器 (PST Conservation Verifier)

在 PST (Physics Semantic Tree) 中间表示图上执行图论遍历与形式化守恒断言，
将物理第一性原理映射为图拓扑结构、节点属性与守恒总线的确定性检验。
全面覆盖人类手写代码审查与 LLM 蒙卡程序生成门禁。
"""

from __future__ import annotations

from typing import Dict, List, Optional, Any
from pydantic import BaseModel, Field

from guardrail.ir.pst_schema import (
    PhysicalAction,
    ConservationDomain,
    PSTEdgeType,
    PSTNode,
    PSTEdge,
    PSTGraph,
)


class PSTViolation(BaseModel):
    """形式化物理违约结果"""
    rule_id: str = Field(description="规则唯一标识，如 PST-INV-001")
    domain: ConservationDomain = Field(description="违约所属守恒总线域")
    severity: str = Field(default="CRITICAL", description="严重等级: CRITICAL / HIGH / WARNING")
    defect_name: str = Field(description="物理缺陷名称")
    description: str = Field(description="基于物理第一性原理的机理诊断")
    violating_node_id: Optional[str] = Field(default=None, description="违约涉及的 PST 节点 ID")
    code_evidence: Optional[str] = Field(default=None, description="源码行证据")
    line_range: Optional[List[int]] = Field(default=None, description="源码行范围")
    remedy: str = Field(description="专家级规范修复建议")


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
        return "\n".join(lines)


class PSTVerifier:
    """物理语义树形式化断言引擎 (Layer 5)"""

    def verify(self, graph: PSTGraph) -> PSTVerificationReport:
        violations: List[PSTViolation] = []

        # 1. 检验公平博弈统计权重守恒 (Fair-Game Weight Invariance · T1-6)
        self._check_fair_game_weight(graph, violations)

        # 2. 检验相空间立体角测度守恒 (Phase-Space Measure Invariance · T4-1)
        self._check_phase_space_measure(graph, violations)

        # 3. 检验因果与绝对实验时钟守恒 (Causal Time Invariance · T4-6)
        self._check_causal_timing(graph, violations)

        # 4. 检验步进边界生命周期契约 (Boundary Step Lifecycle · T1-4)
        self._check_boundary_lifecycle(graph, violations)

        # 5. 检验探测器高斯能量分辨展宽 (FWHM vs Sigma · T4-5)
        self._check_energy_resolution_broadening(graph, violations)

        # 6. 检验半导体电荷载流子 Fano 涨落因子 (Fano Factor Variance · T4-8)
        self._check_fano_charge_fluctuation(graph, violations)

        # 7. 检验探测器死层与灵敏体积物理隔离 (Dead Layer Isolation · T4-4)
        self._check_dead_layer_isolation(graph, violations)

        # 8. 检验高剂量率脉冲时钟微观归一化 (Pulse Dose Rate Temporal Normalization · T2-5)
        self._check_pulsed_dose_rate_timing(graph, violations)

        passed = len(violations) == 0
        return PSTVerificationReport(
            graph_id=graph.graph_id,
            source_file=graph.source_file,
            passed=passed,
            violations_count=len(violations),
            violations=violations,
        )

    def _check_fair_game_weight(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 1: 凡涉及通量累加 (FluenceScore) 的计分节点，必须具有入度边 WeightFlow。
        若缺失 WeightFlow 入度，意味着方差缩减操作下未乘入权重，导致通量被虚假放大。
        """
        accumulate_nodes = graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        for acc in accumulate_nodes:
            if acc.quantity == "FluenceScore":
                incoming_edges = graph.get_incoming_edges(acc.node_id)
                has_weight_flow = any(e.edge_type == PSTEdgeType.WEIGHT_FLOW for e in incoming_edges)

                if not has_weight_flow:
                    violations.append(PSTViolation(
                        rule_id="PST-INV-VARIANCE-01",
                        domain=ConservationDomain.VARIANCE,
                        severity="CRITICAL",
                        defect_name="方差缩减通量计分漏乘粒子统计权重 (Fair-Game Weight Lost)",
                        description=(
                            "PST 图分析表明：计分累加节点 [FluenceScore] 缺少前置的 [WeightFlow] 入度边。"
                            "在中子/光子深穿透几何中，粒子经历分裂或轮盘赌后带有动态权重，直接累加计数 (+1.0) "
                            "会导致宏观通量结果严重高估，破坏公平博弈输运无偏性不变式。"
                        ),
                        violating_node_id=acc.node_id,
                        code_evidence=acc.code_evidence,
                        line_range=list(acc.line_range),
                        remedy="在累加计分时严格乘入动态粒子权重：gFlux += step->GetTrack()->GetWeight();",
                    ))

    def _check_phase_space_measure(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 2: 点辐射源动量抽样节点必须具备各向同性测度 SolidAngle_4Pi。
        若其测度为 FixedPencilBeam，说明点源退化为单向笔束，立体角几何测度丢失。
        """
        sample_nodes = graph.find_nodes_by_action(PhysicalAction.SAMPLE)
        for smp in sample_nodes:
            if smp.quantity == "Direction":
                is_point_source = smp.attributes.get("is_point_source", False)
                if smp.metric == "FixedPencilBeam" and is_point_source:
                    violations.append(PSTViolation(
                        rule_id="PST-INV-MEASURE-01",
                        domain=ConservationDomain.PHASE_SPACE,
                        severity="CRITICAL",
                        defect_name="点辐射源角分布退化为平行笔束 (Degenerate Pencil Beam)",
                        description=(
                            "PST 图分析表明：粒子发射动量抽样节点 [Direction] 的物理测度为 [FixedPencilBeam] (0,0,1)。"
                            "在点放射源或各向同性探测实验中，动量发射必须在 4pi 球面度空间内按 dOmega = sin(theta) dtheta dphi 均匀分布。"
                            "固定单向向量会导致探测器所张立体角被人为放大数十倍，峰效率超几何物理上限。"
                        ),
                        violating_node_id=smp.node_id,
                        code_evidence=smp.code_evidence,
                        line_range=list(smp.line_range),
                        remedy="改用 4pi 各向同性立体角抽样：gun->SetParticleMomentumDirection(G4RandomDirection());",
                    ))

    def _check_causal_timing(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 3: 飞行时间 (TOF) 或事件到达时钟节点必须读取绝对全局时钟 AbsoluteLabClock。
        若读取了 LocalTrackLifetime (GetLocalTime)，粒子跨边界或产生次级时局域时钟清零，时序因果链断裂。
        """
        fetch_nodes = graph.find_nodes_by_action(PhysicalAction.FETCH)
        for fn in fetch_nodes:
            if fn.quantity == "LocalTrackTime" or fn.metric == "LocalTrackLifetime":
                violations.append(PSTViolation(
                    rule_id="PST-INV-CAUSALITY-01",
                    domain=ConservationDomain.CAUSALITY,
                    severity="CRITICAL",
                    defect_name="飞行时间时序测量误读局域时钟 (LocalTime Clock Misuse)",
                    description=(
                        "PST 图分析表明：实验时钟提取节点读取了粒子局域时钟 [LocalTrackLifetime] (GetLocalTime)。"
                        "在 Geant4 输运因果体系中，GetLocalTime() 仅记录粒子自当前径迹诞生起的局部寿命，粒子跨越边界或产生次级时极易被重置。"
                        "测量粒子从源项到探测器的绝对飞行时间 (TOF) 必须采用绝对实验室全局时钟。"
                    ),
                    violating_node_id=fn.node_id,
                    code_evidence=fn.code_evidence,
                    line_range=list(fn.line_range),
                    remedy="改用绝对实验室全局时钟：step->GetPostStepPoint()->GetGlobalTime();",
                ))

    def _check_boundary_lifecycle(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 4: 几何界面跨越判定必须在 PostStepPoint 上提取 fGeomBoundary。
        若在 PreStepPoint 上查询边界状态，由于导航器状态后置更新，会导致边界事件无法捕获或错判。
        """
        fetch_nodes = graph.find_nodes_by_action(PhysicalAction.FETCH)
        for fn in fetch_nodes:
            if fn.quantity == "BoundaryCrossing" and fn.metric == "PreStepPointBoundary":
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
                ))

    def _check_energy_resolution_broadening(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 5: 探测器高斯能量展宽必须将 FWHM 除以 2.35482 转化为标准差 sigma。
        若直接令 sigma = FWHM，展宽方差被放大 2.355 倍，严重畸变全能峰形。
        """
        convert_nodes = graph.find_nodes_by_action(PhysicalAction.CONVERT)
        for cn in convert_nodes:
            if cn.quantity == "GaussianBroadening" and cn.metric == "FWHMEqualsSigma":
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
                ))

    def _check_fano_charge_fluctuation(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 6: 半导体探测器电荷载流子统计起伏方差必须乘入 Fano 因子 (Si 中 F ≈ 0.115)。
        若未乘入 Fano 因子采用纯泊松起伏 var = N，会导致本征电荷信号方差虚高约 9 倍。
        """
        convert_nodes = graph.find_nodes_by_action(PhysicalAction.CONVERT)
        for cn in convert_nodes:
            if cn.quantity == "ChargeFluctuation" and cn.metric == "PoissonVarianceWithoutFano":
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
                ))

    def _check_dead_layer_isolation(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 7: 探测器死层 (Dead Layer) 能量沉积严禁计入有效探测信号。
        死层属于非活性复合区，死层沉积能量与灵敏区能量相加会导致探测效率虚假升高。
        """
        accumulate_nodes = graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        for an in accumulate_nodes:
            if an.quantity == "SensitiveRegionHit" and an.metric == "DeadLayerContaminatedHit":
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
                ))

    def _check_pulsed_dose_rate_timing(self, graph: PSTGraph, violations: List[PSTViolation]) -> None:
        """
        断言 8: 高剂量率脉冲束流剂量率计算必须以微观脉冲持续时间 (如 5*us) 为分母。
        若以默认宏观周期 (如 1.0 s) 为分母，会导致瞬态脉冲剂量率被低估数十万倍。
        """
        convert_nodes = graph.find_nodes_by_action(PhysicalAction.CONVERT)
        for cn in convert_nodes:
            if cn.quantity == "PulseDoseRate" and cn.metric == "MacroscopicTimeDenominator":
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
                ))
