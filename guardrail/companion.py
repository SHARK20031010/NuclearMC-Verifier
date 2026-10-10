#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/companion.py —— MC-V 物理伴侣核心交互引擎 (Physics Companion Inspector)

支持：
1. 零 Flag 极简即时体检 (`mc-v <文件>`)
2. 规约对齐体检 (`mc-v <文件> "需求说明"`)
3. 一键物理修复 (`mc-v fix <文件>`)
4. 实时伴侣监视模式 (`mc-v watch <文件>`)
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
    passed: bool
    violations: List[PSTViolation] = Field(default_factory=list)
    nodes_count: int = 0
    edges_count: int = 0


class MCCompanion:
    """MC-V 物理伴侣核心引擎"""

    def __init__(self):
        self.builder = PSTBuilder()
        self.verifier = PSTVerifier()

    def extract_spec(self, code_text: str, user_spec: Optional[str] = None) -> SpecContract:
        """从用户输入或源码头部注释中提取物理规约契约"""
        if user_spec and user_spec.strip():
            desc = user_spec.strip()
            source_info = "用户指定源项"
            if re.search(r"Cs-?137|Co-?60|点源|point|gamma", desc, re.IGNORECASE):
                source_info = "各向同性放射性点源 (4pi)"
            elif re.search(r"14\s*MeV|neutron|中子|束流|beam", desc, re.IGNORECASE):
                source_info = "高能中子/带电粒子定向束流"

            return SpecContract(
                observable=desc,
                source=source_info,
                temporal_metric="实验绝对全局时钟 (GlobalTime)" if "tof" in desc.lower() else "稳态物理输运",
                normalization="每源粒子归一化 (Per Primary)",
                inferred_from="命令行自然语言输入",
            )

        # 尝试从前 25 行代码注释推断
        first_lines = "\n".join(code_text.splitlines()[:25])
        
        # 1. 探测效率与点源
        if re.search(r"Cs-?137|HPGe|全能峰|探测效率|NaI", first_lines, re.IGNORECASE):
            return SpecContract(
                observable="探测器全能峰绝对效率 (Peak Efficiency)",
                source="Cs-137 放射性点源 (4pi 球面度空间各向同性)",
                temporal_metric="事件符合衰变时间窗",
                normalization="发射总粒子数归一化 (Per Emitted Primary)",
                inferred_from="源码头部注释自动提取",
            )
        # 2. 铁水深穿透与权重
        elif re.search(r"深穿透|铁屏蔽|中子通量|方差缩减|weight|flux", first_lines, re.IGNORECASE):
            return SpecContract(
                observable="深穿透出射端中子通量 (Weighted Neutron Fluence)",
                source="14 MeV 中子定向注入束流",
                temporal_metric="稳态输运时间无关",
                normalization="权重无偏统计归一化 (Sum of Weights / N)",
                inferred_from="源码头部注释自动提取",
            )
        # 3. 脉冲剂量率
        elif re.search(r"脉冲|dose|FLASH|electron|电子束", first_lines, re.IGNORECASE):
            return SpecContract(
                observable="瞬态单脉冲峰值吸收剂量率 (Pulse Dose Rate)",
                source="高剂量率电子束单脉冲",
                temporal_metric="瞬态微观脉冲持续时间 (如 5 us)",
                normalization="靶区介质质量除法归一 (Gy = J/kg)",
                inferred_from="源码头部注释自动提取",
            )
        # 4. 飞行时间谱 TOF
        elif re.search(r"TOF|飞行时间|time.*flight", first_lines, re.IGNORECASE):
            return SpecContract(
                observable="快中子飞行时间谱 (Time of Flight Spectrum)",
                source="散裂靶脉冲中子源",
                temporal_metric="绝对实验室全局时钟 (GetGlobalTime)",
                normalization="每事件到达时间分箱计数",
                inferred_from="源码头部注释自动提取",
            )
        # 默认基准
        return SpecContract(
            observable="辐射输运物理宏观可观测响应量",
            source="定义于 PrimaryGeneratorAction 的粒子源",
            temporal_metric="连续时间推演 (ns/s)",
            normalization="基于事件总数的平均值统计",
            inferred_from="通用输运物理不变式基线",
        )

    def inspect(self, filepath: str, user_spec: Optional[str] = None) -> DiagnosticEnvelope:
        """对单个 C++ 源码执行极速物理语义分析并生成封包"""
        p = Path(filepath)
        if not p.exists():
            raise FileNotFoundError(f"文件未找到: {filepath}")

        code_text = p.read_text(encoding="utf-8", errors="replace")
        code_lines = len(code_text.splitlines())

        t0 = time.perf_counter()
        # 1. 环 0 规约抽取
        contract = self.extract_spec(code_text, user_spec)

        # 2. PST 构建与图组装
        graph = self.builder.build(code_text, filename=str(p))

        # 3. 守恒不变式形式化核验
        report = self.verifier.verify(graph)
        t1 = time.perf_counter()

        latency_ms = (t1 - t0) * 1000.0

        return DiagnosticEnvelope(
            source_file=str(p),
            code_lines=code_lines,
            latency_ms=round(latency_ms, 2),
            spec_contract=contract,
            passed=report.passed,
            violations=report.violations,
            nodes_count=len(graph.nodes),
            edges_count=len(graph.edges),
        )

    def render_dashboard(self, env: DiagnosticEnvelope) -> str:
        """渲染彩色现代终端看板 (类似 Rust/Cargo 视觉风格)"""
        lines = []
        width = 68
        border = "─" * width

        lines.append(f"{BOLD}{CYAN}╭{border}╮{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}  {BOLD}MC-V 物理伴侣 · 粒子输运形式化守恒核验{RESET}"
                     f" {DIM}(耗时: {env.latency_ms} ms){RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}  {DIM}目标文件:{RESET} {BOLD}{env.source_file}{RESET} {DIM}({env.code_lines} 行代码 · PST: {env.nodes_count} 节点, {env.edges_count} 边){RESET}".ljust(width + 17) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}├{border}┤{RESET}")

        # 1. 规约意图回执
        sc = env.spec_contract
        lines.append(f"{BOLD}{CYAN}│{RESET}  {BOLD}📋 前置物理规约对齐契约 (环 0 Spec){RESET} {DIM}[{sc.inferred_from}]{RESET}".ljust(width + 13) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 观测量:   {YELLOW}{sc.observable}{RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 辐射源:   {YELLOW}{sc.source}{RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 时间测度: {YELLOW}{sc.temporal_metric}{RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
        lines.append(f"{BOLD}{CYAN}│{RESET}    • 归一基准: {YELLOW}{sc.normalization}{RESET}".ljust(width + 8) + f"{BOLD}{CYAN}│{RESET}")
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
            lines.append(f"\n{BOLD}{GREEN}  ✨ 恭喜：物理语义树符合全部守恒不变式！代码可安全交付或提交超算。{RESET}\n")
        else:
            lines.append(f"\n{BOLD}{RED}🚨 检出 {len(env.violations)} 项物理守恒违约！请参阅以下机理诊断与处方：{RESET}\n")

            for idx, v in enumerate(env.violations, start=1):
                lines.append(f"{BOLD}┌─ [{idx}] {v.defect_name} · 规则: {v.rule_id} [{v.domain.value}]{RESET}")
                if v.line_range:
                    lines.append(f"│  {DIM}代码位置: 第 {v.line_range[0]} 行{RESET}")
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
                    lines.append(f"│  {BOLD}🔧 推荐 Diff 补丁:{RESET}")
                    for dline in v.diff.splitlines():
                        if dline.startswith("-"):
                            lines.append(f"│     {RED}{dline}{RESET}")
                        elif dline.startswith("+"):
                            lines.append(f"│     {GREEN}{dline}{RESET}")
                        else:
                            lines.append(f"│     {dline}")
                lines.append(f"{BOLD}└────────────────────────────────────────────────────────────{RESET}\n")

            lines.append(f"{DIM}👉 提示: 运行 {BOLD}mc-v fix {env.source_file}{RESET}{DIM} 即可一键自动应用上述推荐补丁{RESET}\n")

        return "\n".join(lines)

    def fix(self, filepath: str) -> bool:
        """一键修复：应用推荐补丁并自动复核"""
        p = Path(filepath)
        if not p.exists():
            print(f"{RED}错误: 文件未找到: {filepath}{RESET}", file=sys.stderr)
            return False

        env = self.inspect(filepath)
        if env.passed:
            print(f"{GREEN}提示: {filepath} 当前已完全符合所有物理守恒律，无需修复！{RESET}")
            return True

        # 读取原文件内容
        code_text = p.read_text(encoding="utf-8")
        
        # 备份原文件
        bak_path = p.with_suffix(p.suffix + ".bak")
        shutil.copyfile(p, bak_path)
        print(f"{CYAN}已备份原文件至: {bak_path}{RESET}")

        modified_text = code_text
        fixes_applied = 0

        for v in env.violations:
            if v.code_evidence and v.proposed_replacement:
                if v.code_evidence in modified_text:
                    modified_text = modified_text.replace(v.code_evidence, v.proposed_replacement, 1)
                    fixes_applied += 1
                elif v.line_range:
                    # 基于行号尝试替换
                    lines = modified_text.splitlines()
                    target_line_idx = v.line_range[0] - 1
                    if 0 <= target_line_idx < len(lines):
                        lines[target_line_idx] = v.proposed_replacement
                        modified_text = "\n".join(lines)
                        fixes_applied += 1

        if fixes_applied == 0:
            print(f"{YELLOW}未能直接自动定位代码片段，请参考上述诊断手动修改。{RESET}")
            return False

        p.write_text(modified_text, encoding="utf-8")
        print(f"{GREEN}已成功缝合 {fixes_applied} 处物理补丁！正在重新复核...{RESET}\n")

        # 重新复核
        new_env = self.inspect(filepath)
        print(self.render_dashboard(new_env))
        return new_env.passed

    def watch(self, filepath: str, poll_interval: float = 0.5) -> None:
        """实时伴侣监视模式：保存文件即毫秒级刷新仪表盘"""
        p = Path(filepath)
        if not p.exists():
            print(f"{RED}错误: 文件未找到: {filepath}{RESET}", file=sys.stderr)
            return

        print(f"{CYAN}进入实时物理伴侣监视模式 · 监听文件: {filepath} (按 Ctrl+C 退出){RESET}")
        last_mtime = 0.0

        try:
            while True:
                try:
                    curr_mtime = os.path.getmtime(filepath)
                except OSError:
                    time.sleep(poll_interval)
                    continue

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
        cc_files = list(Path(".").glob("*.cc")) + list(Path(".").glob("*.cpp"))
        if len(cc_files) == 1:
            target = str(cc_files[0])
            env = companion.inspect(target)
            print(companion.render_dashboard(env))
            sys.exit(0 if env.passed else 1)
        else:
            print(f"{BOLD}MC-V 物理伴侣 (Monte Carlo Formal Verifier){RESET}")
            print("用法:")
            print(f"  {CYAN}mc-v <代码.cc>{RESET}               快速体检 (自动从注释提取物理意图)")
            print(f"  {CYAN}mc-v <代码.cc> \"需求说明\"{RESET}    带指定需求体检")
            print(f"  {CYAN}mc-v fix <代码.cc>{RESET}           一键应用推荐补丁并自动复核")
            print(f"  {CYAN}mc-v watch <代码.cc>{RESET}         实时伴侣监视模式 (保存即刷红绿灯)")
            print(f"  {CYAN}mc-v eval{RESET}                   运行 8 大基准任务自动化评测")
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
        # 默认体检文件
        target_file = cmd
        spec_text = args[1] if len(args) > 1 else None
        try:
            env = companion.inspect(target_file, user_spec=spec_text)
            print(companion.render_dashboard(env))
            sys.exit(0 if env.passed else 1)
        except Exception as e:
            print(f"{RED}错误: {e}{RESET}", file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()
