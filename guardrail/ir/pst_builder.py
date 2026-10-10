#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/pst_builder.py —— 物理语义树构建器 (Physics Semantic Tree Builder)

基于 tree-sitter C++ AST 语法提取、局部上下文增强与数据流依赖追踪，
将 C++ 蒙卡输运代码确定性提升为物理语义树 (PSTGraph)。
支持人类手写代码与大模型生成代码的双向形式化核验。
"""

from __future__ import annotations

import os
import re
from typing import Dict, List, Optional, Tuple, Any, Set
import tree_sitter_cpp as tscpp
from tree_sitter import Language, Parser, Node

from guardrail.ir.pst_schema import (
    PhysicalAction,
    ConservationDomain,
    PSTEdgeType,
    PSTNode,
    PSTEdge,
    PSTGraph,
)


class PSTBuilder:
    """
    物理语义树提取与组装引擎 (PST Builder)
    实现技术路线 L1 (AST 解析) -> L2 (上下文增强) -> L3 (物理语义映射) -> L4 (PST 图组装)
    """

    def __init__(self):
        self.language = Language(tscpp.language())
        self.parser = Parser(self.language)

    def parse_source(self, code_bytes: bytes) -> Node:
        """L1: 解析 C++ 源码生成 AST"""
        tree = self.parser.parse(code_bytes)
        return tree.root_node

    def build_from_file(self, filepath: str, graph_id: Optional[str] = None) -> PSTGraph:
        """从 C++ 源码文件直接构建 PSTGraph"""
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()
        return self.build(content, filename=filepath, graph_id=graph_id)

    def build(self, source_code: str, filename: str = "code.cc", graph_id: Optional[str] = None) -> PSTGraph:
        """主入口：将 C++ 源码字符串转化为完整的 PSTGraph"""
        code_bytes = source_code.encode("utf-8")
        root = self.parse_source(code_bytes)

        gid = graph_id or f"pst_{os.path.basename(filename).replace('.', '_')}"
        graph = PSTGraph(graph_id=gid, source_file=filename)

        # 1. 结构化 AST 递归遍历与上下文采集
        nodes_collected: List[PSTNode] = []
        var_assignments: Dict[str, str] = {}  # var_name -> producing_node_id
        var_types: Dict[str, str] = {}        # var_name -> semantic_type (e.g. "TrackWeight")

        self._traverse(
            node=root,
            code_bytes=code_bytes,
            context={"class": None, "method": None, "condition": None},
            nodes_collected=nodes_collected,
            var_assignments=var_assignments,
            var_types=var_types,
        )

        for n in nodes_collected:
            graph.add_node(n)

        # 2. L4: 基于数据流与因果时序组装 PST 依赖边 (Edges)
        self._build_edges(graph, var_assignments, var_types, nodes_collected)

        return graph

    def _traverse(
        self,
        node: Node,
        code_bytes: bytes,
        context: Dict[str, Any],
        nodes_collected: List[PSTNode],
        var_assignments: Dict[str, str],
        var_types: Dict[str, str],
    ) -> None:
        """递归遍历 AST 并维护作用域与语义标注"""
        node_type = node.type

        # L2: 作用域上下文追踪
        if node_type == "class_specifier":
            name_node = node.child_by_field_name("name")
            class_name = self._node_text(name_node, code_bytes) if name_node else "AnonymousClass"
            context = {**context, "class": class_name}

        elif node_type == "function_definition":
            declarator = node.child_by_field_name("declarator")
            method_name = self._extract_function_name(declarator, code_bytes) or "AnonymousFunction"
            context = {**context, "method": method_name}

        elif node_type == "if_statement":
            cond_node = node.child_by_field_name("condition")
            cond_text = self._node_text(cond_node, code_bytes) if cond_node else ""
            context = {**context, "condition": cond_text}

        # L3: 物理语义映射 (识别粒子源、步进抓取、计分累加与量纲转换)
        self._match_physical_semantics(node, code_bytes, context, nodes_collected, var_assignments, var_types)

        # 递归遍历子节点
        for child in node.children:
            self._traverse(child, code_bytes, context, nodes_collected, var_assignments, var_types)

    def _match_physical_semantics(
        self,
        node: Node,
        code_bytes: bytes,
        context: Dict[str, Any],
        nodes_collected: List[PSTNode],
        var_assignments: Dict[str, str],
        var_types: Dict[str, str],
    ) -> None:
        """对具体的 AST 节点执行微观物理算子识别与规约映射"""
        text = self._node_text(node, code_bytes).strip()
        lines = (node.start_point.row + 1, node.end_point.row + 1)

        # ── 1. 动量角分布抽样 (Sample: Direction) ──
        if node.type == "call_expression" and "SetParticleMomentumDirection" in text:
            nid = f"n_sample_dir_{len(nodes_collected)+1}"
            is_4pi_iso = "G4RandomDirection" in text or ("cosTheta" in text and "sinTheta" in text) or ("twopi" in text and "G4UniformRand" in text)
            _num = r"[-+]?(?:[0-9]+\.?[0-9]*|[0-9]*\.[0-9]+)"
            is_pencil = bool(re.search(rf"G4ThreeVector\s*\(\s*{_num}\s*,\s*{_num}\s*,\s*{_num}\s*\)", text))

            # 上下文感知：精确检测是否为未准直的点放射源场景 (若显式声明为准直/平行束/垂直入射则不属于各向同性点源)
            full_context_str = code_bytes.decode("utf-8", errors="replace")
            is_collimated = bool(re.search(r"(?:准直|collimat|pencil|平行束|垂直入射|正入射)", full_context_str, re.IGNORECASE))
            is_radioactive_point_source = bool(re.search(
                r"(?:Cs-?137|Co-?60|Na-?22|Am-?241|Eu-?152|Ba-?133|\bpoint\s*source\b|\b点源\b|\bisotrop|\b各向同性\b)",
                full_context_str,
                re.IGNORECASE,
            ))
            is_point_source_context = is_radioactive_point_source and not is_collimated

            metric = "SolidAngle_4Pi" if is_4pi_iso else ("FixedPencilBeam" if is_pencil else "CustomDistribution")
            invariant = "dOmega = sin(theta) dtheta dphi (Uniform Sphere)" if is_4pi_iso else "DirectionalBeam"

            nodes_collected.append(PSTNode(
                node_id=nid,
                action=PhysicalAction.SAMPLE,
                domain=ConservationDomain.PHASE_SPACE,
                quantity="Direction",
                metric=metric,
                expected_invariant=invariant,
                code_evidence=text,
                line_range=lines,
                ast_node_type=node.type,
                scope_context=dict(context),
                attributes={"is_point_source": is_point_source_context},
            ))

        # ── 2. 统计权重抓取 (Fetch: TrackWeight) ──
        elif node.type == "call_expression" and ("GetWeight" in text or "fWeight" in text):
            nid = f"n_fetch_weight_{len(nodes_collected)+1}"
            nodes_collected.append(PSTNode(
                node_id=nid,
                action=PhysicalAction.FETCH,
                domain=ConservationDomain.VARIANCE,
                quantity="TrackWeight",
                metric="DynamicWeight",
                expected_invariant="WeightPreserved",
                code_evidence=text,
                line_range=lines,
                ast_node_type=node.type,
                scope_context=dict(context),
            ))
            # 记录当前语句所属变量
            assigned_var = self._find_enclosing_assigned_var(node, code_bytes)
            if assigned_var:
                var_assignments[assigned_var] = nid
                var_types[assigned_var] = "TrackWeight"

        # ── 3. 实验时钟源抓取 (Fetch: GlobalTime vs LocalTime) ──
        elif node.type == "call_expression" and ("GetGlobalTime" in text or "GetLocalTime" in text):
            is_global = "GetGlobalTime" in text
            nid = f"n_fetch_time_{len(nodes_collected)+1}"
            metric = "AbsoluteLabClock" if is_global else "LocalTrackLifetime"
            inv = "t_arrival == GetGlobalTime()" if is_global else "FAILS: LocalTime resets across boundaries"

            nodes_collected.append(PSTNode(
                node_id=nid,
                action=PhysicalAction.FETCH,
                domain=ConservationDomain.CAUSALITY,
                quantity="GlobalTime" if is_global else "LocalTrackTime",
                metric=metric,
                expected_invariant=inv,
                code_evidence=text,
                line_range=lines,
                ast_node_type=node.type,
                scope_context=dict(context),
            ))
            assigned_var = self._find_enclosing_assigned_var(node, code_bytes)
            if assigned_var:
                var_assignments[assigned_var] = nid
                var_types[assigned_var] = "GlobalTime" if is_global else "LocalTrackTime"

        # ── 4. 几何界面穿越生命周期抓取 (Fetch: BoundaryCrossing) (T1-4) ──
        elif node.type == "call_expression" and "GetStepStatus" in text:
            # 判断是在 PreStepPoint 还是 PostStepPoint 上调用
            parent_text = self._node_text(node.parent, code_bytes) if node.parent else text
            is_pre = bool(re.search(r"\b(?:pre|PreStepPoint)\b", text) or re.search(r"\b(?:pre|PreStepPoint)\b.*GetStepStatus", parent_text))
            is_post = bool(re.search(r"\b(?:post|PostStepPoint)\b", text) or re.search(r"\b(?:post|PostStepPoint)\b.*GetStepStatus", parent_text))

            metric = "PreStepPointBoundary" if is_pre else ("PostStepPointBoundary" if is_post else "UnknownBoundaryPoint")
            nid = f"n_boundary_{len(nodes_collected)+1}"
            nodes_collected.append(PSTNode(
                node_id=nid,
                action=PhysicalAction.FETCH,
                domain=ConservationDomain.LIFECYCLE,
                quantity="BoundaryCrossing",
                metric=metric,
                expected_invariant="PostStepPointBoundaryRequired",
                code_evidence=parent_text.strip(),
                line_range=lines,
                ast_node_type=node.type,
                scope_context=dict(context),
            ))

        # ── 5. 能量沉积抓取 (Fetch: EnergyDeposit) ──
        elif node.type == "call_expression" and "GetTotalEnergyDeposit" in text:
            nid = f"n_fetch_edep_{len(nodes_collected)+1}"
            nodes_collected.append(PSTNode(
                node_id=nid,
                action=PhysicalAction.FETCH,
                domain=ConservationDomain.PHASE_SPACE,
                quantity="EnergyDeposit",
                metric="TotalEnergyDeposit",
                expected_invariant="ConservationOfEnergy",
                code_evidence=text,
                line_range=lines,
                ast_node_type=node.type,
                scope_context=dict(context),
            ))
            assigned_var = self._find_enclosing_assigned_var(node, code_bytes)
            if assigned_var:
                var_assignments[assigned_var] = nid
                var_types[assigned_var] = "EnergyDeposit"

        # ── 6. 声明与量纲转换 (Convert / Define) ──
        elif node.type in ("init_declarator", "assignment_expression"):
            lhs_node = node.child_by_field_name("declarator") or node.child_by_field_name("left")
            rhs_node = node.child_by_field_name("value") or node.child_by_field_name("right")
            lhs_text = self._node_text(lhs_node, code_bytes).strip() if lhs_node else ""
            rhs_text = self._node_text(rhs_node, code_bytes).strip() if rhs_node else ""

            # (A) 探测器高斯展宽 FWHM 与标准差转换 (Convert: GaussianBroadening) (T4-5)
            if lhs_text == "sigma" and "fwhm" in rhs_text.lower():
                nid = f"n_broaden_{len(nodes_collected)+1}"
                has_factor = bool(re.search(r"2\.35[45]|2\.35482", rhs_text))
                metric = "FWHMConvertedToSigma" if has_factor else "FWHMEqualsSigma"
                inv = "sigma == fwhm / 2.35482"

                nodes_collected.append(PSTNode(
                    node_id=nid,
                    action=PhysicalAction.CONVERT,
                    domain=ConservationDomain.PHASE_SPACE,
                    quantity="GaussianBroadening",
                    metric=metric,
                    expected_invariant=inv,
                    code_evidence=text,
                    line_range=lines,
                    ast_node_type=node.type,
                    scope_context=dict(context),
                    attributes={"lhs": lhs_text, "rhs": rhs_text, "has_fwhm_factor": has_factor},
                ))

            # (B) 半导体载流子方差 Fano 因子修正 (Convert: ChargeFluctuation) (T4-8)
            elif ("var_n" in lhs_text.lower() or "charge_var" in lhs_text.lower() or "varn" in lhs_text.lower()) and "n" in rhs_text.lower():
                nid = f"n_fano_{len(nodes_collected)+1}"
                has_fano = bool(re.search(r"\bfano\b|0\.115|0\.08", rhs_text, re.IGNORECASE))
                metric = "FanoSuppressedVariance" if has_fano else "PoissonVarianceWithoutFano"
                inv = "var_N == Fano * N"

                nodes_collected.append(PSTNode(
                    node_id=nid,
                    action=PhysicalAction.CONVERT,
                    domain=ConservationDomain.VARIANCE,
                    quantity="ChargeFluctuation",
                    metric=metric,
                    expected_invariant=inv,
                    code_evidence=text,
                    line_range=lines,
                    ast_node_type=node.type,
                    scope_context=dict(context),
                    attributes={"lhs": lhs_text, "rhs": rhs_text, "has_fano": has_fano},
                ))

            # (C) 脉冲吸收剂量率时间归一化 (Convert: PulseDoseRate) (T2-5)
            elif ("dose_rate" in lhs_text.lower() or "doserate" in lhs_text.lower()) and "edep" in rhs_text.lower():
                nid = f"n_pulse_dose_{len(nodes_collected)+1}"
                has_micro_pulse = bool(re.search(r"\*\s*(?:us|ns|ms)\b", rhs_text))
                metric = "MicroscopicPulseDenominator" if has_micro_pulse else "MacroscopicTimeDenominator"
                inv = "DividedByPulseDuration (e.g. 5.0*us)"

                nodes_collected.append(PSTNode(
                    node_id=nid,
                    action=PhysicalAction.CONVERT,
                    domain=ConservationDomain.CAUSALITY,
                    quantity="PulseDoseRate",
                    metric=metric,
                    expected_invariant=inv,
                    code_evidence=text,
                    line_range=lines,
                    ast_node_type=node.type,
                    scope_context=dict(context),
                    attributes={"lhs": lhs_text, "rhs": rhs_text, "has_pulse_time": has_micro_pulse},
                ))

            # (D) 常规物理计分累加 (Accumulate: Fluence / PulseHeight / TOF)
            # 过滤掉初始声明零赋值 (如 static double gFlux = 0.0;)
            is_zero_init = (node.type == "init_declarator") and rhs_text in ("0", "0.0", "0.", "0.0f", "false", "nullptr", "NULL")
            if not is_zero_init and any(k in lhs_text.lower() for k in ("flux", "tally", "edep", "dose", "count", "tof", "yield")):
                nid = f"n_accumulate_{len(nodes_collected)+1}"
                quantity = "FluenceScore" if "flux" in lhs_text.lower() else ("PulseHeight" if "edep" in lhs_text.lower() else "TimeOfFlightTally")

                nodes_collected.append(PSTNode(
                    node_id=nid,
                    action=PhysicalAction.ACCUMULATE,
                    domain=ConservationDomain.VARIANCE if "flux" in lhs_text.lower() else (ConservationDomain.CAUSALITY if "tof" in lhs_text.lower() else ConservationDomain.PHASE_SPACE),
                    quantity=quantity,
                    metric="ScoreTally",
                    code_evidence=text,
                    line_range=lines,
                    ast_node_type=node.type,
                    scope_context=dict(context),
                    attributes={"lhs": lhs_text, "rhs": rhs_text},
                ))

        # ── 7. 灵敏区与死层信号判选 (Accumulate: SensitiveRegionHit) (T4-4) ──
        elif node.type == "if_statement":
            cond_node = node.child_by_field_name("condition")
            cond_text = self._node_text(cond_node, code_bytes).strip() if cond_node else ""
            consequent_node = node.child_by_field_name("consequence")
            consequent_text = self._node_text(consequent_node, code_bytes).strip() if consequent_node else ""

            # 探测器事件有信号计数 (g_any 或 fHit)
            if any(k in consequent_text for k in ("g_any", "fHit", "fPeak", "nHit")) or "any[" in consequent_text:
                if ("eCry" in cond_text or "crystal" in cond_text) and ("eDead" in cond_text or "dead" in cond_text) and "+" in cond_text:
                    nid = f"n_sens_hit_{len(nodes_collected)+1}"
                    nodes_collected.append(PSTNode(
                        node_id=nid,
                        action=PhysicalAction.ACCUMULATE,
                        domain=ConservationDomain.LIFECYCLE,
                        quantity="SensitiveRegionHit",
                        metric="DeadLayerContaminatedHit",
                        expected_invariant="DeadLayerIsolated",
                        code_evidence=f"if {cond_text} {consequent_text}",
                        line_range=lines,
                        ast_node_type=node.type,
                        scope_context=dict(context),
                        attributes={"condition": cond_text, "is_contaminated": True},
                    ))
                elif ("eCry" in cond_text or "crystal" in cond_text) and ("+" not in cond_text or "eDead" not in cond_text):
                    nid = f"n_sens_hit_{len(nodes_collected)+1}"
                    nodes_collected.append(PSTNode(
                        node_id=nid,
                        action=PhysicalAction.ACCUMULATE,
                        domain=ConservationDomain.LIFECYCLE,
                        quantity="SensitiveRegionHit",
                        metric="ActiveRegionOnlyHit",
                        expected_invariant="DeadLayerIsolated",
                        code_evidence=f"if {cond_text} {consequent_text}",
                        line_range=lines,
                        ast_node_type=node.type,
                        scope_context=dict(context),
                        attributes={"condition": cond_text, "is_contaminated": False},
                    ))

    def _build_edges(
        self,
        graph: PSTGraph,
        var_assignments: Dict[str, str],
        var_types: Dict[str, str],
        nodes: List[PSTNode],
    ) -> None:
        """L4: 分析数据流与引用关系，连接 PST 边"""
        accumulate_nodes = graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        fetch_nodes = graph.find_nodes_by_action(PhysicalAction.FETCH)
        convert_nodes = graph.find_nodes_by_action(PhysicalAction.CONVERT)

        edge_counter = 1

        for acc in accumulate_nodes:
            evidence = acc.code_evidence
            rhs = acc.attributes.get("rhs", evidence)

            # 检查权重流依赖 (WeightFlow)
            for var_name, source_nid in var_assignments.items():
                if var_types.get(var_name) == "TrackWeight":
                    if re.search(rf"\b{re.escape(var_name)}\b", rhs) or "GetWeight" in rhs:
                        graph.add_edge(PSTEdge(
                            edge_id=f"e_weight_{edge_counter}",
                            source_id=source_nid,
                            target_id=acc.node_id,
                            edge_type=PSTEdgeType.WEIGHT_FLOW,
                            variable_name=var_name,
                        ))
                        edge_counter += 1

            # 若直接内联调用了 GetWeight()，连接对应的 Fetch 节点
            if "GetWeight" in evidence:
                for fn in fetch_nodes:
                    if fn.quantity == "TrackWeight" and fn.line_range[0] == acc.line_range[0]:
                        if not any(e.source_id == fn.node_id and e.target_id == acc.node_id for e in graph.edges):
                            graph.add_edge(PSTEdge(
                                edge_id=f"e_weight_inline_{edge_counter}",
                                source_id=fn.node_id,
                                target_id=acc.node_id,
                                edge_type=PSTEdgeType.WEIGHT_FLOW,
                                variable_name="inline_weight",
                            ))
                            edge_counter += 1

            # 检查时间因果流依赖 (CausalTime)
            for fn in fetch_nodes:
                if fn.quantity in ("GlobalTime", "LocalTrackTime") and (fn.node_id in var_assignments.values() or fn.line_range[0] == acc.line_range[0]):
                    graph.add_edge(PSTEdge(
                        edge_id=f"e_time_{edge_counter}",
                        source_id=fn.node_id,
                        target_id=acc.node_id,
                        edge_type=PSTEdgeType.CAUSAL_TIME,
                        variable_name=acc.attributes.get("lhs", "time"),
                    ))
                    edge_counter += 1

            # 检查能量沉积累加 (DataDependency)
            for fn in fetch_nodes:
                if fn.quantity == "EnergyDeposit" and (fn.node_id in var_assignments.values() or fn.line_range[0] == acc.line_range[0]):
                    graph.add_edge(PSTEdge(
                        edge_id=f"e_edep_{edge_counter}",
                        source_id=fn.node_id,
                        target_id=acc.node_id,
                        edge_type=PSTEdgeType.DATA_DEPENDENCY,
                        physical_units="MeV",
                    ))
                    edge_counter += 1

            # 检查边界门控 (BoundaryGate)
            for fn in fetch_nodes:
                if fn.quantity == "BoundaryCrossing":
                    graph.add_edge(PSTEdge(
                        edge_id=f"e_boundary_gate_{edge_counter}",
                        source_id=fn.node_id,
                        target_id=acc.node_id,
                        edge_type=PSTEdgeType.BOUNDARY_GATE,
                    ))
                    edge_counter += 1

        # 检查归一化分母边 (NormalizationDenominator)
        for cn in convert_nodes:
            if cn.quantity == "PulseDoseRate":
                for fn in fetch_nodes:
                    if fn.quantity == "EnergyDeposit":
                        graph.add_edge(PSTEdge(
                            edge_id=f"e_norm_pulse_{edge_counter}",
                            source_id=fn.node_id,
                            target_id=cn.node_id,
                            edge_type=PSTEdgeType.NORMALIZATION_DENOMINATOR,
                            physical_units="Gy/s",
                        ))
                        edge_counter += 1

    def _node_text(self, node: Optional[Node], code_bytes: bytes) -> str:
        if not node:
            return ""
        return code_bytes[node.start_byte:node.end_byte].decode("utf-8", errors="replace")

    def _extract_function_name(self, declarator_node: Optional[Node], code_bytes: bytes) -> Optional[str]:
        if not declarator_node:
            return None
        curr = declarator_node
        while curr:
            if curr.type in ("identifier", "field_identifier"):
                return self._node_text(curr, code_bytes)
            sub = curr.child_by_field_name("declarator")
            if sub:
                curr = sub
            elif curr.children:
                curr = curr.children[0]
            else:
                break
        return None

    def _find_enclosing_assigned_var(self, node: Node, code_bytes: bytes) -> Optional[str]:
        """向上追溯当前表达式被赋值给哪一个变量名"""
        curr: Optional[Node] = node.parent
        while curr and curr.type not in ("function_definition", "translation_unit"):
            if curr.type == "init_declarator":
                decl_node = curr.child_by_field_name("declarator")
                if decl_node:
                    return self._node_text(decl_node, code_bytes)
            elif curr.type == "assignment_expression":
                left_node = curr.child_by_field_name("left")
                if left_node:
                    return self._node_text(left_node, code_bytes)
            curr = curr.parent
        return None
