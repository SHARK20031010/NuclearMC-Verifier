#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/companion.py —— MC-V 物理伴侣核心交互引擎 (Physics Companion Inspector)

支持：
1. 零 Flag 极简即时体检 (`mc-v <文件>` 或 `mc-v <工程目录>` / `mc-v .`)
2. 自然语言规约对齐体检 (`mc-v <目标> "大白话需求说明"`) —— 由 LLM 自动拆解结构化槽位
3. 一键物理修复 (`mc-v fix <目标>`) —— 支持单文件与多文件跨模块自动打补丁
4. 实时伴侣监视模式 (`mc-v watch <目标>`) —— 监听所有源码修改，毫秒级刷新
5. 8 大基准自动化评测 (`mc-v eval`)
"""

from __future__ import annotations

import os
import re
import sys
import time
import shutil
from pathlib import Path
from typing import Dict, List, Optional, Any, Tuple
from pydantic import BaseModel, Field

from guardrail.ir.pst_builder import PSTBuilder
from guardrail.ir.pst_verifier import PSTVerifier, PSTVerificationReport, PSTViolation
from guardrail.ir.pst_schema import ConservationDomain, PSTGraph
from guardrail.ir.cmake_parser import CMakeParser, CMakeProjectInfo, CMakeTarget
from guardrail.ir.pst_linker import PSTLinker
from guardrail.ir.intent_extractor import IntentExtractor, PhysicsIntent


# ──────────────────────── ANSI 终端彩色定义 ────────────────────────
RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
MAGENTA = "\033[35m"
CYAN = "\033[36m"
BG_RED = "\033[41m"
BG_GREEN = "\033[42m"


class SpecContract(BaseModel):
    """前置物理规约契约（环 0）"""
    observable: str = "粒子输运观测量"
    source: str = "辐射源项 (动量/能量)"
    temporal_metric: str = "时间演化测度"
    normalization: str = "统计归一化基准"
    inferred_from: str = "代码注释自动推断"


class DiagnosticEnvelope(BaseModel):
    """统一物理诊断封包"""
    source_file: str
    code_lines: int
    latency_ms: float
    spec_contract: SpecContract
    physics_intent: PhysicsIntent
    passed: bool
    violations: List[PSTViolation] = Field(default_factory=list)
    nodes_count: int = 0
    edges_count: int = 0
    # 多文件 CMake 工程增强字段
    is_project: bool = False
    project_name: Optional[str] = None
    target_name: Optional[str] = None
    source_files: List[str] = Field(default_factory=list)
    definitions: List[str] = Field(default_factory=list)
    cross_edges_count: int = 0


class MCCompanion:
    """MC-V 物理伴侣核心引擎"""

    def __init__(self):
        self.builder = PSTBuilder()
        self.verifier = PSTVerifier()
        self.cmake_parser = CMakeParser()
        self.linker = PSTLinker()
        self.intent_extractor = IntentExtractor()

    def extract_intent(self, code_text: str, user_spec: Optional[str] = None) -> Tuple[PhysicsIntent, SpecContract]:
        """通过 LLM 意图解析器抽取标准物理意图卡，并适配为 SpecContract"""
        intent = self.intent_extractor.extract(code_text, user_spec)
        contract = SpecContract(
            observable=intent.observable_desc,
            source=intent.source_desc,
            temporal_metric=intent.timing_clock.value,
            normalization=intent.normalization_basis,
            inferred_from=intent.extraction_source,
        )
        return intent, contract

    def inspect(self, filepath: str, user_spec: Optional[str] = None) -> DiagnosticEnvelope:
        """对单文件或整套 CMake 蒙卡工程执行物理语义树分析并生成诊断封包"""
        p = Path(filepath).resolve()
        if not p.exists():
            raise FileNotFoundError(f"文件或工程未找到: {filepath}")

        # 判断是否为 CMake 工程目录或 CMakeLists.txt
        is_cmake_proj = p.is_dir() or p.name == "CMakeLists.txt"

        if is_cmake_proj:
            t0 = time.perf_counter()
            proj_info = self.cmake_parser.parse_project(str(p))
            if not proj_info.all_source_files:
                raise ValueError(f"在工程路径 {filepath} 下未找到任何 C++ 源码文件 (.cc / .cpp)")

            # 读取全部工程代码用于规约推断与行数统计
            all_texts = []
            total_lines = 0
            for sf in proj_info.all_source_files:
                s_path = Path(sf)
                if s_path.is_file():
                    txt = s_path.read_text(encoding="utf-8", errors="replace")
                    all_texts.append(txt)
                    total_lines += len(txt.splitlines())

            combined_corpus = "\n".join(all_texts)
            intent, contract = self.extract_intent(combined_corpus, user_spec)

            # 调用 PSTLinker 缝合成全景全局图
            global_graph = self.linker.link_project(proj_info)

            # 形式化守恒断言核验 (动态注入 PhysicsIntent)
            report = self.verifier.verify(global_graph, intent=intent)
            t1 = time.perf_counter()

            latency_ms = (t1 - t0) * 1000.0
            cross_edges = sum(1 for e in global_graph.edges if "cross" in e.edge_id)
            target_name = proj_info.targets[0].target_name if proj_info.targets else proj_info.project_name

            return DiagnosticEnvelope(
                source_file=str(p),
                code_lines=total_lines,
                latency_ms=round(latency_ms, 2),
                spec_contract=contract,
                physics_intent=intent,
                passed=report.passed,
                violations=report.violations,
                nodes_count=len(global_graph.nodes),
                edges_count=len(global_graph.edges),
                is_project=True,
                project_name=proj_info.project_name,
                target_name=target_name,
                source_files=proj_info.all_source_files,
                definitions=proj_info.definitions,
                cross_edges_count=cross_edges,
            )

        # 单文件处理流程
        code_text = p.read_text(encoding="utf-8", errors="replace")
        code_lines = len(code_text.splitlines())

        t0 = time.perf_counter()
        # 1. LLM 结构化意图抽取
        intent, contract = self.extract_intent(code_text, user_spec)

        # 2. PST 构建与图组装
        graph = self.builder.build(code_text, filename=str(p))

        # 3. 守恒不变式形式化核验 (动态注入 PhysicsIntent)
        report = self.verifier.verify(graph, intent=intent)
        t1 = time.perf_counter()

        latency_ms = (t1 - t0) * 1000.0

        return DiagnosticEnvelope(
            source_file=str(p),
            code_lines=code_lines,
            latency_ms=round(latency_ms, 2),
            spec_contract=contract,
            physics_intent=intent,
            passed=report.passed,
            violations=report.violations,
            nodes_count=len(graph.nodes),
            edges_count=len(graph.edges),
            is_project=False,
            source_files=[str(p)],
        )

    def render_dashboard(self, env: DiagnosticEnvelope) -> str:
        """渲染彩色现代终端看板 (类似 Rust/Cargo 视觉风格)"""
        lines = []
        width = 68
        border = "─" * width

        lines.append(f"{BOLD}{CYAN}╭{border}╮{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}  {BOLD}MC-V 物理伴侣 · 粒子输运形式化守恒核验{RESET}"
                     f" {DIM}(耗时: {env.latency_ms} ms){RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")

        if env.is_project:
            proj_title = f"{env.project_name or 'Project'}"
            if env.target_name and env.target_name != env.project_name:
                proj_title += f" [目标: {env.target_name}]"
            lines.append(f"{BOLD}{CYAN}│{RESET}  {DIM}工程目标:{RESET} {BOLD}{proj_title}{RESET} {DIM}({len(env.source_files)} 编译单元 · {env.code_lines} 行代码){RESET}".ljust(width + 17) + f"{BOLD}{CYAN}│{RESET}")
            lines.append(f"{BOLD}{CYAN}│{RESET}  {DIM}PST 全景图:{RESET} {env.nodes_count} 节点, {env.edges_count} 边 {DIM}(跨模块纽带: {env.cross_edges_count} 条){RESET}".ljust(width + 17) + f"{BOLD}{CYAN}│{RESET}")
            if env.definitions:
                macro_str = ", ".join(env.definitions)
                lines.append(f"{BOLD}{CYAN}│{RESET}  {DIM}编译宏定义:{RESET} {YELLOW}{macro_str}{RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
        else:
            lines.append(f"{BOLD}{CYAN}│{RESET}  {DIM}目标文件:{RESET} {BOLD}{env.source_file}{RESET} {DIM}({env.code_lines} 行代码 · PST: {env.nodes_count} 节点, {env.edges_count} 边){RESET}".ljust(width + 17) + f"{BOLD}{CYAN}│{RESET}")

        lines.append(f"{BOLD}{CYAN}├{border}┤{RESET}")

        # 1. 结构化物理意图回执 (Ring 0 Intent)
        pi = env.physics_intent
        vr_str = f"{YELLOW}开启 (粒子权重流强约束){RESET}" if pi.variance_reduction else f"{DIM}关闭 (单位权重输运){RESET}"
        lines.append(f"{BOLD}{CYAN}│{RESET}  {BOLD}📋 结构化物理意图契约 (Ring 0 Intent){RESET} {DIM}[{pi.extraction_source}]{RESET}".ljust(width + 13) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 观测量:   {YELLOW}{pi.observable_desc}{RESET} {DIM}[{pi.observable.value}]{RESET}".ljust(width + 17) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 辐射源:   {YELLOW}{pi.source_desc}{RESET} {DIM}[{pi.source_symmetry.value}]{RESET}".ljust(width + 17) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 时间测度: {YELLOW}{pi.timing_clock.value}{RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 减方差:   {vr_str}".ljust(width + 17) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 归一基准: {YELLOW}{pi.normalization_basis}{RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}├{border}┤{RESET}")

        # 2. 五大正交守恒总线状态指示灯
        violated_domains = {v.domain for v in env.violations}
        lines.append(f"{BOLD}{CYAN}│{RESET}  {BOLD}🛡️ 五大正交守恒总线仪表盘 (Conservation Invariants){RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")

        bus_items = [
            (ConservationDomain.PHASE_SPACE, "相空间与角分布测度 (PhaseSpace)"),
            (ConservationDomain.VARIANCE,    "统计权重流无偏性 (Variance)  "),
            (ConservationDomain.CAUSALITY,   "实验绝对时钟与因果 (Causality) "),
            (ConservationDomain.LIFECYCLE,   "步进生命周期与边界 (Lifecycle) "),
            (ConservationDomain.NUCLEAR,     "微观截面与核素衰变 (Nuclear)   "),
        ]

        for dom, name in bus_items:
            if dom in violated_domains:
                status_icon = f"{RED}🔴 阻断 (VIOLATION){RESET}"
            else:
                status_icon = f"{GREEN}🟢 正常 (PASS){RESET}"
            lines.append(f"{BOLD}{CYAN}│{RESET}    {name} : {status_icon}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")

        lines.append(f"{BOLD}{CYAN}╰{border}╯{RESET}")

        # 3. 详细违约卡片与 Diff 处方
        if env.passed:
            lines.append(f"\n{BOLD}{GREEN}  ✨ 恭喜：物理语义树符合全部守恒不变式！仿真代码可安全交付或提交超算。{RESET}\n")
        else:
            lines.append(f"\n{BOLD}{RED}🚨 检出 {len(env.violations)} 项物理守恒违约！请参阅以下机理诊断与处方：{RESET}\n")

            for idx, v in enumerate(env.violations, start=1):
                lines.append(f"{BOLD}┌─ [{idx}] {v.defect_name} · 规则: {v.rule_id} [{v.domain.value}]{RESET}")
                
                # 确定所在文件名
                src_filename = Path(v.source_file).name if v.source_file else (Path(env.source_file).name if not env.is_project else "")
                loc_str = ""
                if src_filename:
                    loc_str += f"{CYAN}{src_filename}{RESET}:"
                if v.line_range:
                    loc_str += f"第 {v.line_range[0]} 行"

                if loc_str:
                    lines.append(f"│  {DIM}代码位置: {loc_str}{RESET}")
                if v.code_evidence:
                    lines.append(f"│  {DIM}现场代码: {YELLOW}{v.code_evidence}{RESET}")
                lines.append(f"│")
                lines.append(f"│  {BOLD}🔍 物理机理诊断:{RESET}")
                for pline in v.description.splitlines():
                    lines.append(f"│     {pline}")
                lines.append(f"│")
                lines.append(f"│  {BOLD}💡 规范修复建议:{RESET} {CYAN}{v.remedy}{RESET}")

                if v.diff:
                    lines.append(f"│")
                    lines.append(f"│  {BOLD}🔧 推荐 Diff 补丁 ({src_filename}):{RESET}")
                    for dline in v.diff.splitlines():
                        if dline.startswith("-"):
                            lines.append(f"│     {RED}{dline}{RESET}")
                        elif dline.startswith("+"):
                            lines.append(f"│     {GREEN}{dline}{RESET}")
                        else:
                            lines.append(f"│     {dline}")
                lines.append(f"{BOLD}└────────────────────────────────────────────────────────────{RESET}\n")

            fix_cmd_target = env.source_file if not env.is_project else (env.project_name or ".")
            lines.append(f"{DIM}👉 提示: 运行 {BOLD}mc-v fix {fix_cmd_target}{RESET}{DIM} 即可一键自动应用上述推荐补丁{RESET}\n")

        return "\n".join(lines)

    def fix(self, filepath: str) -> bool:
        """一键修复：应用推荐补丁并自动复核 (支持单文件与多文件工程)"""
        p = Path(filepath).resolve()
        if not p.exists():
            print(f"{RED}错误: 目标文件或工程未找到: {filepath}{RESET}", file=sys.stderr)
            return False

        env = self.inspect(filepath)
        if env.passed:
            print(f"{GREEN}提示: {filepath} 当前已完全符合所有物理守恒律，无需修复！{RESET}")
            return True

        # 按具体源文件归类违约项
        fixes_by_file: Dict[str, List[PSTViolation]] = {}
        for v in env.violations:
            target_f = v.source_file or env.source_file
            if target_f not in fixes_by_file:
                fixes_by_file[target_f] = []
            fixes_by_file[target_f].append(v)

        total_fixes_applied = 0

        for file_path_str, viols in fixes_by_file.items():
            f_path = Path(file_path_str)
            if not f_path.is_file():
                continue

            code_text = f_path.read_text(encoding="utf-8")
            
            # 备份原文件
            bak_path = f_path.with_suffix(f_path.suffix + ".bak")
            shutil.copyfile(f_path, bak_path)
            print(f"{CYAN}已备份原文件至: {bak_path}{RESET}")

            modified_text = code_text
            file_fixes = 0

            for v in viols:
                if v.code_evidence and v.proposed_replacement:
                    if v.code_evidence in modified_text:
                        modified_text = modified_text.replace(v.code_evidence, v.proposed_replacement, 1)
                        file_fixes += 1
                    elif v.line_range:
                        lines = modified_text.splitlines()
                        target_line_idx = v.line_range[0] - 1
                        if 0 <= target_line_idx < len(lines):
                            lines[target_line_idx] = v.proposed_replacement
                            modified_text = "\n".join(lines)
                            file_fixes += 1

            if file_fixes > 0:
                f_path.write_text(modified_text, encoding="utf-8")
                total_fixes_applied += file_fixes
                print(f"{GREEN}已成功向 [{f_path.name}] 缝合 {file_fixes} 处物理补丁！{RESET}")

        if total_fixes_applied == 0:
            print(f"{YELLOW}未能直接自动定位代码片段，请参考上述诊断手动修改。{RESET}")
            return False

        print(f"\n{CYAN}正在重新复核...{RESET}\n")
        new_env = self.inspect(filepath)
        print(self.render_dashboard(new_env))
        return new_env.passed

    def watch(self, filepath: str, poll_interval: float = 0.5) -> None:
        """实时伴侣监视模式：保存文件即毫秒级刷新仪表盘 (支持单文件与多文件工程)"""
        p = Path(filepath).resolve()
        if not p.exists():
            print(f"{RED}错误: 目标文件或工程未找到: {filepath}{RESET}", file=sys.stderr)
            return

        print(f"{CYAN}进入实时物理伴侣监视模式 · 监听目标: {filepath} (按 Ctrl+C 退出){RESET}")
        last_mtime = 0.0

        try:
            while True:
                watch_files = [p]
                if p.is_dir() or p.name == "CMakeLists.txt":
                    proj_dir = p if p.is_dir() else p.parent
                    watch_files = (
                        list(proj_dir.glob("CMakeLists.txt"))
                        + list(proj_dir.rglob("*.cc"))
                        + list(proj_dir.rglob("*.cpp"))
                        + list(proj_dir.rglob("*.hh"))
                        + list(proj_dir.rglob("*.h"))
                    )

                curr_mtime = 0.0
                for wf in watch_files:
                    try:
                        mt = os.path.getmtime(wf)
                        if mt > curr_mtime:
                            curr_mtime = mt
                    except OSError:
                        pass

                if curr_mtime != last_mtime:
                    last_mtime = curr_mtime
                    # 清屏并重新渲染
                    sys.stdout.write("\033c")
                    sys.stdout.flush()
                    try:
                        env = self.inspect(filepath)
                        print(self.render_dashboard(env))
                    except Exception as e:
                        print(f"{RED}解析异常: {e}{RESET}")

                time.sleep(poll_interval)
        except KeyboardInterrupt:
            print(f"\n{DIM}已退出监视模式。{RESET}")


def main():
    args = sys.argv[1:]
    companion = MCCompanion()

    if not args:
        # 懒人自动搜寻
        if Path("CMakeLists.txt").exists():
            env = companion.inspect(".")
            print(companion.render_dashboard(env))
            sys.exit(0 if env.passed else 1)

        cc_files = list(Path(".").glob("*.cc")) + list(Path(".").glob("*.cpp"))
        if len(cc_files) == 1:
            target = str(cc_files[0])
            env = companion.inspect(target)
            print(companion.render_dashboard(env))
            sys.exit(0 if env.passed else 1)
        else:
            print(f"{BOLD}MC-V 物理伴侣 (Monte Carlo Formal Verifier){RESET}")
            print("用法:")
            print(f"  {CYAN}mc-v .                       体检当前 CMake 蒙卡工程{RESET}")
            print(f"  {CYAN}mc-v <代码.cc / 目录>          快速体检 (自动从源码与注释提取物理意图){RESET}")
            print(f"  {CYAN}mc-v <代码.cc / 目录> \"需求说明\"{RESET}  自然语言物理需求对齐体检")
            print(f"  {CYAN}mc-v fix <代码.cc / 目录>{RESET}     一键应用推荐补丁并自动复核")
            print(f"  {CYAN}mc-v watch <代码.cc / 目录>{RESET}   实时伴侣监视模式 (保存即刷红绿灯)")
            print(f"  {CYAN}mc-v eval{RESET}                     运行 8 大基准任务自动化评测")
            sys.exit(1)

    cmd = args[0]

    if cmd == "fix" and len(args) >= 2:
        ok = companion.fix(args[1])
        sys.exit(0 if ok else 1)

    elif cmd == "watch" and len(args) >= 2:
        companion.watch(args[1])
        sys.exit(0)

    elif cmd == "eval":
        from benchmarks.eval_pst import run_evaluation, print_markdown_report
        res = run_evaluation()
        print_markdown_report(res)
        sys.exit(0)

    elif cmd == "status":
        from guardrail.switch import is_enabled, is_strict, require_spec
        print("==================================================")
        print("  MC-V 状态看板")
        print("==================================================")
        print(f"  核心开关: {'[ON 开启]' if is_enabled() else '[OFF 关闭]'}")
        print(f"  阻断模式: {'STRICT' if is_strict() else 'WARN_ONLY'}")
        print(f"  规约要求: {'REQUIRED' if require_spec() else 'OPTIONAL'}")
        print("==================================================")
        sys.exit(0)

    else:
        # 默认体检文件或工程目录
        target_path = cmd
        spec_text = args[1] if len(args) > 1 else None
        try:
            env = companion.inspect(target_path, user_spec=spec_text)
            print(companion.render_dashboard(env))
            sys.exit(0 if env.passed else 1)
        except Exception as e:
            print(f"{RED}错误: {e}{RESET}", file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()
