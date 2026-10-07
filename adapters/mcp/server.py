#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
adapters/mcp/server.py —— 标准 Model Context Protocol (MCP) 服务端

兼容主流 MCP 宿主：
  - OpenCode (opencode mcp add)
  - Claude Code (claude mcp add)
  - Antigravity (mcp_config.json)
  - Codex / Cursor / Zed

基于标准 stdio JSON-RPC 2.0 协议实现，零第三方网络库依赖，轻量快速启动。
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

SERVER_NAME = "mc-formal-verifier"
SERVER_VERSION = "1.0.0"
PROTOCOL_VERSION = "2024-11-05"

ENGINE = UniversalGuardrailEngine()


def get_tools():
    return [
        {
            "name": "verify_monte_carlo_code",
            "description": (
                "针对大语言模型生成的粒子输运蒙卡程序（Geant4 / OpenMC）进行形式化核验。"
                "基于第一性原理检验五大正交守恒不变式（能量动量、荷数、相空间测度、权重无偏性、量纲线性性）"
                "与框架生命周期语义契约，拦截静默物理违约与伪收敛。"
            ),
            "inputSchema": {
                "type": "object",
                "properties": {
                    "code": {
                        "type": "string",
                        "description": "待核验的 C++ / Python 粒子输运仿真程序完整源代码",
                    },
                    "intent": {
                        "type": "object",
                        "description": "可选的前置规约契约字典（包含 observable, volume, source, time_window, normalization 等）",
                    },
                    "filename": {
                        "type": "string",
                        "description": "源码文件名（如 main.cc、detector.cc），默认为 code.cc",
                        "default": "code.cc",
                    },
                },
                "required": ["code"],
            },
        },
        {
            "name": "get_verifier_status",
            "description": "查询当前形式化核验器的全局启闭开关状态与配置模式。",
            "inputSchema": {
                "type": "object",
                "properties": {},
            },
        },
    ]


def handle_tool_call(tool_name: str, arguments: dict):
    if tool_name == "get_verifier_status":
        enabled = is_enabled()
        strict = is_strict()
        return {
            "content": [
                {
                    "type": "text",
                    "text": (
                        f"MC-FormalVerifier 当前状态:\n"
                        f"- 全局开关: {'开启 (Active)' if enabled else '关闭 (No-Op)'}\n"
                        f"- 阻断模式: {'严格阻断 (Strict Block)' if strict else '仅告警 (Warn Only)'}\n"
                        f"- 规约要求: {'强制契约对齐' if require_spec() else '宽松模式'}\n"
                        f"- 支持平台: Antigravity, OpenCode, DSH, Claude Code, Codex"
                    ),
                }
            ],
            "isError": False,
        }

    if tool_name == "verify_monte_carlo_code":
        code_text = arguments.get("code", "")
        filename = arguments.get("filename", "code.cc")
        intent_dict = arguments.get("intent")
        intent_str = json.dumps(intent_dict, ensure_ascii=False) if intent_dict else None

        # 检查全局开关
        if not is_enabled():
            return {
                "content": [
                    {
                        "type": "text",
                        "text": "ℹ️ 形式化核验器当前处于全局关闭状态（No-Op 模式），代码未执行物理守恒量断言。",
                    }
                ],
                "isError": False,
            }

        # 启动通用形式化核验引擎
        report = ENGINE.inspect_source(
            code_text=code_text,
            filename=filename,
            intent_text=intent_str,
            require_intent=require_spec(),
        )

        defects = report.get("defects", [])
        if not defects:
            return {
                "content": [
                    {
                        "type": "text",
                        "text": (
                            "✅ 形式化核验通过 (VERIFICATION PASSED)\n"
                            "代码已通过五大正交物理守恒不变式检验（能量、荷数、相空间、权重、量纲）及框架语义契约，无静默物理违约。"
                        ),
                    }
                ],
                "isError": False,
            }

        # 发现缺陷
        formatted_report = format_report(report)
        err_msg = (
            f"❌ 形式化核验发现 {len(defects)} 处物理守恒量违约或框架语义缺陷：\n\n"
            f"{formatted_report}\n\n"
            f"【修复指令】请根据上述形式化违约槽位正规修复输运物理逻辑，严禁使用经验常数或人为加权进行欺骗性伪收敛！"
        )
        return {
            "content": [{"type": "text", "text": err_msg}],
            "isError": is_strict(),
        }

    raise ValueError(f"Unknown tool: {tool_name}")


def main():
    while True:
        line = sys.stdin.readline()
        if not line:
            break
        line = line.strip()
        if not line:
            continue

        try:
            req = json.loads(line)
        except Exception:
            continue

        req_id = req.get("id")
        method = req.get("method")
        params = req.get("params", {})

        if method == "initialize":
            resp = {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "protocolVersion": PROTOCOL_VERSION,
                    "capabilities": {"tools": {}, "prompts": {}},
                    "serverInfo": {
                        "name": SERVER_NAME,
                        "version": SERVER_VERSION,
                    },
                },
            }
        elif method == "notifications/initialized":
            continue
        elif method == "ping":
            resp = {"jsonrpc": "2.0", "id": req_id, "result": {}}
        elif method == "prompts/list":
            resp = {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "prompts": [
                        {
                            "name": "mc-physics-spec",
                            "description": "获取粒子输运蒙卡程序形式化规约与五大正交守恒不变式白皮书",
                        }
                    ]
                },
            }
        elif method == "prompts/get":
            spec_file = _ROOT / "PHYSICS_SPEC.md"
            spec_text = spec_file.read_text(encoding="utf-8") if spec_file.exists() else "PHYSICS_SPEC.md not found"
            resp = {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "description": "粒子输运蒙卡程序形式化物理规约",
                    "messages": [
                        {
                            "role": "user",
                            "content": {
                                "type": "text",
                                "text": spec_text,
                            },
                        }
                    ],
                },
            }
        elif method == "tools/list":
            resp = {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {"tools": get_tools()},
            }
        elif method == "tools/call":
            tool_name = params.get("name")
            args = params.get("arguments", {})
            try:
                call_res = handle_tool_call(tool_name, args)
                resp = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": call_res,
                }
            except Exception as e:
                resp = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "error": {"code": -32603, "message": str(e)},
                }
        else:
            resp = {
                "jsonrpc": "2.0",
                "id": req_id,
                "error": {"code": -32601, "message": f"Method {method} not found"},
            }

        sys.stdout.write(json.dumps(resp, ensure_ascii=False) + "\n")
        sys.stdout.flush()


if __name__ == "__main__":
    main()
