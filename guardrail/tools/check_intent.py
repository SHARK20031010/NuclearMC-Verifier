#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
环 0 回执校验器 —— 纯确定性，不含 LLM/网络。

用法:
    python3 check_intent.py <文件>          # 校验文件里的 guardrail-intent 块
    python3 check_intent.py --selftest      # 跑内建用例

退出码: 0 = 通过; 1 = 有问题
"""

import json
import re
import sys

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
    # "other" -> 自由
}

KNOWN_WARNINGS = {
    "no_sievert", "no_point_estimator", "no_pulse", "long_lived_nuclide",
    "no_energy_binning", "entry_not_events", "per_source_needs_strength",
}

SV_UNITS = {"Sv", "mSv", "uSv"}
F9_RE = re.compile(r"^\s*[\d.eE+-]+\s*[A-Za-z0-9/^-]+\s*$")


# ---------------------------------------------------------------- 抽取

def extract(text):
    """从文本里抽出 guardrail-intent 块，返回 dict；没有则 None。"""
    m = BLOCK_RE.search(text)
    if not m:
        return None
    try:
        return json.loads(m.group(1))
    except json.JSONDecodeError as e:
        return {"__parse_error__": str(e)}


# ---------------------------------------------------------------- 校验

def mandatory_warnings(a):
    """按答案算：哪些警告是必须出现的。"""
    need = set()
    f2, f3 = a.get("F2"), a.get("F3")
    if f2 in ("equivalent", "effective") or f3 in SV_UNITS:
        need.add("no_sievert")
    if a.get("F4") == "point":
        need.add("no_point_estimator")
    if a.get("F5") == "pulse":
        need.add("no_pulse")
    if a.get("F10") == "spectrum":
        need.add("no_energy_binning")
    if a.get("F7") in ("report", "compare"):
        need.add("entry_not_events")
    if a.get("F6") == "per_source":
        need.add("per_source_needs_strength")
    return need


def validate(obj):
    """返回 (errors, missing_warnings, stats)。"""
    errs = []

    if obj is None:
        return (["环 0 未确认：没有找到 guardrail-intent 块。"
                 "（没有块 = 没问。全部 10 项按 A 处理。）"], [], {})
    if "__parse_error__" in obj:
        return ([f"回执块不是合法 JSON：{obj['__parse_error__']}"], [], {})

    # 校验 2：字段齐吗
    for f in FIELDS:
        if f not in obj:
            errs.append(f"缺字段 {f}")
    if "warnings" not in obj:
        errs.append("缺字段 warnings")

    # 逐项取值
    answers = {}
    for f in FIELDS:
        e = obj.get(f)
        if e is None:
            continue
        if not isinstance(e, dict) or "v" not in e or "src" not in e:
            errs.append(f"{f} 结构不对，应为 {{v, src}}")
            continue
        v, src = e["v"], e["src"]
        answers[f] = v

        # 校验 7：src
        if src not in ("U", "A"):
            errs.append(f"{f} 的 src='{src}' 非法，只能是 U 或 A")

        # 校验 3：取值合法
        if f == "F1b":
            if not isinstance(v, str) or not v.strip():
                errs.append("F1b（作用对象）为空")
        elif f == "F9":
            if not isinstance(v, str):
                errs.append("F9 必须是字符串")
        elif f == "F2":
            f1a = obj.get("F1a", {}).get("v")
            if f1a == "other":
                pass
            elif f1a in F2_BY_F1A:
                if v not in F2_BY_F1A[f1a]:
                    errs.append(f"F2='{v}' 与 F1a='{f1a}' 不配套，"
                                f"该量下只能是 {sorted(F2_BY_F1A[f1a])}")
            elif f1a:
                errs.append(f"F1a='{f1a}' 未知，无法校验 F2")
        elif f in ALLOWED:
            if v not in ALLOWED[f]:
                errs.append(f"{f}='{v}' 非法，只能是 {sorted(ALLOWED[f])}")

    # 校验 5：F9 条件
    f6, f9 = answers.get("F6"), obj.get("F9", {}).get("v")
    if f6 is not None and f9 is not None:
        if f6 == "per_source":
            if f9 != "N/A":
                errs.append(f"F6=per_source 时 F9 必须是 N/A，现在是 '{f9}'")
        else:
            if f9 == "N/A":
                errs.append(f"F6={f6} 需要源强，但 F9 是 N/A")
            elif not F9_RE.match(str(f9)):
                errs.append(f"F9='{f9}' 不像「数值+单位」")

    # 校验 6：警告
    got = obj.get("warnings")
    missing = []
    if isinstance(got, list):
        bad = [w for w in got if w not in KNOWN_WARNINGS]
        if bad:
            errs.append(f"未知警告 ID: {bad}")
        missing = sorted(mandatory_warnings(answers) - set(got))
        if missing:
            errs.append(f"漏发强制警告: {missing}")
    elif "warnings" in obj:
        errs.append("warnings 必须是数组")

    # 统计
    n_u = sum(1 for f in FIELDS if obj.get(f, {}).get("src") == "U")
    n_a = sum(1 for f in FIELDS if obj.get(f, {}).get("src") == "A")
    stats = {
        "用户自己定(U)": n_u,
        "AI 自主定(A)": n_a,
        "A 占比": f"{n_a}/{n_u + n_a}" if (n_u + n_a) else "n/a",
        "已发警告": sorted(got) if isinstance(got, list) else [],
    }
    return (errs, missing, stats)


def report(text, label="回执"):
    obj = extract(text)
    errs, missing, stats = validate(obj)
    print(f"=== {label} ===")
    if errs:
        for e in errs:
            print(f"  [X] {e}")
    else:
        print("  [OK] 校验通过")
    if obj is not None and "__parse_error__" not in (obj or {}):
        for k, v in stats.items():
            print(f"  {k}: {v}")
    print()
    return 0 if not errs else 1


# ---------------------------------------------------------------- 自测

GOOD = '''```guardrail-intent
{
  "F1a": {"v": "dose", "src": "U"},
  "F1b": {"v": "铅板后 1 cm 处整个面", "src": "U"},
  "F2":  {"v": "absorbed", "src": "U"},
  "F3":  {"v": "Gy", "src": "A"},
  "F4":  {"v": "surface_avg", "src": "U", "note": "板后表面"},
  "F5":  {"v": "steady", "src": "A"},
  "F6":  {"v": "per_source", "src": "U"},
  "F7":  {"v": "trend", "src": "A"},
  "F8":  {"v": "analytic", "src": "U", "note": "窄束 exp(-mu x)"},
  "F9":  {"v": "N/A", "src": "U"},
  "F10": {"v": "scalar", "src": "U"},
  "warnings": ["per_source_needs_strength"]
}
```'''

# 剂量当量 + Sv：必须同时发 no_sievert
GOOD_SV = GOOD.replace('"absorbed"', '"equivalent"') \
               .replace('"Gy"', '"Sv"') \
               .replace('["per_source_needs_strength"]',
                        '["per_source_needs_strength", "no_sievert"]')

NO_BLOCK = "我直接开始写代码了。"

MISSING_FIELD = GOOD.replace('  "F10": {"v": "scalar", "src": "U"},\n', '')

BAD_VALUE = GOOD.replace('"scalar"', '"a number"')

BAD_F2 = GOOD.replace('"absorbed"', '"intrinsic"')

BAD_F9 = GOOD.replace('"per_source"', '"per_second"')

MISS_WARN = GOOD.replace('["per_source_needs_strength"]', '[]')

BAD_JSON = '''```guardrail-intent
{"F1a": {"v": "dose" "src": "U"}}
```'''


def selftest():
    cases = [
        ("① 完整合规（吸收剂量/Gy）",        GOOD,        0),
        ("② 剂量当量+Sv 且有 no_sievert",    GOOD_SV,     0),
        ("③ 没有回执块",                     NO_BLOCK,    1),
        ("④ 缺 F10",                         MISSING_FIELD, 1),
        ("⑤ F10 取值非法",                   BAD_VALUE,   1),
        ("⑥ F2 与 F1a 不配套（dose+intrinsic）", BAD_F2,  1),
        ("⑦ F6=per_second 但 F9=N/A",        BAD_F9,      1),
        ("⑧ 漏发强制警告",                   MISS_WARN,   1),
        ("⑨ JSON 坏了",                      BAD_JSON,    1),
    ]
    fails = 0
    for label, text, want in cases:
        got = report(text, label)
        ok = (got == want)
        if not ok:
            fails += 1
            print(f"  !! 自测失败：期望退出码 {want}，实际 {got}\n")
    print("=" * 46)
    print(f"自测：{len(cases) - fails}/{len(cases)} 通过")
    return 0 if fails == 0 else 1


# ---------------------------------------------------------------- main

def main():
    if len(sys.argv) == 2 and sys.argv[1] == "--selftest":
        return selftest()
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    with open(sys.argv[1], encoding="utf-8") as f:
        return report(f.read(), sys.argv[1])


if __name__ == "__main__":
    sys.exit(main())
