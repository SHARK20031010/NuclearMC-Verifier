#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build.py —— 把专家写的 Markdown 规则编译成程序读的 JSON + 人用的倒排索引

用法:
    python3 build.py            # 编译（有错则非零退出）
    python3 build.py --check    # 只校验不写文件

产出:
    compiled/rules.json     检查器读这个
    index/by-trigger.md     按触发源倒排
    index/by-stage.md       按阶段倒排
    index/by-mechanism.md   按致错机制倒排
    index/all.md            全部规则一览
"""
import json
import re
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    sys.exit("需要 PyYAML: pip install pyyaml")

ROOT = Path(__file__).resolve().parent
RULES_DIR = ROOT / "rules"
OUT_DIR = ROOT / "compiled"
INDEX_DIR = ROOT / "index"

REQUIRED = ["id", "title", "stage", "mechanism", "severity", "output",
            "detect", "applies_to", "status", "trigger", "verdict",
            "sources", "authors", "reviewed_by"]
STAGES = set(range(9))
MECHANISMS = {f"M{i}" for i in range(1, 15)}
SEVERITIES = {"high", "medium", "low"}
OUTPUTS = {"violation", "notice", "question"}
DETECTS = {"drift", "snapshot", "declaration", "code"}
CODE_KINDS = {"missing", "present", "same_line", "always"}
STATUSES = {"implemented", "needs_probe", "draft"}
DRIFT_OPS = {"RegisterPhysics", "RegisterMe", "AddDataSet"}
DRIFT_CHANGES = {"ctor_added", "model_added", "dataset_added",
                 "energy_shift", "hidden_shift"}


def parse_rule(path: Path):
    """返回 (meta dict, body str, errors list)"""
    text = path.read_text(encoding="utf-8")
    m = re.match(r"^---\s*\n(.*?)\n---\s*\n(.*)$", text, re.S)
    if not m:
        return None, "", [f"{path.name}: 找不到 front matter（--- 开头结尾）"]
    try:
        meta = yaml.safe_load(m.group(1))
    except yaml.YAMLError as e:
        return None, "", [f"{path.name}: front matter YAML 解析失败: {e}"]
    if not isinstance(meta, dict):
        return None, "", [f"{path.name}: front matter 不是键值对"]
    return meta, m.group(2), []


def validate(meta, body, fname):
    errs = []
    for k in REQUIRED:
        if k not in meta or meta[k] in (None, ""):
            errs.append(f"{fname}: 缺字段 `{k}`")
    if errs:
        return errs

    if meta["stage"] not in STAGES:
        errs.append(f"{fname}: stage 必须是 0-8，实际 {meta['stage']}")
    if meta["mechanism"] not in MECHANISMS:
        errs.append(f"{fname}: mechanism 必须是 M1-M11，实际 {meta['mechanism']}")
    if meta["severity"] not in SEVERITIES:
        errs.append(f"{fname}: severity 必须是 {SEVERITIES}")
    if meta["output"] not in OUTPUTS:
        errs.append(f"{fname}: output 必须是 {OUTPUTS}")
    if meta["detect"] not in DETECTS:
        errs.append(f"{fname}: detect 必须是 {DETECTS}")
    if meta["status"] not in STATUSES:
        errs.append(f"{fname}: status 必须是 {STATUSES}")
    if not isinstance(meta["applies_to"], list) or not meta["applies_to"]:
        errs.append(f"{fname}: applies_to 必须是非空列表")
    if not isinstance(meta["sources"], list) or not meta["sources"]:
        errs.append(f"{fname}: sources 必须是非空列表（每条规则都要有出处）")

    # 只有 expert 写死的 question 才允许 output=question
    if meta["output"] == "question" and not meta.get("question"):
        errs.append(f"{fname}: output=question 时必须写 `question` 字段")

    # trigger 结构校验
    t = meta["trigger"]
    if not isinstance(t, dict):
        errs.append(f"{fname}: trigger 必须是字典")
        return errs
    src = t.get("source")
    if src != meta["detect"]:
        errs.append(f"{fname}: trigger.source({src}) 必须等于 detect({meta['detect']})")
    if src == "drift":
        if t.get("op") and t["op"] not in DRIFT_OPS and t["op"] != "*":
            errs.append(f"{fname}: drift trigger 的 op 非法: {t['op']}")
        if t.get("change") and t["change"] not in DRIFT_CHANGES:
            errs.append(f"{fname}: drift trigger 的 change 非法: {t['change']}")
    elif src == "snapshot":
        if not t.get("subject"):
            errs.append(f"{fname}: snapshot trigger 缺 subject")
        if not t.get("key"):
            errs.append(f"{fname}: snapshot trigger 缺 key")
        modes = [x for x in ("name_matches", "value_matches", "expect", "value_is") if x in t]
        if not modes:
            errs.append(f"{fname}: snapshot trigger 需要 name_matches / value_matches / expect / value_is 之一")
        if "expect" in t and modes != ["expect"]:
            errs.append(f"{fname}: expect 不能与 name_matches/value_matches/value_is 同时用")
    elif src == "code":
        checks = t.get("checks")
        if not isinstance(checks, list) or not checks:
            errs.append(f"{fname}: code trigger 需要非空 checks 列表")
        else:
            for i, ck in enumerate(checks):
                if not isinstance(ck, dict):
                    errs.append(f"{fname}: checks[{i}] 必须是字典")
                    continue
                kind = ck.get("kind")
                if kind not in CODE_KINDS:
                    errs.append(f"{fname}: checks[{i}].kind 必须是 {CODE_KINDS}，实际 {kind}")
                    continue
                if kind == "same_line":
                    ps = ck.get("patterns")
                    if not isinstance(ps, list) or len(ps) < 2:
                        errs.append(f"{fname}: checks[{i}] same_line 需要 patterns 且至少两项")
                elif kind == "always":
                    pass
                elif not ck.get("pattern"):
                    errs.append(f"{fname}: checks[{i}] 缺 pattern")
                if not ck.get("label"):
                    errs.append(f"{fname}: checks[{i}] 缺 label（要给人看的一句话）")
        if t.get("present") is not None and not isinstance(t.get("present"), list):
            errs.append(f"{fname}: code trigger 的 present 必须是正则列表")

    if len(body.strip()) < 20:
        errs.append(f"{fname}: 正文太短，请写清「现象 / 为什么是问题」")

    return errs


def main():
    check_only = "--check" in sys.argv
    files = sorted(RULES_DIR.glob("*.md"))
    if not files:
        sys.exit(f"没有找到规则文件: {RULES_DIR}/*.md")

    all_errs, rules, seen_ids = [], [], {}
    for p in files:
        meta, body, errs = parse_rule(p)
        all_errs += errs
        if meta is None:
            continue
        errs = validate(meta, body, p.name)
        all_errs += errs
        if errs:
            continue
        rid = meta["id"]
        if rid in seen_ids:
            all_errs.append(f"{p.name}: id `{rid}` 与 {seen_ids[rid]} 重复")
            continue
        seen_ids[rid] = p.name
        rules.append({"meta": meta, "file": p.name, "body": body.strip()})

    if all_errs:
        print("❌ 校验失败：\n")
        for e in all_errs:
            print("  -", e)
        sys.exit(1)

    rules.sort(key=lambda r: r["meta"]["id"])
    print(f"✅ 校验通过：{len(rules)} 条规则")

    if check_only:
        return

    OUT_DIR.mkdir(exist_ok=True)
    INDEX_DIR.mkdir(exist_ok=True)

    payload = {
        "schema_version": 1,
        "rule_count": len(rules),
        "rules": [{"id": r["meta"]["id"], **r["meta"], "_file": r["file"]} for r in rules],
    }
    (OUT_DIR / "rules.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"   → compiled/rules.json")

    def write_index(name, title, groups):
        lines = [f"# {title}", "",
                 f"共 {len(rules)} 条规则。本文件由 `build.py` 自动生成，**不要手改**。", ""]
        for gname, items in groups:
            lines.append(f"## {gname}（{len(items)}）")
            lines.append("")
            lines.append("| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |")
            lines.append("|---|---|---|---|---|---|")
            for m in sorted(items, key=lambda x: x["id"]):
                trig = m["detect"]
                if m["detect"] == "drift":
                    t = m["trigger"]
                    trig = f"drift/{t.get('change', '*')}"
                elif m["detect"] == "snapshot":
                    trig = f"snap/{m['trigger'].get('subject')}"
                lines.append(f"| [{m['id']}](rules/{m['_file']}) | {m['title']} | "
                             f"{trig} | {m['output']} | {m['severity']} | {m['status']} |")
            lines.append("")
        (INDEX_DIR / name).write_text("\n".join(lines), encoding="utf-8")
        print(f"   → index/{name}")

    metas = [r["meta"] | {"_file": r["file"]} for r in rules]

    # 按触发源倒排
    by_detect = {}
    for m in metas:
        key = m["detect"]
        if m["detect"] == "drift":
            key = f"drift · {m['trigger'].get('change', '*')}"
        elif m["detect"] == "snapshot":
            key = f"snapshot · {m['trigger'].get('subject')}"
        else:
            key = "declaration（只能问用户）"
        by_detect.setdefault(key, []).append(m)
    write_index("by-trigger.md", "按触发源倒排", sorted(by_detect.items()))

    by_stage = {}
    for m in metas:
        by_stage.setdefault(f"阶段 {m['stage']}", []).append(m)
    write_index("by-stage.md", "按阶段倒排", sorted(by_stage.items()))

    by_mech = {}
    for m in metas:
        by_mech.setdefault(m["mechanism"], []).append(m)
    write_index("by-mechanism.md", "按致错机制倒排", sorted(by_mech.items()))

    write_index("all.md", "全部规则", [("全部", metas)])


if __name__ == "__main__":
    main()
