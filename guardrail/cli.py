#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/cli.py —— NuclearMC-Verifier Python 跨平台标准统一命令行入口
"""

import os
import sys
import argparse
from pathlib import Path

# 确保项目根目录在 sys.path
PROJECT_ROOT = Path(__file__).resolve().parents[1]
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

CONFIG_FILE = PROJECT_ROOT / ".mc-verifier.yaml"


def cmd_on() -> int:
    """开启全局核验门禁"""
    _set_enabled(True)
    print("=" * 66)
    print("  [MC-FormalVerifier] 全局开关已设置为：开启 (ON / ACTIVE)")
    print("  所有 5 大 Harness (Antigravity, OpenCode, DSH, Claude Code, Codex)")
    print("  将强制执行 5 大物理守恒不变式硬性拦截与输入规约核验！")
    print("=" * 66)
    return 0


def cmd_off() -> int:
    """关闭全局核验门禁"""
    _set_enabled(False)
    print("=" * 66)
    print("  [MC-FormalVerifier] 全局开关已设置为：关闭 (OFF / NO-OP)")
    print("  所有 5 大 Harness 立即进入 0ms 直通模式，完全不拦截、零性能开销。")
    print("=" * 66)
    return 0


def _set_enabled(val: bool):
    import re
    val_str = "true" if val else "false"
    if not CONFIG_FILE.exists():
        CONFIG_FILE.write_text(f"enabled: {val_str}\n", encoding="utf-8")
    else:
        text = CONFIG_FILE.read_text(encoding="utf-8")
        text = re.sub(r'enabled:\s*(true|false)', f'enabled: {val_str}', text, flags=re.IGNORECASE)
        CONFIG_FILE.write_text(text, encoding="utf-8")


def cmd_status() -> int:
    """查看当前启闭状态及 Harness 就绪情况"""
    from guardrail.switch import is_enabled, is_strict, require_spec
    status_str = "ON (已开启 ✅)" if is_enabled() else "OFF (已关闭 ⏸️)"
    mode_str = "STRICT (硬性阻断)" if is_strict() else "WARN_ONLY (告警提示)"
    spec_str = "REQUIRED (强制回执)" if require_spec() else "OPTIONAL (可选)"

    print("=" * 66)
    print("  针对大语言模型生成粒子输运蒙卡程序的形式化核验器 · 状态看板")
    print("=" * 66)
    print(f"  • 守恒门禁总开关: {status_str}")
    print(f"  • 物理判定模式:   {mode_str}")
    print(f"  • 前置输入规约:   {spec_str}")
    print("=" * 66)
    return 0


def cmd_check(args) -> int:
    """手动执行单文件形式化核验 (默认双核联合核验：PST + 守恒总线规则)"""
    target = args.target
    if not target:
        print("错误: 请提供需要核验的代码文件路径 (.cc/.cpp)。", file=sys.stderr)
        return 1
    
    target_path = Path(target)
    if not target_path.exists():
        print(f"错误: 文件未找到: {target}", file=sys.stderr)
        return 1

    from guardrail.check import verify_file_joint, format_joint_report
    res = verify_file_joint(
        src_path=target_path,
        pst_only=getattr(args, "pst_only", False),
        rules_only=getattr(args, "rules_only", False)
    )
    
    if getattr(args, "json", False):
        import json
        print(json.dumps(res, indent=2, ensure_ascii=False))
    else:
        print(format_joint_report(res))
    
    return 0 if res.get("is_valid", False) else 1


def cmd_check_pst(args) -> int:
    """执行物理语义树 (PST) 中间表示提取与形式化核验"""
    target = args.target
    if not target:
        print("错误: 请提供需要核验的代码文件路径 (.cc/.cpp)。", file=sys.stderr)
        return 1

    from guardrail.check import verify_file_joint, format_joint_report
    target_path = Path(target)
    if not target_path.exists():
        print(f"错误: 文件未找到: {target}", file=sys.stderr)
        return 1

    res = verify_file_joint(src_path=target_path, pst_only=True)
    if getattr(args, "json", False):
        import json
        print(json.dumps(res, indent=2, ensure_ascii=False))
    else:
        print(format_joint_report(res))
    
    return 0 if res.get("is_valid", False) else 1


def cmd_inspect(args) -> int:
    """物理实参透视看板"""
    from guardrail.companion import main as companion_main
    companion_args = ["manifest", args.target]
    if getattr(args, "json", False):
        companion_args.append("--json")
    sys.argv = ["mc-v"] + companion_args
    try:
        companion_main()
        return 0
    except SystemExit as e:
        return e.code if isinstance(e.code, int) else 0


def cmd_eval_pst(args) -> int:
    """运行 PST 全套基准评测"""
    from scripts.eval_pst import run_evaluation, print_markdown_report
    results = run_evaluation()
    print_markdown_report(results)
    return 0


def main():
    parser = argparse.ArgumentParser(
        prog="mc-verifier",
        description="形式化核验器 (NuclearMC-Verifier) 统一管理工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
常用命令示例:
  mc-verifier status           - 查看当前启闭状态及各 Harness 就绪情况
  mc-verifier on               - 开启核验（所有 Harness 强制执行守恒量门禁）
  mc-verifier off              - 关闭核验（进入严格 0ms No-Op 直通）
  mc-verifier check <文件.cc>   - 快速形式化核验 (通用守恒引擎)
  mc-verifier check-pst <文件> - 物理语义树 (PST) 深度图论核验
  mc-verifier inspect <文件>   - 物理实参透视 (几何/材料/源项/物理列表全景)
  mc-verifier eval-pst         - 运行 PST 全套物理黄金基准评测 (8 组 16 份源码)
"""
    )
    
    subparsers = parser.add_subparsers(dest="command", help="子命令")
    
    subparsers.add_parser("on", help="开启核验门禁")
    subparsers.add_parser("off", help="关闭核验门禁")
    subparsers.add_parser("status", help="查看当前启闭状态")
    
    check_p = subparsers.add_parser("check", help="手动执行单文件形式化核验 (默认双核联合：PST + 守恒总线)")
    check_p.add_argument("target", help="C++ 源码路径 (.cc/.cpp)")
    check_p.add_argument("--pst-only", action="store_true", help="仅执行物理语义树 (PST) 图论拓扑核验")
    check_p.add_argument("--rules-only", action="store_true", help="仅执行守恒总线轻量规则核验")
    check_p.add_argument("--json", action="store_true", help="输出 JSON 格式")
    
    pst_p = subparsers.add_parser("check-pst", help="执行物理语义树 (PST) 中间表示提取与形式化核验")
    pst_p.add_argument("target", help="C++ 源码路径 (.cc/.cpp)")
    
    insp_p = subparsers.add_parser("inspect", help="物理实参透视看板")
    insp_p.add_argument("target", help="源码或项目路径")
    insp_p.add_argument("--json", action="store_true", help="输出 JSON 格式")
    
    subparsers.add_parser("eval-pst", help="运行 PST 全套黄金基准评测")

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(0)

    args = parser.parse_args()
    
    if args.command == "on":
        sys.exit(cmd_on())
    elif args.command == "off":
        sys.exit(cmd_off())
    elif args.command == "status":
        sys.exit(cmd_status())
    elif args.command == "check":
        sys.exit(cmd_check(args))
    elif args.command == "check-pst":
        sys.exit(cmd_check_pst(args))
    elif args.command == "inspect":
        sys.exit(cmd_inspect(args))
    elif args.command == "eval-pst":
        sys.exit(cmd_eval_pst(args))
    else:
        parser.print_help()
        sys.exit(0)


if __name__ == "__main__":
    main()
