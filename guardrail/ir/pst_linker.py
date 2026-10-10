#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/pst_linker.py —— 物理语义树连接器 (PST Linker)

负责将多文件 C++ 工程（DetectorConstruction.cc, PrimaryGenerator.cc,
SteppingAction.cc, EventAction.cc, RunAction.cc 等）的离散物理语义子图，
基于三大跨模块纽带缝合成单一全景物理语义图 (Global PSTGraph)：
1. 数据流跨模块调用与共享状态纽带 (Call Graph & Shared State)
2. 几何实体与死层作用域纽带 (Volume Scope & Containment)
3. Geant4 生命周期层级纽带 (Lifecycle Timeline: Run -> Event -> Step)
"""

from __future__ import annotations

import os
import re
from pathlib import Path
from typing import Dict, List, Optional, Any, Set, Tuple

from guardrail.ir.pst_schema import (
    PhysicalAction,
    ConservationDomain,
    PSTEdgeType,
    PSTNode,
    PSTEdge,
    PSTGraph,
)
from guardrail.ir.pst_builder import PSTBuilder
from guardrail.ir.cmake_parser import CMakeProjectInfo, CMakeParser


class PSTLinker:
    """物理语义树连接器 (PST Linker)"""

    def __init__(self):
        self.builder = PSTBuilder()

    def link_project(self, project_info: CMakeProjectInfo) -> PSTGraph:
        """主入口：将 CMake 工程解析得到的全部源码文件编译并连接为主 PSTGraph"""
        global_graph_id = f"pst_proj_{project_info.project_name or 'project'}"
        global_graph = PSTGraph(
            graph_id=global_graph_id,
            source_file=project_info.project_dir,
        )

        subgraphs: Dict[str, PSTGraph] = {}
        all_text_by_file: Dict[str, str] = {}
        full_project_text = []

        # 1. 编译期：对每个源码文件提取局部 PST 子图
        for src_path in project_info.all_source_files:
            p = Path(src_path)
            if not p.exists():
                continue
            text = p.read_text(encoding="utf-8", errors="replace")
            all_text_by_file[src_path] = text
            full_project_text.append(text)

            # 构建局部子图
            local_g = self.builder.build(text, filename=src_path)
            subgraphs[src_path] = local_g

        combined_code_corpus = "\n".join(full_project_text)

        # 2. 上下文全局增强：检查整个工程是否为未准直的点放射源环境
        is_collimated = bool(re.search(r"(?:准直|collimat|pencil|平行束|垂直入射|正入射)", combined_code_corpus, re.IGNORECASE))
        is_radioactive_point_source = bool(re.search(
            r"(?:Cs-?137|Co-?60|Na-?22|Am-?241|Eu-?152|Ba-?133|\bpoint\s*source\b|\b点源\b|\bisotrop|\b各向同性\b)",
            combined_code_corpus,
            re.IGNORECASE,
        ))
        is_global_point_source = is_radioactive_point_source and not is_collimated

        # 3. 节点合并与命名空间化
        node_map: Dict[str, PSTNode] = {}
        for file_idx, (src_path, sub_g) in enumerate(subgraphs.items(), start=1):
            try:
                rel_path = Path(src_path).relative_to(project_info.project_dir)
                rel_name = str(rel_path).replace("/", "_").replace(".", "_")
            except Exception:
                rel_name = f"f{file_idx}_{Path(src_path).stem}"
            for node in sub_g.nodes:
                namespaced_id = f"{rel_name}_{node.node_id}"
                # 如果是动量抽样节点，更新其全工程点源感知上下文
                if node.quantity == "Direction":
                    node.attributes["is_point_source"] = is_global_point_source

                # 记录所属源文件真实路径
                node.attributes["source_file"] = src_path

                new_node = node.model_copy(update={"node_id": namespaced_id})
                node_map[namespaced_id] = new_node
                global_graph.add_node(new_node)

            # 保留原文件内部的边
            for edge in sub_g.edges:
                namespaced_src = f"{rel_name}_{edge.source_id}"
                namespaced_tgt = f"{rel_name}_{edge.target_id}"
                global_graph.add_edge(PSTEdge(
                    edge_id=f"{rel_name}_{edge.edge_id}",
                    source_id=namespaced_src,
                    target_id=namespaced_tgt,
                    edge_type=edge.edge_type,
                    variable_name=edge.variable_name,
                    physical_units=edge.physical_units,
                    attributes=edge.attributes,
                ))

        # 4. 连接期 (Linking Phase)：建立跨文件语义有向边
        self._link_cross_module_edges(global_graph, subgraphs, all_text_by_file, project_info)

        return global_graph

    def _link_cross_module_edges(
        self,
        global_graph: PSTGraph,
        subgraphs: Dict[str, PSTGraph],
        all_text_by_file: Dict[str, str],
        project_info: CMakeProjectInfo,
    ) -> None:
        """建立跨编译单元的拓扑依赖边"""
        fetch_nodes = global_graph.find_nodes_by_action(PhysicalAction.FETCH)
        accumulate_nodes = global_graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        convert_nodes = global_graph.find_nodes_by_action(PhysicalAction.CONVERT)
        sample_nodes = global_graph.find_nodes_by_action(PhysicalAction.SAMPLE)

        edge_counter = 1

        # 纽带 1: 跨模块权重流 (WeightFlow) 连接
        # 如果 SteppingAction 中调用了 GetWeight()，而 EventAction / RunAction 中累加了 Flux / Score
        for acc in accumulate_nodes:
            if acc.quantity == "FluenceScore":
                has_local_weight = any(e.target_id == acc.node_id and e.edge_type == PSTEdgeType.WEIGHT_FLOW for e in global_graph.edges)
                if not has_local_weight:
                    for fn in fetch_nodes:
                        if fn.quantity == "TrackWeight":
                            # 跨文件拉一条权重流边
                            global_graph.add_edge(PSTEdge(
                                edge_id=f"e_cross_weight_{edge_counter}",
                                source_id=fn.node_id,
                                target_id=acc.node_id,
                                edge_type=PSTEdgeType.WEIGHT_FLOW,
                                variable_name="cross_module_weight",
                            ))
                            edge_counter += 1

        # 纽带 2: 跨模块能量沉积与计分累加 (DataDependency: Edep -> PulseHeight)
        # 例如 SteppingAction 抓取 GetTotalEnergyDeposit，传给 EventAction 中的总能量
        for fn in fetch_nodes:
            if fn.quantity == "EnergyDeposit":
                for acc in accumulate_nodes:
                    if acc.quantity in ("PulseHeight", "SensitiveRegionHit"):
                        if not any(e.source_id == fn.node_id and e.target_id == acc.node_id for e in global_graph.edges):
                            global_graph.add_edge(PSTEdge(
                                edge_id=f"e_cross_edep_{edge_counter}",
                                source_id=fn.node_id,
                                target_id=acc.node_id,
                                edge_type=PSTEdgeType.DATA_DEPENDENCY,
                                physical_units="MeV",
                            ))
                            edge_counter += 1

        # 纽带 3: 跨模块绝对实验时钟流 (CausalTime)
        for fn in fetch_nodes:
            if fn.quantity in ("GlobalTime", "LocalTrackTime"):
                for acc in accumulate_nodes:
                    if acc.quantity == "TimeOfFlightTally":
                        if not any(e.source_id == fn.node_id and e.target_id == acc.node_id for e in global_graph.edges):
                            global_graph.add_edge(PSTEdge(
                                edge_id=f"e_cross_time_{edge_counter}",
                                source_id=fn.node_id,
                                target_id=acc.node_id,
                                edge_type=PSTEdgeType.CAUSAL_TIME,
                            ))
                            edge_counter += 1

        # 纽带 4: 跨模块边界门控 (BoundaryGate)
        for fn in fetch_nodes:
            if fn.quantity == "BoundaryCrossing":
                for acc in accumulate_nodes:
                    if not any(e.source_id == fn.node_id and e.target_id == acc.node_id for e in global_graph.edges):
                        global_graph.add_edge(PSTEdge(
                            edge_id=f"e_cross_boundary_{edge_counter}",
                            source_id=fn.node_id,
                            target_id=acc.node_id,
                            edge_type=PSTEdgeType.BOUNDARY_GATE,
                        ))
                        edge_counter += 1
