#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/switch.py —— 跨 Harness 统一开关解析器

负责在 Antigravity, OpenCode, DSH, Claude Code, Codex 等所有 5 大宿主环境中提供
一致且高效的启停判定。
当开关处于关闭状态时，判定逻辑耗时 < 0.2ms，确保严格的 No-Op 契约。
"""

import os
import sys
from pathlib import Path
from typing import Dict, Any

_PROJECT_ROOT = Path(__file__).resolve().parents[1]
_CONFIG_FILE = _PROJECT_ROOT / ".mc-verifier.yaml"

_CACHED_CONFIG = None


def load_config() -> Dict[str, Any]:
    """读取全局配置文件；若文件不存在或读取失败，返回安全默认值。"""
    global _CACHED_CONFIG
    if _CACHED_CONFIG is not None:
        return _CACHED_CONFIG

    defaults = {
        "enabled": True,
        "strict_mode": True,
        "require_specification": True,
        "invariants": {
            "energy_conservation": True,
            "conserved_quantum_numbers": True,
            "phase_space_measure": True,
            "fair_game_weight": True,
            "dimensional_linearity": True,
            "framework_api_contracts": True,
        }
    }

    if not _CONFIG_FILE.exists():
        _CACHED_CONFIG = defaults
        return _CACHED_CONFIG

    try:
        import yaml
        with open(_CONFIG_FILE, "r", encoding="utf-8") as f:
            data = yaml.safe_load(f)
            if isinstance(data, dict):
                defaults.update(data)
    except Exception:
        pass

    _CACHED_CONFIG = defaults
    return _CACHED_CONFIG


def is_enabled() -> bool:
    """
    判断形式化核验器总开关是否开启。
    优先级：
      1. 环境变量 MC_VERIFIER_ENABLE (0 为强制关，1 为强制开)
      2. 配置文件 .mc-verifier.yaml 中的 enabled 项
    """
    env_val = os.environ.get("MC_VERIFIER_ENABLE", "").strip()
    if env_val == "0":
        return False
    if env_val == "1":
        return True

    cfg = load_config()
    return bool(cfg.get("enabled", True))


def is_strict() -> bool:
    """是否为严格阻断模式。"""
    cfg = load_config()
    return bool(cfg.get("strict_mode", True))


def require_spec() -> bool:
    """是否强制要求前置规约契约。"""
    cfg = load_config()
    return bool(cfg.get("require_specification", True))


if __name__ == "__main__":
    status = "ENABLED" if is_enabled() else "DISABLED"
    mode = "STRICT" if is_strict() else "WARN_ONLY"
    print(f"MC-FormalVerifier Status: {status} (Mode: {mode})")
