#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
adapters/antigravity/hook_runner.py —— Antigravity 生命周期钩子执行器

负责在 Antigravity 智能体执行 write_to_file 或 replace_file_content 时进行实时拦截，
基于第一性原理守恒不变式与输入规约契约，进行确定性形式化核验。
"""

import json
import os
import sys
from pathlib import Path

_HERE = Path(__file__).resolve().parent
_ROOT = _HERE.parents[1]
sys.path.insert(0, str(_ROOT))

from guardrail.switch import is_enabled, is_strict, require_spec
from guardrail.engine.universal_engine import UniversalGuardrailEngine, format_report

ENGINE = UniversalGuardrailEngine()

# 仅对辐射物理 / 粒子输运相关文件与 C++ 源码执行深度核验
TARGET_EXTENSIONS = {".cc", ".cpp", ".cxx", ".hh", ".h"}
PHYSICS_KEYWORDS = ["G4", "geant4", "openmc", "ParticleGun", "GPS", "SteppingAction", "EventAction", "PhysicsList"]


def is_physics_code(code_text: str, filename: str) -> bool:
    ext = os.path.splitext(filename)[1].lower()
    if ext not in TARGET_EXTENSIONS:
        return False
    return any(kw in code_text for kw in PHYSICS_KEYWORDS)


def main():
    # 1. 开关检查：若关闭，立即 0ms 穿透放行
    if not is_enabled():
        # 输出空 JSON 合约
        sys.stdout.write("{}\n")
        sys.stdout.flush()
        return

    # 2. 读取 Antigravity 传入的上下文
    try:
        raw_input = sys.stdin.read()
        if not raw_input.strip():
            sys.stdout.write("{}\n")
            return
        payload = json.loads(raw_input)
    except Exception:
        sys.stdout.write("{}\n")
        return

    tool_call = payload.get("toolCall", {})
    tool_name = tool_call.get("name", "")
    args = tool_call.get("args", {})

    target_file = args.get("TargetFile", "")
    code_content = args.get("CodeContent") or args.get("ReplacementContent", "")

    # 若非物理模拟代码，放行
    if not is_physics_code(code_content, target_file):
        sys.stdout.write(json.dumps({"decision": "allow"}) + "\n")
        return

    # 3. 运行形式化核验引擎
    report = ENGINE.inspect_source(
        code_text=code_content,
        filename=os.path.basename(target_file),
        require_intent=require_spec(),
    )

    defects = report.get("defects", [])
    if not defects:
        # 完全合规，放行
        sys.stdout.write(json.dumps({"decision": "allow"}) + "\n")
        return

    # 4. 检出物理守恒违约
    formatted = format_report(report)
    reason = (
        f"【形式化核验器阻断】检测到 {len(defects)} 处物理守恒量违约或框架语义缺陷：\n"
        f"{formatted}\n\n"
        f"请严格按照物理守恒不变式与输入规约契约修正代码，禁止伪造常数！"
    )

    if is_strict():
        resp = {
            "decision": "deny",
            "reason": reason,
        }
    else:
        resp = {
            "decision": "allow",
            "reason": f"⚠️ 警告（非阻断）：\n{reason}",
        }

    sys.stdout.write(json.dumps(resp, ensure_ascii=False) + "\n")
    sys.stdout.flush()


if __name__ == "__main__":
    main()
