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
        """
        连接期 (Linking Phase)：基于三大语义纽带建立跨编译单元的拓扑依赖边：
        1. 数据流跨模块调用与共享状态纽带 (Call Graph & Shared State)
        2. 几何实体与死层作用域纽带 (Volume Scope & Containment)
        3. Geant4 生命周期层级与容器重置纽带 (Lifecycle Timeline: Run -> Event -> Step -> Hit)
        """
        fetch_nodes = global_graph.find_nodes_by_action(PhysicalAction.FETCH)
        accumulate_nodes = global_graph.find_nodes_by_action(PhysicalAction.ACCUMULATE)
        convert_nodes = global_graph.find_nodes_by_action(PhysicalAction.CONVERT)
        sample_nodes = global_graph.find_nodes_by_action(PhysicalAction.SAMPLE)
        define_nodes = global_graph.find_nodes_by_action(PhysicalAction.DEFINE)

        edge_counter = 1

        # ─────────────────────────────────────────────────────────────
        # 纽带 1: 数据流跨模块调用与共享状态纽带 (Call Graph & Shared State)
        # ─────────────────────────────────────────────────────────────
        # 1.1 跨模块权重流 (WeightFlow) 连接：
        # 解决痛点：SensitiveDetector.cc 显式乘入权重，或 SteppingAction 调用 fRunAction->AddFlux(1.0, w)
        has_any_weight_fetch = any(fn.quantity == "TrackWeight" for fn in fetch_nodes)
        weight_fetchers = [fn for fn in fetch_nodes if fn.quantity == "TrackWeight"]

        for acc in accumulate_nodes:
            is_cross_call = acc.attributes.get("cross_module_call", False)
            method_name = acc.attributes.get("method", "").lower()
            call_has_weight = acc.attributes.get("has_weight", False)

            # 跨模块调用分发 (CallDispatch)
            if is_cross_call:
                # 寻找同工程中可能接受该调用的目标累加器或 RunAction/EventAction 中的 Tally
                target_candidates = [
                    a for a in accumulate_nodes
                    if a.node_id != acc.node_id and (
                        "runaction" in a.node_id.lower() or "eventaction" in a.node_id.lower() or
                        "flux" in a.quantity.lower() or "pulse" in a.quantity.lower()
                    )
                ]
                for tgt in target_candidates:
                    global_graph.add_edge(PSTEdge(
                        edge_id=f"e_cross_dispatch_{edge_counter}",
                        source_id=acc.node_id,
                        target_id=tgt.node_id,
                        edge_type=PSTEdgeType.CALL_DISPATCH,
                        variable_name=acc.attributes.get("method"),
                    ))
                    edge_counter += 1

                    # 若跨模块实参携带权重，将 WeightFlow 注入目标累加器
                    if call_has_weight:
                        global_graph.add_edge(PSTEdge(
                            edge_id=f"e_cross_weight_flow_{edge_counter}",
                            source_id=acc.node_id,
                            target_id=tgt.node_id,
                            edge_type=PSTEdgeType.WEIGHT_FLOW,
                            variable_name="dispatched_weight",
                        ))
                        edge_counter += 1

            # 跨文件感知：若工程中有任何文件抓取了 TrackWeight，且当前累加器属于通量或缺少本地入度
            if acc.quantity in ("FluenceScore", "PulseHeight", "SensitiveRegionHit"):
                has_weight_edge = any(
                    e.target_id == acc.node_id and e.edge_type == PSTEdgeType.WEIGHT_FLOW
                    for e in global_graph.edges
                )
                if not has_weight_edge and (has_any_weight_fetch or call_has_weight):
                    src_fn = weight_fetchers[0] if weight_fetchers else (acc if call_has_weight else None)
                    if src_fn:
                        global_graph.add_edge(PSTEdge(
                            edge_id=f"e_cross_weight_{edge_counter}",
                            source_id=src_fn.node_id,
                            target_id=acc.node_id,
                            edge_type=PSTEdgeType.WEIGHT_FLOW,
                            variable_name="cross_module_weight",
                        ))
                        edge_counter += 1

        # 1.2 跨模块能量沉积依赖 (Edep -> PulseHeight)
        for fn in fetch_nodes:
            if fn.quantity == "EnergyDeposit":
                for acc in accumulate_nodes:
                    if acc.quantity in ("PulseHeight", "SensitiveRegionHit") and fn.node_id != acc.node_id:
                        if not any(e.source_id == fn.node_id and e.target_id == acc.node_id for e in global_graph.edges):
                            global_graph.add_edge(PSTEdge(
                                edge_id=f"e_cross_edep_{edge_counter}",
                                source_id=fn.node_id,
                                target_id=acc.node_id,
                                edge_type=PSTEdgeType.DATA_DEPENDENCY,
                                physical_units="MeV",
                            ))
                            edge_counter += 1

        # 1.3 跨模块时钟与边界状态传递
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
            elif fn.quantity == "BoundaryCrossing":
                for acc in accumulate_nodes:
                    if not any(e.source_id == fn.node_id and e.target_id == acc.node_id for e in global_graph.edges):
                        global_graph.add_edge(PSTEdge(
                            edge_id=f"e_cross_boundary_{edge_counter}",
                            source_id=fn.node_id,
                            target_id=acc.node_id,
                            edge_type=PSTEdgeType.BOUNDARY_GATE,
                        ))
                        edge_counter += 1

        # ─────────────────────────────────────────────────────────────
        # 纽带 2: 几何实体与死层作用域纽带 (Volume Scope & Containment)
        # ─────────────────────────────────────────────────────────────
        volume_nodes = [dn for dn in define_nodes if dn.quantity == "LogicalVolume"]
        for vn in volume_nodes:
            vol_name = vn.attributes.get("volume_name", "")
            is_dead = vn.attributes.get("is_dead_layer", False)
            if not vol_name:
                continue

            # 遍历所有计分节点与边界判定节点，建立几何从属关系边 (VolumeContainment)
            for acc in accumulate_nodes:
                acc_code = acc.code_evidence
                # 检查计分逻辑中是否引用了该体积名称
                if vol_name.lower() in acc_code.lower() or vn.node_id != acc.node_id:
                    global_graph.add_edge(PSTEdge(
                        edge_id=f"e_cross_vol_{edge_counter}",
                        source_id=vn.node_id,
                        target_id=acc.node_id,
                        edge_type=PSTEdgeType.VOLUME_CONTAINMENT,
                        variable_name=vol_name,
                        attributes={"is_dead_layer": is_dead},
                    ))
                    edge_counter += 1

                    # 跨文件死层隔离校验：若定义为死层，但在计分中未隔离或混入灵敏区探测信号
                    if is_dead:
                        if ("+" in acc_code and "dead" in acc_code.lower()) or "dead" in acc.attributes.get("lhs", "").lower():
                            acc.metric = "DeadLayerContaminatedHit"

        # ─────────────────────────────────────────────────────────────
        # 纽带 3: Geant4 生命周期层级与容器重置纽带 (Lifecycle Timeline)
        # ─────────────────────────────────────────────────────────────
        reset_nodes = [dn for dn in define_nodes if dn.quantity == "LifecycleReset"]
        # 3.1 建立全局生命周期层级流 (Run -> Event -> Track -> Step -> Hit)
        tier_hierarchy = ["Run", "Event", "Track", "Step", "Hit"]
        nodes_by_tier: Dict[str, List[PSTNode]] = {t: [] for t in tier_hierarchy}

        for n in global_graph.nodes:
            t = n.scope_context.get("lifecycle_tier")
            if t in nodes_by_tier:
                nodes_by_tier[t].append(n)

        for i in range(len(tier_hierarchy) - 1):
            upper_tier = tier_hierarchy[i]
            lower_tier = tier_hierarchy[i + 1]
            if nodes_by_tier[upper_tier] and nodes_by_tier[lower_tier]:
                # 建立层级拓扑纽带边
                u_node = nodes_by_tier[upper_tier][0]
                l_node = nodes_by_tier[lower_tier][0]
                global_graph.add_edge(PSTEdge(
                    edge_id=f"e_lifecycle_tier_{edge_counter}",
                    source_id=u_node.node_id,
                    target_id=l_node.node_id,
                    edge_type=PSTEdgeType.LIFECYCLE_HIERARCHY,
                    variable_name=f"{upper_tier}_to_{lower_tier}",
                ))
                edge_counter += 1

        # 3.2 跨事件/跨 Run 容器重置纽带 (LifecycleReset)
        for rn in reset_nodes:
            reset_var = rn.attributes.get("reset_var", "").lower()
            tier = rn.attributes.get("tier", "Event")
            for acc in accumulate_nodes:
                acc_lhs = acc.attributes.get("lhs", "").lower()
                acc_code = acc.code_evidence.lower()
                # 变量名匹配
                if reset_var and (reset_var in acc_lhs or acc_lhs in reset_var or reset_var in acc_code):
                    global_graph.add_edge(PSTEdge(
                        edge_id=f"e_lifecycle_reset_{edge_counter}",
                        source_id=rn.node_id,
                        target_id=acc.node_id,
                        edge_type=PSTEdgeType.LIFECYCLE_RESET,
                        variable_name=reset_var,
                        attributes={"tier": tier},
                    ))
                    edge_counter += 1
