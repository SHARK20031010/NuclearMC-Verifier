#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
adapters/dsh/verify_helper.py —— DSH 专属高速流式形式化核验管道

与 Node.js DSH 插件通过 stdio 进行全内存 JSON 管道通信，避免任何磁盘临时文件 I/O，
执行五大正交物理守恒不变式与框架生命周期断言。
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


def main():
    try:
        raw = sys.stdin.read()
        if not raw.strip():
            sys.stdout.write(json.dumps({"passed": True, "skipped": True}) + "\n")
            return
        payload = json.loads(raw)
    except Exception as e:
        sys.stdout.write(json.dumps({"passed": True, "error": str(e)}) + "\n")
        return

    # 开关状态检查
    if not is_enabled():
        sys.stdout.write(json.dumps({"passed": True, "enabled": False}) + "\n")
        return

    code_text = payload.get("code", "")
    filename = payload.get("filename", "code.cc")
    intent_str = payload.get("intent")
    strict = payload.get("strict", is_strict())

    if not code_text.strip():
        sys.stdout.write(json.dumps({"passed": True}) + "\n")
        return

    report = ENGINE.inspect_source(
        code_text=code_text,
        filename=os.path.basename(filename),
        intent_text=intent_str,
        require_intent=require_spec(),
    )

    defects = report.get("all_defects") or report.get("defects") or []
    is_fail = (report.get("verdict") == "FAIL") or (len(defects) > 0)

    if not is_fail:
        sys.stdout.write(json.dumps({
            "passed": True,
            "defects_count": 0,
            "message": "代码符合五大正交守恒不变式契约"
        }) + "\n")
        return

    formatted = format_report(report, pure_inquiry=False)
    reason = (
        f"【DSH 蒙卡形式化核验器硬阻断】检测到 {len(defects)} 处物理守恒量违约或框架语义缺陷：\n\n"
        f"{formatted}\n\n"
        f"【修复契约】请遵循 PHYSICS_SPEC.md 修正输运物理逻辑或框架 API 用法，严禁通过硬编码数学常数欺骗数值！"
    )

    sys.stdout.write(json.dumps({
        "passed": not strict,
        "strict": strict,
        "defects_count": len(defects),
        "reason": reason,
        "defects": defects,
    }, ensure_ascii=False) + "\n")
    sys.stdout.flush()


if __name__ == "__main__":
    main()
