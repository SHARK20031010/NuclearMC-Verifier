#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
intent_core.py —— 上游防错总线：环 0 需求确认与代码意图联动审计总线 (Ring 0 Intent Core)
基于《环0_槽位定义_v2.md》与《环0_回执格式与插件校验_v1.md》，提供：
1. guardrail-intent 结构化回执的纯确定性机械抽取与 7 类规范校验；
2. 意图与代码实现的双向交叉审计（如 Sv 与辐射权重因数、源强绝对归一化分母、时间窗与空间尺度对齐）。
"""

import json
import re
from typing import Dict, List, Any, Optional, Tuple

BLOCK_RE = re.compile(r"```guardrail-intent\s*\n(.*?)\n\s*```", re.S)

FIELDS = ["F1a", "F1b", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10"]

ALLOWED = {
    "F1a": {"dose", "fluence", "activity", "spectrum", "efficiency",
            "microdosimetry", "other"},
    "F3":  {"Gy", "rad", "MeV/g", "Sv", "mSv", "uSv", "Bq", "Ci",
            "Bq/g", "Bq/cm3", "cm-2", "cm-2s-1", "other"},
    "F4":  {"point", "volume_avg", "surface_avg", "peak", "distribution"},
    "F5":  {"steady", "instant", "window", "pulse", "cooling"},
    "F6":  {"per_source", "per_second", "per_bq", "per_current",
            "per_power", "per_pulse"},
    "F7":  {"trend", "report", "compare"},
    "F8":  {"experiment", "analytic", "other_mc", "order_only", "none"},
    "F10": {"scalar", "curve", "map", "spectrum"},
}

F2_BY_F1A = {
    "dose":           {"absorbed", "equivalent", "effective"},
    "fluence":        {"fluence_vol", "fluence_flat", "count"},
    "activity":       {"atoms", "activity", "specific"},
    "spectrum":       {"diff", "integral"},
    "efficiency":     {"intrinsic", "absolute"},
    "microdosimetry": {"y", "z", "y_d"},
}

KNOWN_WARNINGS = {
    "no_sievert", "no_point_estimator", "no_pulse", "long_lived_nuclide",
    "no_energy_binning", "entry_not_events", "per_source_needs_strength",
}

SV_UNITS = {"Sv", "mSv", "uSv"}
F9_RE = re.compile(r"^\s*[\d.eE+-]+\s*[A-Za-z0-9/^-]+\s*$")


def extract_intent_block(text: str) -> Optional[Dict[str, Any]]:
    """从文本中抽取 guardrail-intent 块，支持 JSON 格式，若无则返回 None。"""
    if not text:
        return None
    m = BLOCK_RE.search(text)
    if m:
        try:
            return json.loads(m.group(1))
        except json.JSONDecodeError as e:
            return {"__parse_error__": str(e)}
    # 尝试直接解析纯 JSON 文本
    clean = text.strip()
    if clean.startswith("{") and clean.endswith("}"):
        try:
            obj = json.loads(clean)
            if any(f in obj for f in ("F1a", "F1_observable", "ring0_confirmation")):
                if "ring0_confirmation" in obj:
                    return obj["ring0_confirmation"]
                return obj
        except json.JSONDecodeError:
            pass
    return None


def mandatory_warnings_for_answers(answers: Dict[str, Any]) -> set:
    """按意图答案计算：哪些警告是必须发出的。"""
    need = set()
    f2 = answers.get("F2")
    f3 = answers.get("F3")
    if f2 in ("equivalent", "effective") or f3 in SV_UNITS:
        need.add("no_sievert")
    if answers.get("F4") == "point":
        need.add("no_point_estimator")
    if answers.get("F5") == "pulse":
        need.add("no_pulse")
    if answers.get("F10") == "spectrum":
        need.add("no_energy_binning")
    if answers.get("F7") in ("report", "compare"):
        need.add("entry_not_events")
    if answers.get("F6") == "per_source":
        need.add("per_source_needs_strength")
    return need


class IntentCore:
    """环 0 需求确认与代码意图联动审计总线。"""

    def __init__(self):
        pass

    def validate_intent_receipt(self, obj: Optional[Dict[str, Any]]) -> Tuple[List[str], List[str], Dict[str, Any]]:
        """纯确定性校验环 0 回执结构完整性与合法性。返回 (errors, missing_warnings, stats)。"""
        errs = []
        if obj is None:
            return (["环 0 未确认：未找到 guardrail-intent 结构化回执块。（全部 10 项按 A 缺省处理）"], [], {})
        if "__parse_error__" in obj:
            return ([f"回执块不是合法 JSON：{obj['__parse_error__']}"], [], {})

        # 校验 2：字段齐全性
        for f in FIELDS:
            if f not in obj:
                errs.append(f"环 0 回执缺字段 {f}")
        if "warnings" not in obj:
            errs.append("环 0 回执缺字段 warnings")

        # 逐项取值与合法性
        answers = {}
        for f in FIELDS:
            e = obj.get(f)
            if e is None:
                continue
            if not isinstance(e, dict) or "v" not in e or "src" not in e:
                errs.append(f"{f} 结构不符合规范，应为 {{'v': ..., 'src': 'U'|'A'}}")
                continue
            v, src = e["v"], e["src"]
            answers[f] = v

            # 校验 src
            if src not in ("U", "A"):
                errs.append(f"{f} 的决策源 src='{src}' 非法，只能是 U（用户指定）或 A（AI 代理决策）")

            # 校验枚举
            if f == "F1b":
                if not isinstance(v, str) or not v.strip():
                    errs.append("F1b（目标物理作用对象）为空，必须显式指认几何空间")
            elif f == "F9":
                if not isinstance(v, str):
                    errs.append("F9（粒子源绝对强度）必须为字符串（'N/A' 或 '数值+单位'）")
            elif f == "F2":
                f1a = obj.get("F1a", {}).get("v")
                if f1a == "other":
                    pass
                elif f1a in F2_BY_F1A:
                    if v not in F2_BY_F1A[f1a]:
                        errs.append(f"F2='{v}' 与 F1a='{f1a}' 不配套，该类别下合法选项为 {sorted(F2_BY_F1A[f1a])}")
                elif f1a:
                    errs.append(f"F1a='{f1a}' 未知，无法校验 F2 精确定义")
            elif f in ALLOWED:
                if v not in ALLOWED[f]:
                    errs.append(f"{f}='{v}' 非法，只能属于枚举范围 {sorted(ALLOWED[f])}")

        # 校验 F9 与 F6 互斥关联
        f6 = answers.get("F6")
        f9 = obj.get("F9", {}).get("v")
        if f6 is not None and f9 is not None:
            if f6 == "per_source":
                if f9 != "N/A":
                    errs.append(f"F6=per_source（相对每源粒子）时 F9 必须为 N/A，当前为 '{f9}'")
            else:
                if f9 == "N/A":
                    errs.append(f"F6={f6} 需要绝对物理源强，但 F9 填为 N/A")
                elif not F9_RE.match(str(f9)):
                    errs.append(f"F9='{f9}' 格式不合法，应为「数值+物理单位」（如 '1.0e6 Bq', '10 uA'）")

        # 校验警告齐全性
        got = obj.get("warnings")
        missing_warnings = []
        if isinstance(got, list):
            bad = [w for w in got if w not in KNOWN_WARNINGS]
            if bad:
                errs.append(f"包含未知警告 ID: {bad}")
            missing_warnings = sorted(mandatory_warnings_for_answers(answers) - set(got))
            if missing_warnings:
                errs.append(f"漏发硬性出厂物理警告: {missing_warnings}")
        elif "warnings" in obj:
            errs.append("warnings 必须为列表")

        n_u = sum(1 for f in FIELDS if obj.get(f, {}).get("src") == "U")
        n_a = sum(1 for f in FIELDS if obj.get(f, {}).get("src") == "A")
        stats = {
            "user_decisions": n_u,
            "ai_decisions": n_a,
            "ai_decision_ratio": f"{n_a}/{n_u + n_a}" if (n_u + n_a) else "n/a",
            "declared_warnings": sorted(got) if isinstance(got, list) else [],
            "answers": answers,
        }
        return (errs, missing_warnings, stats)

    def cross_audit_intent_and_code(self, intent_obj: Dict[str, Any], code_text: str) -> List[Dict[str, Any]]:
        """意图与代码联动交叉审计：检查代码实现是否忠实落地了环 0 回执中的承诺。"""
        issues = []
        if not intent_obj or "__parse_error__" in intent_obj:
            return issues

        answers = {}
        for f in FIELDS:
            if f in intent_obj and isinstance(intent_obj[f], dict):
                answers[f] = intent_obj[f].get("v")

        f1a = answers.get("F1a")
        f2 = answers.get("F2")
        f3 = answers.get("F3")
        f6 = answers.get("F6")
        f9 = answers.get("F9")
        f5 = answers.get("F5")

        # 1. 剂量当量/有效剂量 (Sv) 陷阱：Geant4 原生只有 gray，无 sievert
        if f2 in ("equivalent", "effective") or f3 in SV_UNITS:
            has_sievert_unit = bool(re.search(r"\b(?:sievert|Sv|mSv|uSv)\b", code_text))
            has_wr_weight = bool(re.search(r"(?:quality_factor|w_r|radiation_weight|ICRP|q_factor|品质因数|权重因子)", code_text, re.IGNORECASE))
            if not has_wr_weight:
                issues.append({
                    "core": "INTENT_CORE",
                    "contract": "RING0_SIEVIRT_CONTRACT",
                    "severity": "CRITICAL",
                    "defect_name": "环 0 目标为剂量当量 (Sv) 但代码未引入辐射品质因数",
                    "description": f"环 0 回执声明目标物理量为 {f2}（单位 {f3}），但代码直接将能量沉积除以质量输出。Geant4 内部无 sievert 概念且无内置 ICRP 转换系数，将 Gy 直接等同于 Sv 在混合辐射场中会导致严重物理失真。",
                    "remedy": "在输出剂量当量前，必须显式结合粒子种类或 LET 谱乘以辐射加权因子（w_r 或 Q(L)）。"
                })

        # 2. 绝对源强归一化契约 (F6=per_second / per_bq vs 代码后处理)
        if f6 in ("per_second", "per_bq", "per_current", "per_power") and f9 and f9 != "N/A":
            has_source_scaling = bool(re.search(r"(?:activity|source_strength|intensity|current|beam_flux|源强|活度)\s*[\*\/]", code_text, re.IGNORECASE))
            if not has_source_scaling:
                issues.append({
                    "core": "INTENT_CORE",
                    "contract": "RING0_NORMALIZATION_CONTRACT",
                    "severity": "HIGH",
                    "defect_name": "环 0 声明绝对时间/活度归一，但代码输出未乘绝对源强",
                    "description": f"环 0 回执声明归一基准为 {f6}（源强 {f9}），但代码仅按常规蒙卡模拟除以模拟总事件数，未在后处理中乘以实际源强/束流强度，导致输出量纲停留在无量纲每源粒子。",
                    "remedy": f"在 EndOfRunAction 或 main() 输出结果时，将单粒子均值乘以实际物理源强：{f9}。"
                })

        # 3. 脉冲时间窗契约 (F5=pulse / window)
        if f5 in ("pulse", "window"):
            has_time_cut = bool(re.search(r"(?:GetGlobalTime|time_window|t_gate|gate_width|pulse_width)\s*[\<\>\=]", code_text))
            if not has_time_cut:
                issues.append({
                    "core": "INTENT_CORE",
                    "contract": "RING0_TIME_WINDOW_CONTRACT",
                    "severity": "HIGH",
                    "defect_name": "环 0 声明瞬发脉冲/时间窗测量，但代码无时间截断门控",
                    "description": f"环 0 回执声明时间尺度为 {f5}，但计分逻辑中未检测粒子到达时间 GetGlobalTime() 或时间窗条件，会将整个衰变链长寿命延迟粒子的沉积一并计入脉冲时间窗。",
                    "remedy": "在 SteppingAction 或 SensitiveDetector 中增加全局时间门控判定：`if (preStep->GetGlobalTime() > pulse_window) return;`"
                })

        return issues

    def audit(self, code_text: str, intent_text: Optional[str] = None, require_intent: bool = False) -> Dict[str, Any]:
        """执行环 0 综合审计。"""
        issues = []
        raw_intent = intent_text if intent_text else code_text
        intent_obj = extract_intent_block(raw_intent)

        if intent_obj is None:
            if require_intent:
                issues.append({
                    "core": "INTENT_CORE",
                    "contract": "RING0_CONFIRMATION_CONTRACT",
                    "severity": "CRITICAL",
                    "defect_name": "环 0 需求确认缺位 (Specification Gap)",
                    "description": "未检测到 guardrail-intent 回执块。目标物理量 (F1)、精确定义 (F2)、归一化分母 (F6) 与源强 (F9) 完全缺失，模型在此类需求缺失下容易在下游代码中悄悄自主决策并产生静默物理偏差。",
                    "remedy": "必须在代码前输出标准的 ```guardrail-intent ... ``` 结构化确认块，明确各槽位选择与决策来源。"
                })
            return {
                "has_intent": False,
                "issues": issues,
                "stats": {},
                "intent_obj": None
            }

        errs, missing_warnings, stats = self.validate_intent_receipt(intent_obj)
        for err in errs:
            issues.append({
                "core": "INTENT_CORE",
                "contract": "RING0_SCHEMA_CONTRACT",
                "severity": "HIGH",
                "defect_name": "环 0 回执规范违背",
                "description": err,
                "remedy": "修正 guardrail-intent 块中的对应槽位取值，严格遵循固定枚举规范。"
            })

        cross_issues = self.cross_audit_intent_and_code(intent_obj, code_text)
        issues.extend(cross_issues)

        return {
            "has_intent": True,
            "issues": issues,
            "stats": stats,
            "intent_obj": intent_obj
        }
