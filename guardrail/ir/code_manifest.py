#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/code_manifest.py —— 蒙卡代码实参清单提取器 (Code Parameter Manifest Extractor)

基于 tree-sitter AST 静态后根遍历 (Post-order AST Traversal) 与跨模块符号链接，
直接从 C++ 源码中零开销提取 100% 确定性的真实仿真输入参数：
1. 辐射源项 (Source): 粒子种类、初级能量/谱、空间位置、发射角分布测度、事件数
2. 几何与材料 (Geometry): 实体形状、尺寸、材料介质、摆位坐标、灵敏体与死层
3. 物理过程 (Physics): 物理列表、高精度强子模型、电磁模型、截断阈
4. 计分与响应 (Tally): 观测量、能量计数窗、界面门控、权重传递
5. 运行控制 (Run): RunManager 类型、线程数、减方差方法

提供结构化 JSON 导出、美化 Markdown/ANSI 终端卡片输出，并支持双向意图漂移检测 (Intent Drift Check)。
"""

from __future__ import annotations

import os
import re
import time
from pathlib import Path
from typing import Dict, List, Optional, Any, Tuple
from pydantic import BaseModel, Field

import tree_sitter_cpp as tscpp
from tree_sitter import Language, Parser, Node

from guardrail.ir.cmake_parser import CMakeParser, CMakeProjectInfo
from guardrail.ir.intent_extractor import PhysicsIntent, SourceSymmetry, ObservableType


class SourceManifest(BaseModel):
    """辐射源项参数清单"""
    particles: List[str] = Field(default_factory=list, description="粒子种类 (如 gamma, neutron)")
    energies: List[str] = Field(default_factory=list, description="初级能量 / 能谱 (如 661.7 * keV, 14 * MeV)")
    position: Optional[str] = Field(default=None, description="发射空间位置 (如 G4ThreeVector(0, 0, 0))")
    direction: Optional[str] = Field(default=None, description="动量发射方向 / 向量")
    angular_distribution_type: str = Field(default="未指定", description="角分布测度类型 (4pi 各向同性 / 单向平行束 / 自定义)")
    beam_on_events: Optional[str] = Field(default=None, description="模拟历史事件数 (如 10000)")
    source_type: str = Field(default="G4ParticleGun", description="源项生成器类型 (G4ParticleGun / GPS / 自定义)")
    evidence: Dict[str, str] = Field(default_factory=dict, description="对应源码位置 (参数名 -> 文件:行号)")


class GeometrySolid(BaseModel):
    """几何实体与介质参数"""
    solid_name: str = Field(description="几何实体名称 (如 World, HPGeCrystal)")
    shape_type: str = Field(description="几何形状类型 (如 G4Box, G4Tubs, G4Sphere)")
    parameters: Dict[str, str] = Field(default_factory=dict, description="尺寸参数 (如 x, y, z 半宽, rmin, rmax)")
    material: Optional[str] = Field(default=None, description="填充介质材料 (如 G4_AIR, G4_Ge, BPE)")
    logical_volume: Optional[str] = Field(default=None, description="逻辑体名称")
    placement_position: Optional[str] = Field(default=None, description="空间相对放置坐标")
    placement_mother: Optional[str] = Field(default=None, description="母体积名称")
    role: str = Field(default="结构介质体 (Passive)", description="几何角色 (World / SensitiveDetector / DeadLayer / Shield)")
    code_location: str = Field(description="源码位置 (文件:行号)")


class PhysicsManifest(BaseModel):
    """物理过程与物理列表参数"""
    physics_list_name: Optional[str] = Field(default=None, description="物理列表名称 (如 QGSP_BERT, QGSP_BIC_HP)")
    modular_physics: List[str] = Field(default_factory=list, description="注册的模块化物理过程")
    production_cuts: Optional[str] = Field(default=None, description="产生截止阈 (如 1.0*mm)")
    step_limits: Optional[str] = Field(default=None, description="用户步长限制 (G4UserLimits)")
    special_features: List[str] = Field(default_factory=list, description="高精度中子/核衰变等特殊物理特性")
    evidence: Dict[str, str] = Field(default_factory=dict, description="对应源码位置")


class TallyManifest(BaseModel):
    """计分响应与统计量参数"""
    tally_id: str = Field(description="计分项标识")
    observable_type: str = Field(description="观测量种类 (全能峰探测效率 / 界面粒子通量 / 能量沉积 / 吸收剂量 / TOF)")
    scoring_variable: Optional[str] = Field(default=None, description="计分变量名 (如 fPeakCounts, gFlux, fTotalEdep)")
    scoring_volume_or_surface: Optional[str] = Field(default=None, description="计分作用域 (体积或界面)")
    energy_window: Optional[str] = Field(default=None, description="能谱窗口 / 道数 (如 [0.660, 0.663] MeV)")
    boundary_filter: Optional[str] = Field(default=None, description="界面状态过滤器 (如 fGeomBoundary, PostStepPoint)")
    weight_preservation: bool = Field(default=False, description="是否保留统计权重 (GetWeight)")
    time_window: Optional[str] = Field(default=None, description="时间窗 / 符合时间门")
    code_location: str = Field(description="源码位置 (文件:行号)")


class RunControlManifest(BaseModel):
    """运行环境与减方差参数"""
    run_manager_type: str = Field(default="G4RunManager (单线程)", description="RunManager 类型")
    threads: Optional[str] = Field(default=None, description="工作线程数")
    random_engine_seed: Optional[str] = Field(default=None, description="随机数引擎 / 种子配置")
    variance_reduction: str = Field(default="无偏直抽样 (Analog)", description="减方差技巧配置")
    evidence: Dict[str, str] = Field(default_factory=dict, description="对应源码位置")


class CodeParameterManifest(BaseModel):
    """代码实参完整结构化清单 (Ground-Truth Code Parameter Manifest)"""
    target: str = Field(description="目标分析路径 (文件或工程目录)")
    project_name: Optional[str] = Field(default=None, description="工程名称")
    is_multi_file_project: bool = Field(default=False, description="是否为多文件 CMake 工程")
    source_files_analyzed: List[str] = Field(default_factory=list, description="分析的源文件列表")
    extraction_latency_ms: float = Field(default=0.0, description="AST 静态提取耗时 (毫秒)")
    source: SourceManifest = Field(default_factory=SourceManifest)
    geometry: List[GeometrySolid] = Field(default_factory=list)
    materials_catalog: List[str] = Field(default_factory=list, description="检测到的材料库清单")
    physics: PhysicsManifest = Field(default_factory=PhysicsManifest)
    tallies: List[TallyManifest] = Field(default_factory=list)
    run_control: RunControlManifest = Field(default_factory=RunControlManifest)
    drift_warnings: List[str] = Field(default_factory=list, description="意图与代码参数漂移警告")

    def to_dict(self) -> Dict[str, Any]:
        """序列化为标准 Python 字典"""
        return self.model_dump()

    def to_json(self, indent: int = 2) -> str:
        """导出为格式化 JSON 字符串"""
        return self.model_dump_json(indent=indent)

    def to_markdown(self) -> str:
        """生成 Markdown 格式的完整参数清单文档"""
        lines = [
            f"# 蒙特卡罗仿真输入参数显式清单 (Code Parameter Manifest)",
            f"",
            f"- **分析目标**: `{self.target}` ({'多文件 CMake 工程' if self.is_multi_file_project else '单文件仿真源码'})",
            f"- **分析耗时**: `{self.extraction_latency_ms:.2f} ms` (Tree-sitter AST 纯静态零开销确定性提取)",
            f"- **分析源文件数**: `{len(self.source_files_analyzed)}` 份 C++ 编译单元",
            f"",
        ]

        if self.drift_warnings:
            lines.append("## ⚠️ 物理意图与代码实参漂移警报 (Parameter Drift)")
            for w in self.drift_warnings:
                lines.append(f"> 🚨 **{w}**")
            lines.append("")

        lines.append("## 1. 辐射源项参数 (Radiation Source)")
        lines.append(f"| 参数项 | 代码实参提取值 | 角分布测度 | 源码位置 |")
        lines.append(f"| :--- | :--- | :--- | :--- |")
        parts = ", ".join(self.source.particles) if self.source.particles else "未显式指定"
        energies = ", ".join(self.source.energies) if self.source.energies else "未显式指定"
        pos = self.source.position or "未显式指定"
        dir_val = self.source.direction or "未显式指定"
        ev_p = self.source.evidence.get("particle", "-")
        ev_e = self.source.evidence.get("energy", "-")
        ev_pos = self.source.evidence.get("position", "-")
        ev_dir = self.source.evidence.get("direction", "-")

        lines.append(f"| **粒子种类** | `{parts}` | - | `{ev_p}` |")
        lines.append(f"| **初级能量** | `{energies}` | - | `{ev_e}` |")
        lines.append(f"| **发射位置** | `{pos}` | - | `{ev_pos}` |")
        lines.append(f"| **发射方向/分布** | `{dir_val}` | **{self.source.angular_distribution_type}** | `{ev_dir}` |")
        lines.append(f"| **模拟事件数** | `{self.source.beam_on_events or '未指定'}` | - | `{self.source.evidence.get('beam_on', '-')}` |")
        lines.append("")

        lines.append("## 2. 几何拓扑与介质材料 (Geometry & Materials)")
        lines.append(f"| 几何体名称 | 形状类别 | 尺寸参数 | 填充材料 | 相对摆位坐标 | 物理角色 | 源码位置 |")
        lines.append(f"| :--- | :--- | :--- | :--- | :--- | :--- | :--- |")
        if self.geometry:
            for g in self.geometry:
                dim_str = ", ".join(f"{k}={v}" for k, v in g.parameters.items()) if g.parameters else "-"
                mat_str = g.material or "未指定"
                pos_str = g.placement_position or "原点 (0,0,0)"
                lines.append(f"| **{g.solid_name}** | `{g.shape_type}` | `{dim_str}` | `{mat_str}` | `{pos_str}` | {g.role} | `{g.code_location}` |")
        else:
            lines.append("| (未检测到几何体定义) | - | - | - | - | - | - |")
        lines.append("")

        lines.append("## 3. 物理列表与微观过程 (Physics List)")
        pl_name = self.physics.physics_list_name or "未显式定义 (或自定义 ModularPhysicsList)"
        lines.append(f"- **主物理列表**: `{pl_name}` (`{self.physics.evidence.get('physics_list', '-')}`)")
        if self.physics.modular_physics:
            lines.append(f"- **注册物理模块**: {', '.join(f'`{m}`' for m in self.physics.modular_physics)}")
        if self.physics.production_cuts:
            lines.append(f"- **产生截止阈**: `{self.physics.production_cuts}`")
        if self.physics.special_features:
            lines.append(f"- **特殊物理特性**: {', '.join(self.physics.special_features)}")
        lines.append("")

        lines.append("## 4. 计分响应与统计量 (Tally & Scoring)")
        lines.append(f"| 计分项 | 观测量种类 | 作用域 / 几何界面 | 能谱计数窗口 | 界面状态过滤 | 权重保护 | 源码位置 |")
        lines.append(f"| :--- | :--- | :--- | :--- | :--- | :---: | :--- |")
        if self.tallies:
            for t in self.tallies:
                vol_str = t.scoring_volume_or_surface or "-"
                win_str = t.energy_window or "全谱积分"
                flt_str = t.boundary_filter or "-"
                wt_str = "✅ 保留" if t.weight_preservation else "❌ 丢失(平权)"
                lines.append(f"| `{t.tally_id}` | **{t.observable_type}** | `{vol_str}` | `{win_str}` | `{flt_str}` | {wt_str} | `{t.code_location}` |")
        else:
            lines.append("| (未检测到显式计分器) | - | - | - | - | - | - |")
        lines.append("")

        lines.append("## 5. 运行控制与减方差 (Run Control)")
        lines.append(f"- **RunManager 类型**: `{self.run_control.run_manager_type}`")
        if self.run_control.threads:
            lines.append(f"- **工作线程数**: `{self.run_control.threads}`")
        lines.append(f"- **减方差策略**: `{self.run_control.variance_reduction}`")
        lines.append("")

        return "\n".join(lines)

    def render_cli(self) -> str:
        """生成带 ANSI 高亮色彩的终端卡片"""
        BOLD = "\033[1m"
        RESET = "\033[0m"
        CYAN = "\033[36m"
        GREEN = "\033[32m"
        YELLOW = "\033[33m"
        RED = "\033[31m"
        DIM = "\033[2m"
        MAGENTA = "\033[35m"

        w = 80
        lines = [
            f"{BOLD}{CYAN}┌" + "─" * (w - 2) + f"┐{RESET}",
            f"{BOLD}{CYAN}│  📋 蒙特卡罗仿真输入参数显式清单 (Code Parameter Manifest)" + " " * (w - 63) + f"│{RESET}",
            f"{BOLD}{CYAN}├" + "─" * (w - 2) + f"┤{RESET}",
            f"│  {BOLD}目标路径{RESET}: {self.target[:42]:<42}  {DIM}[{self.extraction_latency_ms:.2f} ms]{RESET}" + " " * max(0, w - 60 - len(self.target[:42])) + "│",
            f"│  {BOLD}工程架构{RESET}: {'多文件 CMake 工程 (' + str(len(self.source_files_analyzed)) + ' 单元)' if self.is_multi_file_project else '单文件 C++ 源码':<50}" + " " * (w - 66) + "│",
        ]

        if self.drift_warnings:
            lines.append(f"{CYAN}├" + "─" * (w - 2) + f"┤{RESET}")
            lines.append(f"│  {BOLD}{RED}⚠️  参数漂移警报 (Intent vs Code Drift):{RESET}" + " " * (w - 44) + "│")
            for dw in self.drift_warnings:
                lines.append(f"│    {RED}• {dw[:70]}{RESET}" + " " * max(0, w - 7 - len(dw[:70])) + "│")

        # 1. 辐射源项
        lines.append(f"{CYAN}├" + "─" * (w - 2) + f"┤{RESET}")
        lines.append(f"│  {BOLD}{MAGENTA}[1. 辐射源项参数 (Radiation Source)]{RESET}" + " " * (w - 39) + "│")
        parts = ", ".join(self.source.particles) if self.source.particles else "未显式指定"
        energies = ", ".join(self.source.energies) if self.source.energies else "未显式指定"
        lines.append(f"│    • 粒子种类: {GREEN}{parts:<20}{RESET} 初级能量: {GREEN}{energies:<26}{RESET}│")
        pos_str = self.source.position or "(0, 0, 0)"
        lines.append(f"│    • 发射位置: {pos_str:<20} 事件历史: {GREEN}{self.source.beam_on_events or '未指定':<26}{RESET}│")
        ang_color = GREEN if ("4pi" in self.source.angular_distribution_type.lower() or "各向同性" in self.source.angular_distribution_type) else YELLOW
        lines.append(f"│    • 角分布测度: {ang_color}{self.source.angular_distribution_type:<42}{RESET}" + " " * (w - 60) + "│")
        if self.source.direction:
            lines.append(f"│    • 动量向量: {self.source.direction:<52}" + " " * (w - 68) + "│")

        # 2. 几何材料
        lines.append(f"{CYAN}├" + "─" * (w - 2) + f"┤{RESET}")
        lines.append(f"│  {BOLD}{MAGENTA}[2. 几何拓扑与材料 (Geometry & Materials)]{RESET}" + " " * (w - 45) + "│")
        if self.geometry:
            for g in self.geometry:
                dim_str = ", ".join(f"{k}={v}" for k, v in g.parameters.items())[:38]
                mat_str = g.material or "未指定"
                lines.append(f"│    • {BOLD}{g.solid_name:<14}{RESET} [{g.shape_type:<8}] 介质: {CYAN}{mat_str:<12}{RESET} 角色: {g.role:<14}│")
                lines.append(f"│      └─ 尺寸: {DIM}{dim_str:<34}{RESET} 摆位: {DIM}{g.placement_position or '原点 (0,0,0)':<20}{RESET}│")
        else:
            lines.append(f"│    • (未检测到几何体定义)" + " " * (w - 29) + "│")

        # 3. 物理过程
        lines.append(f"{CYAN}├" + "─" * (w - 2) + f"┤{RESET}")
        lines.append(f"│  {BOLD}{MAGENTA}[3. 物理列表与过程 (Physics List)]{RESET}" + " " * (w - 36) + "│")
        pl = self.physics.physics_list_name or "自定义 ModularPhysicsList"
        lines.append(f"│    • 主物理列表: {GREEN}{pl:<32}{RESET}" + " " * (w - 51) + "│")
        if self.physics.modular_physics:
            lines.append(f"│    • 附加模块: {', '.join(self.physics.modular_physics)[:56]:<56}│")
        if self.physics.special_features:
            lines.append(f"│    • 物理特性: {', '.join(self.physics.special_features)[:56]:<56}│")

        # 4. 计分响应
        lines.append(f"{CYAN}├" + "─" * (w - 2) + f"┤{RESET}")
        lines.append(f"│  {BOLD}{MAGENTA}[4. 计分响应与统计量 (Tally & Scoring)]{RESET}" + " " * (w - 42) + "│")
        if self.tallies:
            for t in self.tallies:
                wt = f"{GREEN}权重保留{RESET}" if t.weight_preservation else f"{RED}丢失权重{RESET}"
                lines.append(f"│    • {BOLD}{t.observable_type:<24}{RESET} 变量: `{t.scoring_variable or 'tally'}` [{wt}]" + " " * (w - 57) + "│")
                if t.energy_window:
                    lines.append(f"│      └─ 能谱窗口: {CYAN}{t.energy_window:<50}{RESET}│")
                if t.boundary_filter:
                    lines.append(f"│      └─ 边界门控: {DIM}{t.boundary_filter:<50}{RESET}│")
        else:
            lines.append(f"│    • (未检测到显式计分统计)" + " " * (w - 31) + "│")

        # 5. 运行控制
        lines.append(f"{CYAN}├" + "─" * (w - 2) + f"┤{RESET}")
        lines.append(f"│  {BOLD}{MAGENTA}[5. 运行控制与减方差 (Run Control)]{RESET}" + " " * (w - 37) + "│")
        lines.append(f"│    • 运行引擎: {self.run_control.run_manager_type:<24} 减方差: {GREEN}{self.run_control.variance_reduction:<22}{RESET}│")
        lines.append(f"{BOLD}{CYAN}└" + "─" * (w - 2) + f"┘{RESET}")
        return "\n".join(lines)


class CodeManifestExtractor:
    """
    确定性代码实参提取引擎
    利用 Tree-sitter AST 解析全套编译单元，提取结构化物理参数
    """

    def __init__(self):
        self.language = Language(tscpp.language())
        self.parser = Parser(self.language)
        self.cmake_parser = CMakeParser()

    def extract(self, target_path: str, intent: Optional[PhysicsIntent] = None) -> CodeParameterManifest:
        """主入口：从指定文件或 CMake 工程提取完整代码实参清单"""
        t0 = time.perf_counter()
        p = Path(target_path).resolve()
        if not p.exists():
            raise FileNotFoundError(f"目标路径不存在: {target_path}")

        is_cmake_proj = p.is_dir() or p.name == "CMakeLists.txt"
        manifest = CodeParameterManifest(target=str(target_path))

        all_sources: List[Tuple[str, str]] = []  # [(filename, code_text)]

        if is_cmake_proj:
            manifest.is_multi_file_project = True
            proj_info = self.cmake_parser.parse_project(str(p))
            manifest.project_name = proj_info.project_name
            manifest.source_files_analyzed = proj_info.all_source_files

            for sf in proj_info.all_source_files:
                sf_path = Path(sf)
                if sf_path.is_file():
                    all_sources.append((sf_path.name, sf_path.read_text(encoding="utf-8", errors="replace")))
        else:
            manifest.is_multi_file_project = False
            manifest.source_files_analyzed = [str(p)]
            all_sources.append((p.name, p.read_text(encoding="utf-8", errors="replace")))

        # 遍历所有源文件提取微观参数
        self._extract_from_sources(all_sources, manifest)

        # 检查意图与代码实参漂移
        if intent:
            manifest.drift_warnings = self._detect_drift(manifest, intent)

        manifest.extraction_latency_ms = (time.perf_counter() - t0) * 1000.0
        return manifest

    def _extract_from_sources(self, sources: List[Tuple[str, str]], manifest: CodeParameterManifest) -> None:
        """从源文件集合中提取五大维度物理参数"""
        materials_map: Dict[str, str] = {}       # var_name -> material_name
        solids_by_name: Dict[str, GeometrySolid] = {} # solid_name -> GeometrySolid
        solids_by_var: Dict[str, str] = {}            # var_name -> solid_name
        logical_map: Dict[str, Tuple[str, str]] = {}  # log_var/log_name -> (solid_ref, mat_ref)

        for filename, code_text in sources:
            code_bytes = code_text.encode("utf-8")
            tree = self.parser.parse(code_bytes)
            self._walk_ast_postorder(
                node=tree.root_node,
                code_bytes=code_bytes,
                filename=filename,
                manifest=manifest,
                materials_map=materials_map,
                solids_by_name=solids_by_name,
                solids_by_var=solids_by_var,
                logical_map=logical_map,
            )

        manifest.geometry = list(solids_by_name.values())
        manifest.materials_catalog = list(set(materials_map.values()))

        # 后处理推断与补全
        self._post_process(manifest)

    def _walk_ast_postorder(
        self,
        node: Node,
        code_bytes: bytes,
        filename: str,
        manifest: CodeParameterManifest,
        materials_map: Dict[str, str],
        solids_by_name: Dict[str, GeometrySolid],
        solids_by_var: Dict[str, str],
        logical_map: Dict[str, Tuple[str, str]],
    ) -> None:
        """后根顺序遍历 (先遍历子节点，再处理父节点) 便于自底向上组合嵌套对象"""
        for child in node.children:
            self._walk_ast_postorder(
                node=child,
                code_bytes=code_bytes,
                filename=filename,
                manifest=manifest,
                materials_map=materials_map,
                solids_by_name=solids_by_name,
                solids_by_var=solids_by_var,
                logical_map=logical_map,
            )

        line_num = node.start_point.row + 1
        text = self._node_text(node, code_bytes).strip()

        # ── 1. 函数调用表达式 (call_expression) ──
        if node.type == "call_expression":
            # (A) 粒子种类查找
            if "FindParticle" in text:
                match = re.search(r'FindParticle\s*\(\s*(?:isN\s*\?\s*)?"([^"]+)"(?:\s*:\s*"([^"]+)")?\s*\)', text)
                if match:
                    for p in match.groups():
                        if p and p not in manifest.source.particles:
                            manifest.source.particles.append(p)
                            manifest.source.evidence["particle"] = f"{filename}:{line_num}"

            elif any(f"G4{name}::" in text for name in ("Gamma", "Neutron", "Electron", "Proton")):
                for name in ("gamma", "neutron", "e-", "proton"):
                    if name in text.lower() and name not in manifest.source.particles:
                        manifest.source.particles.append(name)
                        manifest.source.evidence["particle"] = f"{filename}:{line_num}"

            # (B) 初级能量
            elif "SetParticleEnergy" in text:
                args = self._extract_call_args(node, code_bytes)
                if args:
                    ene_expr = args[0]
                    manifest.source.energies.append(ene_expr)
                    manifest.source.evidence["energy"] = f"{filename}:{line_num}"

            # (C) 发射位置
            elif "SetParticlePosition" in text:
                args = self._extract_call_args(node, code_bytes)
                if args:
                    manifest.source.position = args[0]
                    manifest.source.evidence["position"] = f"{filename}:{line_num}"

            # (D) 发射方向与角分布
            elif "SetParticleMomentumDirection" in text:
                args = self._extract_call_args(node, code_bytes)
                if args:
                    dir_expr = args[0]
                    manifest.source.direction = dir_expr
                    manifest.source.evidence["direction"] = f"{filename}:{line_num}"

                    if "G4RandomDirection" in dir_expr or "G4RandomDirection" in text:
                        manifest.source.angular_distribution_type = "4π 球面度各向同性 (G4RandomDirection)"
                    elif re.search(r"G4ThreeVector\s*\(\s*[-+]?[0-9].*?\)", dir_expr) or re.search(r"\{\s*[-+]?[0-9].*?\}", dir_expr):
                        manifest.source.angular_distribution_type = f"单向平行束 (Pencil Beam: {dir_expr})"
                    else:
                        manifest.source.angular_distribution_type = "空间角分布 (自定义抽样)"

            # (E) 运行历史数
            elif "BeamOn" in text:
                args = self._extract_call_args(node, code_bytes)
                if args:
                    manifest.source.beam_on_events = args[0]
                    manifest.source.evidence["beam_on"] = f"{filename}:{line_num}"

            # (F) 物理列表初始化
            elif "SetUserInitialization" in text and any(k in text for k in ("Physics", "BERT", "QGSP", "FTFP", "Shielding")):
                args = self._extract_call_args(node, code_bytes)
                if args:
                    pl = args[0].replace("new ", "").replace("()", "").strip()
                    manifest.physics.physics_list_name = pl
                    manifest.physics.evidence["physics_list"] = f"{filename}:{line_num}"

            # (G) 模块化物理过程注册
            elif "RegisterPhysics" in text:
                args = self._extract_call_args(node, code_bytes)
                if args:
                    mod_p = args[0].replace("new ", "").replace("()", "").strip()
                    manifest.physics.modular_physics.append(mod_p)
                    manifest.physics.evidence[f"mod_{len(manifest.physics.modular_physics)}"] = f"{filename}:{line_num}"

            # (H) 产生截止阈
            elif "SetDefaultCutValue" in text:
                args = self._extract_call_args(node, code_bytes)
                if args:
                    manifest.physics.production_cuts = args[0]
                    manifest.physics.evidence["cuts"] = f"{filename}:{line_num}"

            # (I) 材料检索 (FindOrBuildMaterial)
            elif "FindOrBuildMaterial" in text:
                args = self._extract_call_args(node, code_bytes)
                if args:
                    mat_name = args[0].strip('"')
                    assigned_var = self._find_assigned_var(node, code_bytes)
                    if assigned_var:
                        materials_map[assigned_var] = mat_name
                    materials_map[mat_name] = mat_name

        # ── 2. 对象创建表达式 (new_expression) ──
        elif node.type == "new_expression":
            type_node = node.child_by_field_name("type")
            type_name = self._node_text(type_node, code_bytes).strip() if type_node else ""
            args = self._extract_call_args(node, code_bytes)

            # (A) 几何实体固体 (Solids: G4Box, G4Tubs, G4Sphere, etc.)
            if type_name in ("G4Box", "G4Tubs", "G4Sphere", "G4Cons", "G4Trd"):
                if args:
                    solid_name = args[0].strip('"')
                    params = {}
                    if type_name == "G4Box" and len(args) >= 4:
                        params = {"half_x": args[1], "half_y": args[2], "half_z": args[3]}
                    elif type_name == "G4Tubs" and len(args) >= 6:
                        params = {"rmin": args[1], "rmax": args[2], "half_z": args[3], "sphi": args[4], "dphi": args[5]}
                    elif type_name == "G4Sphere" and len(args) >= 7:
                        params = {"rmin": args[1], "rmax": args[2], "sphi": args[3], "dphi": args[4], "stheta": args[5], "dtheta": args[6]}

                    role = "结构介质体 (Passive)"
                    if "world" in solid_name.lower():
                        role = "World 容器"
                    elif any(k in solid_name.lower() for k in ("crystal", "hpge", "det", "active")):
                        role = "灵敏探测晶体 (Active Crystal)"
                    elif "dead" in solid_name.lower():
                        role = "非活性死层 (Dead Layer)"
                    elif any(k in solid_name.lower() for k in ("pb", "lead", "bpe", "shield", "fe")):
                        role = "屏蔽材料体 (Shielding)"

                    solid_obj = GeometrySolid(
                        solid_name=solid_name,
                        shape_type=type_name,
                        parameters=params,
                        role=role,
                        code_location=f"{filename}:{line_num}",
                    )
                    solids_by_name[solid_name] = solid_obj

                    assigned_var = self._find_assigned_var(node, code_bytes)
                    if assigned_var:
                        solids_by_var[assigned_var] = solid_name

            # (B) 逻辑体 (G4LogicalVolume: Solid -> Material -> LogicalName)
            elif type_name == "G4LogicalVolume" and len(args) >= 3:
                solid_ref = args[0].strip('"')
                mat_ref = args[1].strip('"')
                log_name = args[2].strip('"')

                # 解析 solid_ref 可能是变量名或内联 new G4Box("World", ...)
                actual_solid_name = solids_by_var.get(solid_ref, solid_ref)
                if "new G4" in solid_ref or "G4" in solid_ref:
                    m_s = re.search(r'"([^"]+)"', solid_ref)
                    if m_s and m_s.group(1) in solids_by_name:
                        actual_solid_name = m_s.group(1)

                # 解析 mat_ref 可能是变量名或内联 FindOrBuildMaterial("...")
                actual_mat_name = materials_map.get(mat_ref, mat_ref)
                if "FindOrBuildMaterial" in mat_ref:
                    m_m = re.search(r'FindOrBuildMaterial\s*\(\s*"([^"]+)"', mat_ref)
                    if m_m:
                        actual_mat_name = m_m.group(1)

                assigned_var = self._find_assigned_var(node, code_bytes)
                if assigned_var:
                    logical_map[assigned_var] = (actual_solid_name, actual_mat_name)
                logical_map[log_name] = (actual_solid_name, actual_mat_name)

                # 关联材料到对应 Solid
                if actual_solid_name in solids_by_name:
                    solids_by_name[actual_solid_name].material = actual_mat_name
                    solids_by_name[actual_solid_name].logical_volume = log_name

            # (C) 物理放置 (G4PVPlacement)
            elif type_name == "G4PVPlacement" and len(args) >= 4:
                # new G4PVPlacement(rot, trans, logicVol, name, motherVol, ...)
                trans = args[1] if len(args) > 1 else "{0,0,0}"
                log_ref = args[2] if len(args) > 2 else ""
                mother_ref = args[4].strip('"') if len(args) > 4 else "World"

                if trans in ("{}", "G4ThreeVector()", "0"):
                    trans = "原点 (0, 0, 0)"

                # 匹配所属 Solid
                actual_solid_name = None
                if log_ref in logical_map:
                    actual_solid_name, _ = logical_map[log_ref]
                elif "new G4LogicalVolume" in log_ref or "G4LogicalVolume" in log_ref:
                    m_l = re.search(r'"([^"]+)"', log_ref)
                    if m_l and m_l.group(1) in solids_by_name:
                        actual_solid_name = m_l.group(1)
                elif args[3].strip('"') in solids_by_name:
                    actual_solid_name = args[3].strip('"')

                if actual_solid_name and actual_solid_name in solids_by_name:
                    solids_by_name[actual_solid_name].placement_position = trans
                    solids_by_name[actual_solid_name].placement_mother = mother_ref

            # (D) 自定义材料 (new G4Material)
            elif type_name == "G4Material" and len(args) >= 2:
                mat_name = args[0].strip('"')
                density = args[1]
                full_mat_str = f"{mat_name} ({density})"
                materials_map[mat_name] = full_mat_str
                assigned_var = self._find_assigned_var(node, code_bytes)
                if assigned_var:
                    materials_map[assigned_var] = full_mat_str

            # (E) 物理列表 (new QGSP_BERT / new QGSP_BIC_HP)
            elif any(k in type_name for k in ("QGSP", "FTFP", "Shielding", "BERT", "BIC")):
                manifest.physics.physics_list_name = type_name
                manifest.physics.evidence["physics_list"] = f"{filename}:{line_num}"

            # (F) RunManager
            elif "RunManager" in type_name:
                manifest.run_control.run_manager_type = f"{type_name} ({'多线程' if 'MT' in type_name else '单线程'})"
                manifest.run_control.evidence["run_manager"] = f"{filename}:{line_num}"

        # ── 3. 条件判断与计分窗口 (Tally: Energy Window / Boundary Gate) ──
        elif node.type == "if_statement":
            cond_node = node.child_by_field_name("condition")
            cond_text = self._node_text(cond_node, code_bytes).strip() if cond_node else ""
            consequent_node = node.child_by_field_name("consequence")
            conseq_text = self._node_text(consequent_node, code_bytes).strip() if consequent_node else ""

            # (A) 能谱全能峰判选窗口 (如 edep >= 0.660 && edep <= 0.663)
            if "edep" in cond_text.lower() and (">=" in cond_text or "<=" in cond_text) and ("&&" in cond_text or "and" in cond_text):
                manifest.tallies.append(TallyManifest(
                    tally_id=f"tally_peak_{len(manifest.tallies)+1}",
                    observable_type="全能峰探测效率 (Peak Efficiency Window)",
                    energy_window=cond_text.strip("()"),
                    scoring_variable=self._extract_tally_var(conseq_text),
                    weight_preservation="GetWeight" in conseq_text or "GetWeight" in text,
                    code_location=f"{filename}:{line_num}",
                ))

            # (B) 几何边界穿透判定 (fGeomBoundary)
            elif "fGeomBoundary" in cond_text or "GetStepStatus" in cond_text:
                is_post = "PostStepPoint" in text or "post" in text
                manifest.tallies.append(TallyManifest(
                    tally_id=f"tally_flux_{len(manifest.tallies)+1}",
                    observable_type="界面粒子泄漏谱 / 通量 (Boundary Leakage)",
                    boundary_filter=f"{'PostStepPoint' if is_post else 'PreStepPoint'} == fGeomBoundary",
                    scoring_variable=self._extract_tally_var(conseq_text),
                    weight_preservation="GetWeight" in text or "GetWeight" in conseq_text,
                    code_location=f"{filename}:{line_num}",
                ))

    def _post_process(self, manifest: CodeParameterManifest) -> None:
        """根据上下文后处理推断与补全"""
        for sf in manifest.source_files_analyzed:
            p = Path(sf)
            if p.is_file():
                txt = p.read_text(encoding="utf-8", errors="replace")
                if "cosTheta" in txt and "sinTheta" in txt and ("twopi" in txt or "3.1415" in txt or "CLHEP::pi" in txt):
                    manifest.source.angular_distribution_type = "4π 球面度空间各向同性 (Uniform Sphere: sinθ dθ dφ)"
                if "G4EmStandardPhysics_option4" in txt:
                    if "高精度电磁物理模型 (EM Opt4)" not in manifest.physics.special_features:
                        manifest.physics.special_features.append("高精度电磁物理模型 (EM Opt4)")
                if "HP" in (manifest.physics.physics_list_name or "") or "BIC_HP" in txt:
                    if "连续能量高精度中子库输运 (HP/NeutronHP)" not in manifest.physics.special_features:
                        manifest.physics.special_features.append("连续能量高精度中子库输运 (HP/NeutronHP)")
                if "G4RadioactiveDecay" in txt:
                    if "放射性衰变过程 (RadioactiveDecay)" not in manifest.physics.special_features:
                        manifest.physics.special_features.append("放射性衰变过程 (RadioactiveDecay)")
                if "GetWeight" in txt:
                    manifest.run_control.variance_reduction = "减方差重要抽样 / 粒子统计权重流动 (Variance Reduction Enabled)"

    def _detect_drift(self, manifest: CodeParameterManifest, intent: PhysicsIntent) -> List[str]:
        """双向交叉校验：检测用户意图 (Ring 0) 与代码实参 (Ring 1-4) 之间的参数漂移"""
        warnings = []

        # 1. 源项角分布漂移
        if intent.source_symmetry == SourceSymmetry.ISOTROPIC_4PI:
            if "单向" in manifest.source.angular_distribution_type or "Pencil" in manifest.source.angular_distribution_type:
                warnings.append(
                    f"源项角分布漂移: 用户意图要求 4π 各向同性立体角辐射源，但代码实际设置为单向平行发射 [{manifest.source.direction}]！"
                )
        elif intent.source_symmetry == SourceSymmetry.PENCIL_BEAM:
            if "各向同性" in manifest.source.angular_distribution_type or "4π" in manifest.source.angular_distribution_type:
                warnings.append(
                    f"源项角分布漂移: 用户意图要求单向准直平行束，但代码实际采用了 4π 球面度立体角抽样！"
                )

        # 2. 粒子种类漂移
        if intent.source_desc:
            desc_l = intent.source_desc.lower()
            if "cs-137" in desc_l or "gamma" in desc_l:
                if manifest.source.particles and not any("gamma" in p.lower() for p in manifest.source.particles):
                    warnings.append(
                        f"粒子种类漂移: 意图声明为伽马辐射源 (Cs-137)，但代码中配置发射粒子为 [{', '.join(manifest.source.particles)}]！"
                    )

        # 3. 减方差权重漂移
        if intent.variance_reduction:
            if not any(t.weight_preservation for t in manifest.tallies):
                warnings.append(
                    f"统计权重漂移: 意图涉及减方差重要抽样，但代码计分器未调用 GetWeight() 累加统计权重，将导致统计严重偏倚！"
                )

        return warnings

    def _extract_call_args(self, node: Node, code_bytes: bytes) -> List[str]:
        """从 call_expression 或 new_expression 提取实参列表文本"""
        arg_node = node.child_by_field_name("arguments")
        if not arg_node:
            for child in node.children:
                if child.type == "argument_list":
                    arg_node = child
                    break
        if not arg_node:
            return []

        args = []
        for c in arg_node.named_children:
            args.append(self._node_text(c, code_bytes).strip())
        return args

    def _find_assigned_var(self, node: Node, code_bytes: bytes) -> Optional[str]:
        """向上追溯当前右值表达式被赋值给哪一个左值变量"""
        curr: Optional[Node] = node.parent
        while curr and curr.type not in ("function_definition", "translation_unit"):
            if curr.type == "init_declarator":
                d_node = curr.child_by_field_name("declarator")
                while d_node and d_node.type == "pointer_declarator":
                    d_node = d_node.child_by_field_name("declarator")
                if d_node:
                    return self._node_text(d_node, code_bytes).strip().lstrip("*")
            elif curr.type == "assignment_expression":
                l_node = curr.child_by_field_name("left")
                if l_node:
                    return self._node_text(l_node, code_bytes).strip().lstrip("*")
            curr = curr.parent
        return None

    def _extract_tally_var(self, text: str) -> Optional[str]:
        """从计分语句中提取累加变量"""
        match = re.search(r'([a-zA-Z0-9_]+)\s*(?:\+\+|[\+\*]=)', text)
        if match:
            return match.group(1)
        match = re.search(r'([a-zA-Z0-9_]+)\s*\[.*?\]\s*\+\+', text)
        if match:
            return match.group(1)
        return None

    def _node_text(self, node: Optional[Node], code_bytes: bytes) -> str:
        if not node:
            return ""
        return code_bytes[node.start_byte:node.end_byte].decode("utf-8", errors="replace")
