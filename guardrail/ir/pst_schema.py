#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/pst_schema.py —— 物理语义树 (Physics Semantic Tree, PST) 中间表示形式化规范

本模块定义面向科学计算蒙特卡罗粒子输运仿真的全新中间表示 (Intermediate Representation, IR)。
将语法层 AST（CallExpr, DeclStmt, AssignExpr）与数据流图提升为带有物理第一性原理守恒不变式语义的类型化 DAG。
"""

from __future__ import annotations

import json
from enum import Enum
from typing import Dict, List, Optional, Tuple, Any, Set
from pydantic import BaseModel, Field, ConfigDict


class PhysicalAction(str, Enum):
    """
    五大原子物理操作类型 (Five Elementary Physical Actions)
    封闭性覆盖蒙特卡罗输运中所有可观测量的产生、采样、跟踪、统计与归一化。
    """
    DEFINE = "Define"         # 物理设定：材料介质、探测器几何、死层、源项强度、截面库
    SAMPLE = "Sample"         # 随机抽样：立体角各向同性抽样、能谱反变换抽样、空间/时间抽样
    FETCH = "Fetch"           # 步进抓取：提取能量沉积、全局绝对时钟、粒子统计权重、步进状态
    ACCUMULATE = "Accumulate" # 物理计数：能谱多道累加、跨界面通量累加、符合计数、历史积分
    CONVERT = "Convert"       # 量纲归一：除以体积质量计算剂量 (Gy)、每源粒子归一化、单位变换


class ConservationDomain(str, Enum):
    """
    六大正交守恒总线域 (Six Orthogonal Conservation Domains)
    """
    PHASE_SPACE = "PhaseSpace"       # 相空间测度域：立体角测度、极坐标几何开方、展宽
    CAUSALITY = "Causality"           # 因果与绝对时钟域：全局实验室时钟 vs 局域寿命、TOF、符合时序
    VARIANCE = "Variance"             # 统计权重流域：减方差重要抽样、分裂与轮盘赌权重无偏传递
    LIFECYCLE = "Lifecycle"           # 步进生命周期与几何边界域：后步点判定、死层隔离、容器拓扑隔离
    NUCLEAR = "Nuclear"               # 核素与微观反应域：截面连续性、衰变分支比、光核/热散射模型
    RING0_INTENT = "Ring0Intent"      # 上游需求与意图对齐域：物理量槽位、几何实体与实验基准对齐


class PSTEdgeType(str, Enum):
    """
    物理语义依赖边类型 (Physics Semantic Edge Types)
    """
    DATA_DEPENDENCY = "DataDependency"                       # 常规数据依赖：标量、向量或对象值传递
    WEIGHT_FLOW = "WeightFlow"                               # 统计权重传递流：TrackWeight 流向 Tally Score
    CAUSAL_TIME = "CausalTime"                               # 时钟因果流：源项时钟向到达时钟的基准传递
    NORMALIZATION_DENOMINATOR = "NormalizationDenominator"   # 归一化分母传递：介质质量或源粒子数流向 Convert
    BOUNDARY_GATE = "BoundaryGate"                           # 几何界面状态门控：必须受 fGeomBoundary 保护
    VOLUME_CONTAINMENT = "VolumeContainment"                 # 几何拓扑从属关系：死层与灵敏体积物理隔离
    LIFECYCLE_HIERARCHY = "LifecycleHierarchy"               # 生命周期层级纽带：Run -> Event -> Track -> Step -> Hit
    LIFECYCLE_RESET = "LifecycleReset"                       # 容器重置纽带：BeginOfEvent / BeginOfRun 到累加变量的生命周期重置
    CALL_DISPATCH = "CallDispatch"                           # 跨模块调用分发纽带：如 SteppingAction -> RunAction::AddFlux


class PSTNode(BaseModel):
    """
    物理语义树节点 (PST Node)
    代表一个具有独立物理意义的原子计算步骤。
    """
    model_config = ConfigDict(extra="ignore")

    node_id: str = Field(description="节点唯一标识，如 n_sample_dir_01")
    action: PhysicalAction = Field(description="五大物理动作类型 (Define/Sample/Fetch/Accumulate/Convert)")
    domain: ConservationDomain = Field(description="所属正交守恒总线域")
    quantity: str = Field(description="涉及的核心物理量，如 Direction, GlobalTime, Weight, Dose, EnergyDeposit")
    metric: Optional[str] = Field(default=None, description="物理测度或形式，如 SolidAngle_4Pi, MicroscopicMass")
    expected_invariant: Optional[str] = Field(default=None, description="预期的守恒不变式契约，如 dOmega = sin(theta) dtheta dphi")
    code_evidence: str = Field(description="对应的 C++ 代码原文字段证据")
    line_range: Tuple[int, int] = Field(description="在源码中的起始与结束行号 (1-based)")
    ast_node_type: Optional[str] = Field(default=None, description="底层 AST 节点类型，如 CallExpr, AssignExpr, DeclStmt")
    scope_context: Dict[str, Any] = Field(default_factory=dict, description="作用域上下文：类名、方法名、步进阶段、边界分支")
    attributes: Dict[str, Any] = Field(default_factory=dict, description="扩展物理属性，如单位、核素符号、固定向量值")


class PSTEdge(BaseModel):
    """
    物理语义树有向依赖边 (PST Edge)
    表示物理量、统计权重、时间戳基准或归一化分母的因果流转关系。
    """
    model_config = ConfigDict(extra="ignore")

    edge_id: str = Field(description="边唯一标识，如 e_weight_01")
    source_id: str = Field(description="源节点 node_id")
    target_id: str = Field(description="目标节点 node_id")
    edge_type: PSTEdgeType = Field(description="依赖边类型")
    variable_name: Optional[str] = Field(default=None, description="承载该流转的变量名或表达式，如 weight, mass, globalTime")
    physical_units: Optional[str] = Field(default=None, description="物理单位约束，如 MeV, Gy, ns, g/cm3")
    attributes: Dict[str, Any] = Field(default_factory=dict, description="附加属性")


class PSTGraph(BaseModel):
    """
    物理语义树 / 语义有向无环图 (Physics Semantic Tree / Graph)
    整个输运仿真程序在物理守恒层面的完整中间表示。
    """
    model_config = ConfigDict(extra="ignore")

    graph_id: str = Field(description="PST 唯一标识")
    source_file: str = Field(description="源码文件名或相对路径")
    target_observable: Optional[str] = Field(default=None, description="目标物理量，如 Fluence, AbsorbedDose, Efficiency")
    nodes: List[PSTNode] = Field(default_factory=list, description="物理算子节点列表")
    edges: List[PSTEdge] = Field(default_factory=list, description="物理依赖边列表")
    metadata: Dict[str, Any] = Field(default_factory=dict, description="图级元数据，如提取引擎版本、编译器配置")

    # ─────────────────────────────────────────────────────────────
    # 辅助索引与图遍历方法
    # ─────────────────────────────────────────────────────────────

    def get_node(self, node_id: str) -> Optional[PSTNode]:
        for n in self.nodes:
            if n.node_id == node_id:
                return n
        return None

    def add_node(self, node: PSTNode) -> None:
        if self.get_node(node.node_id) is not None:
            raise ValueError(f"Duplicate node_id: {node.node_id}")
        self.nodes.append(node)

    def add_edge(self, edge: PSTEdge) -> None:
        if not self.get_node(edge.source_id):
            raise ValueError(f"Edge source_id not found: {edge.source_id}")
        if not self.get_node(edge.target_id):
            raise ValueError(f"Edge target_id not found: {edge.target_id}")
        self.edges.append(edge)

    def get_incoming_edges(self, target_id: str) -> List[PSTEdge]:
        return [e for e in self.edges if e.target_id == target_id]

    def get_outgoing_edges(self, source_id: str) -> List[PSTEdge]:
        return [e for e in self.edges if e.source_id == source_id]

    def find_nodes_by_action(self, action: PhysicalAction) -> List[PSTNode]:
        return [n for n in self.nodes if n.action == action]

    def find_nodes_by_domain(self, domain: ConservationDomain) -> List[PSTNode]:
        return [n for n in self.nodes if n.domain == domain]

    def has_path(self, source_id: str, target_id: str, edge_type: Optional[PSTEdgeType] = None) -> bool:
        """检查是否存在从 source 到 target 的有向路径（可限制特定边类型）"""
        visited: Set[str] = set()
        queue = [source_id]
        while queue:
            curr = queue.pop(0)
            if curr == target_id:
                return True
            if curr in visited:
                continue
            visited.add(curr)
            for e in self.get_outgoing_edges(curr):
                if edge_type is None or e.edge_type == edge_type:
                    queue.append(e.target_id)
        return False

    def validate_ir(self) -> List[str]:
        """校验 PST IR 本身的结构自洽性（无悬挂边、节点唯一性等）"""
        errors = []
        node_ids = {n.node_id for n in self.nodes}
        if len(node_ids) != len(self.nodes):
            errors.append("PST 存在重复的 node_id")
        for e in self.edges:
            if e.source_id not in node_ids:
                errors.append(f"边 {e.edge_id} 的源节点 {e.source_id} 不存在")
            if e.target_id not in node_ids:
                errors.append(f"边 {e.edge_id} 的目标节点 {e.target_id} 不存在")
        return errors

    def to_dot(self) -> str:
        """导出为 Graphviz DOT 格式，便于可视化与论文插图绘制"""
        lines = [f'digraph "{self.graph_id}" {{', '  rankdir=LR;', '  node [shape=box, style=rounded, fontname="Helvetica"];']
        
        # 节点着色规范（按 Domain 映射）
        colors = {
            ConservationDomain.PHASE_SPACE: "#E1F5FE",   # 浅蓝
            ConservationDomain.CAUSALITY: "#FFF3E0",     # 浅橙
            ConservationDomain.VARIANCE: "#E8F5E9",      # 浅绿
            ConservationDomain.LIFECYCLE: "#F3E5F5",     # 浅紫
            ConservationDomain.NUCLEAR: "#FFEBEE",       # 浅红
            ConservationDomain.RING0_INTENT: "#ECEFF1",  # 浅灰
        }

        for n in self.nodes:
            c = colors.get(n.domain, "#FFFFFF")
            lbl = f"[{n.action.value}] {n.quantity}\\nLine {n.line_range[0]}-{n.line_range[1]}\\n{n.code_evidence[:30]}"
            lines.append(f'  "{n.node_id}" [label="{lbl}", fillcolor="{c}", style="filled,rounded"];')

        # 边样式规范（按 EdgeType 映射）
        edge_styles = {
            PSTEdgeType.WEIGHT_FLOW: '[color="#2E7D32", penwidth=2.0, label="WeightFlow"]',
            PSTEdgeType.NORMALIZATION_DENOMINATOR: '[color="#C62828", penwidth=2.0, label="NormDiv"]',
            PSTEdgeType.CAUSAL_TIME: '[color="#EF6C00", penwidth=1.5, label="CausalTime"]',
            PSTEdgeType.BOUNDARY_GATE: '[color="#6A1B9A", stroke="dashed", label="BoundaryGate"]',
            PSTEdgeType.VOLUME_CONTAINMENT: '[color="#455A64", style="dotted", label="Containment"]',
            PSTEdgeType.DATA_DEPENDENCY: '[color="#9E9E9E", label="DataDep"]',
            PSTEdgeType.LIFECYCLE_HIERARCHY: '[color="#1565C0", penwidth=1.8, style="dashed", label="Lifecycle"]',
            PSTEdgeType.LIFECYCLE_RESET: '[color="#00838F", penwidth=1.8, label="Reset"]',
            PSTEdgeType.CALL_DISPATCH: '[color="#5E35B1", penwidth=1.5, label="CallDispatch"]',
        }

        for e in self.edges:
            style = edge_styles.get(e.edge_type, '[color="#000000"]')
            lines.append(f'  "{e.source_id}" -> "{e.target_id}" {style};')

        lines.append("}")
        return "\n".join(lines)
