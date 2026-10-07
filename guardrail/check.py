#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check.py —— 确定性检查器

读三样东西：
  compiled/rules.json   专家写的规则（编译产物）
  <snapshot>.json       程序运行配置快照（audit 输出）
  <records>.rec         写动作 + 漂移记录（hook.so 输出）

产出一份「需你确认的事项」清单。

注意：本程序**不做任何判断性措辞**。输出里的每句话都来自规则里的
verdict / question 字段 —— 那是专家写死的。程序只负责查出事实。

用法:
  python3 check.py --snapshot out/FTFP_BERT_HP_base.json \
                   --records  out/FTFP_BERT_HP_base.rec \
                   --task-type all
  python3 check.py ... --json        # 机器可读输出
"""
import argparse
import json
import os
import re
import sys
from fnmatch import fnmatch
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MAX_ITEMS_PER_RULE = 5
MAX_LIST = 8          # 汇总型规则最多列几个不同的值


# ─────────────────── 官方物理列表清单（构造器集合指纹） ───────────────────
# 由 mkofficial.py 从出厂基线快照生成。用「构造器集合」而不是「名字」判断，
# 理由有两条实测证据：
#   1. 程序里那个名字字符串可能是随便起的，也可能因为直接 new 类而根本没有名字；
#   2. Geant4 官方清单里本来就有「名字不同、构造器集合完全相同」的列表
#      （FTFP_BERT = FTFP_BERT_GS = FTFP_BERT_SS；Shielding = ShieldingM = ShieldingM_HP）。
_HERE = os.path.dirname(os.path.abspath(__file__))
_OFFICIAL_PATH = os.path.join(_HERE, "data", "official_lists.json")
_OFFICIAL = None


def official_sigs():
    """返回 {frozenset(构造器名), ...}；数据文件缺失时返回空集合（此时不判定）。"""
    global _OFFICIAL
    if _OFFICIAL is None:
        try:
            d = json.load(open(_OFFICIAL_PATH, encoding="utf-8"))
            _OFFICIAL = {frozenset(v) for v in d.values() if v}
        except Exception:
            _OFFICIAL = set()
    return _OFFICIAL


# ───────────────────────── 事实模型 ─────────────────────────
class Fact:
    __slots__ = ("subject", "name", "key", "value")

    def __init__(self, subject, name, key, value):
        self.subject, self.name, self.key, self.value = subject, name, key, value

    def __repr__(self):
        return f"{self.subject}:{self.name}.{self.key}={self.value!r}"


def flatten_snapshot(snap):
    """把配置快照摊平成一堆 (subject, name, key, value) 事实。"""
    facts = []
    facts.append(Fact("physics_list", "-", "value", snap.get("physics_list", "")))
    ctors = frozenset(snap.get("constructors", []))
    facts.append(Fact("physics_list", "-", "constructor_count", len(ctors)))
    sigs = official_sigs()
    # 数据缺失时不判定（给 True 让「期望为真」的规则不误报）
    facts.append(Fact("physics_list", "-", "official", (ctors in sigs) if sigs else True))
    # 相对「最接近的那个官方列表」，多装出来的构造器。
    # 为什么不用「列表交付之后再装」这个判据：RegisterPhysics 只可能发生在物理构造阶段
    # （InitializePhysics 之前），交付标记永远翻不过去，那个判据实测不可达。
    if sigs and ctors:
        nearest = max(sigs, key=lambda s: len(s & ctors))
        extra = sorted(ctors - nearest)
    else:
        extra = []
    facts.append(Fact("physics_list", "-", "extra_constructors",
                      "、".join(extra) if extra else "(无)"))

    # 派生事实：强子弹性过程装了没有？
    # 实测教训（T1-2）：模型自己拼物理列表时写
    #   RegisterPhysics(new G4EmStandardPhysics_option4());
    #   RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
    # 两行都合法、都编译通过、跑起来也不报错——但只装了「非弹性」，
    # 漏掉「弹性」。14 MeV 中子穿混凝土的透射率因此算成 0.445（正确 0.225）。
    # 判据：装了任何非弹性、却一个弹性都没有。
    #
    # ★ 踩过的坑：不能用 `"elastic" in name.lower()` 判弹性 ——
    #   "inelastic" 里就含着 "elastic" 这个子串，于是 `hInelastic QGSP_BIC_HP`
    #   同时被算成"有弹性"。实测这条规则的第一次实现因此在 T1-2 上一次都没响。
    #   必须用否定后顾排除 in 前缀。
    _ELAS = re.compile(r"(?<!in)elastic", re.I)
    has_inel = any(("inelastic" in c.lower()) for c in ctors)
    has_elas = any(_ELAS.search(c) for c in ctors)
    facts.append(Fact("physics_list", "-", "hadron_missing_elastic",
                      bool(has_inel and not has_elas)))
    for c in snap.get("constructors", []):
        facts.append(Fact("constructor", c, "present", True))

    for e in snap.get("processes", []):
        pname = f"{e['particle']}/{e['process']}"
        models = e.get("models", [])
        facts.append(Fact("process", pname, "present", True))
        facts.append(Fact("process", pname, "model_count", len(models)))
        for m in models:
            mname = f"{pname}/{m['name']}"
            emin, emax = m["emin_eV"], m["emax_MeV"]
            facts.append(Fact("model", mname, "emin_eV", emin))
            facts.append(Fact("model", mname, "emax_MeV", emax))
            # 派生事实：范围是否有效（下限必须低于上限，注意单位换算 eV vs MeV）
            facts.append(Fact("model", mname, "range_valid", emin < emax * 1e6))

        # 派生事实：多个模型按能量段排下来，中间有没有"没人管"的能量缝隙。
        # Geant4 里一个过程挂多个模型是常态，而且允许重叠（HP 就是这么设计的），
        # 所以"模型多"本身不是问题 —— 只有"缝隙"才是真问题。
        #
        # 容差：快照里 emax 存的是 MeV、emin 存的是 eV，比较时要把 emax 乘回 1e6，
        # 这个十进制往返会引入 ~1e-9 eV 量级的浮点误差。实测 Geant4 官方列表上
        # alpha/ionIoni 的 BraggIon(Emax) 与 BetheBloch(Emin) 其实首尾相接，
        # 报出来的"缝隙"只有 9.3e-10 eV —— 不加容差会把这种数值噪音当成物理缝隙，
        # 在 28/28 个干净官方列表上都误报。
        TOL_EV = 1.0  # 绝对容差 1 eV，远小于任何有物理意义的能量段缝隙
        rng = sorted((m["emin_eV"], m["emax_MeV"] * 1e6) for m in models)
        gap = any(rng[i + 1][0] > rng[i][1] + TOL_EV for i in range(len(rng) - 1))
        facts.append(Fact("process", pname, "coverage_gap", gap))

    for k, v in snap.get("em_parameters", {}).items():
        if k.startswith("_"):
            continue
        facts.append(Fact("em_parameter", k, "value", v))
    for k, v in snap.get("hadronic_parameters", {}).items():
        if k.startswith("_"):
            continue
        facts.append(Fact("hadronic_parameter", k, "value", v))
    return facts


# ───────────────────────── 记录解析 ─────────────────────────
def parse_kv(body):
    d = {}
    for part in body.split("|"):
        if "=" in part:
            k, v = part.split("=", 1)
            d[k] = v
    return d


def load_records(path):
    ops, drifts, marks = [], [], []
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        kind, _, body = line.partition("|")
        d = parse_kv(body)
        if kind == "OP":
            ops.append(d)
        elif kind == "DRIFT":
            drifts.append(d)
        elif kind == "MARK":
            marks.append(d)
    return ops, drifts, marks


OP_TO_CHANGE = {"RegisterPhysics": "ctor_added",
                "RegisterMe": "model_added",
                "AddDataSet": "dataset_added"}


# ───────────────────────── 匹配 ─────────────────────────
def deviates(value, expect):
    if isinstance(expect, dict):
        for op, rhs in expect.items():
            try:
                if op == "lt" and not (value < rhs):
                    return True
                if op == "gt" and not (value > rhs):
                    return True
                if op == "ne" and value == rhs:
                    return True
            except TypeError:
                return True
        return False
    if expect == "absent":
        return value is not True
    if expect == "present":
        return value is not True
    return value != expect


def match_snapshot(rule, facts):
    t = rule["trigger"]
    subj, key = t["subject"], t["key"]
    name = t.get("name", "*")
    hits = []
    for f in facts:
        if f.subject != subj or f.key != key:
            continue
        if not fnmatch(f.name, name):
            continue
        if "name_matches" in t and not fnmatch(f.name, t["name_matches"]):
            continue
        if "value_matches" in t and not fnmatch(str(f.value), t["value_matches"]):
            continue
        if "value_is" in t and f.value != t["value_is"]:
            continue
        if "expect" in t and not deviates(f.value, t["expect"]):
            continue
        hits.append(f)
    return hits


def match_drift(rule, ops, drifts):
    t = rule["trigger"]
    want_change = t.get("change")
    want_phase = t.get("phase")          # build = 物理列表自带 / user = 列表之外加的
    hits = []

    if want_change in (None, "ctor_added", "model_added", "dataset_added"):
        for e in ops:
            if want_change and OP_TO_CHANGE.get(e.get("op")) != want_change:
                continue
            if want_phase and e.get("phase") != want_phase:
                continue
            if not fnmatch(e.get("process", "-"), t.get("process", "*")):
                continue
            if "model" in t and not fnmatch(e.get("model", "-"), t["model"]):
                continue
            hits.append(("op", e))

    if want_change in (None, "energy_shift", "model_added", "model_removed",
                       "model_replaced", "hidden_shift"):
        for e in drifts:
            if want_change == "hidden_shift":
                if e.get("where") != "final_sweep":
                    continue
            elif want_change and e.get("change") != want_change:
                continue
            if not fnmatch(e.get("process", ""), t.get("process", "*")):
                continue
            if "model" in t and not fnmatch(e.get("model", ""), t["model"]):
                continue
            hits.append(("drift", e))
    return hits


# ───────────────────────── 代码写法匹配 ─────────────────────────
def strip_comments(src):
    """去掉 // 行注释与 /* */ 块注释。

    为什么必须去：规则里的 `missing` 判据问的是「代码里写了吗」。
    如果不去注释，一句话注释就能把「没写」伪装成「写了」——
    例如 `// 记得设 Birks 系数` 会让规则以为已经设了。
    """
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


def _line_of(src, off):
    return src.count("\n", 0, off) + 1


def _line_text(src, ln):
    ls = src.splitlines()
    return ls[ln - 1].strip() if 1 <= ln <= len(ls) else ""


def match_code(rule, src):
    """代码写法规则。返回 [(check, ctx), ...]

    trigger 结构：
      present:  [正则...]   前提。至少命中一个这条规则才适用（不写=总是适用）
      checks:
        - kind: missing     pattern: <正则>   ← 该写的没写
        - kind: present     pattern: <正则>   ← 这个写法出现了
        - kind: same_line   patterns: [..]    ← 同一行同时出现
    """
    if not src:
        return []
    t = rule["trigger"]
    bare = strip_comments(src)
    pre = t.get("present")
    if pre and not any(re.search(p, bare) for p in pre):
        return []

    hits = []
    for ck in (t.get("checks") or []):
        kind = ck.get("kind", "present")
        label = ck.get("label", "")
        if kind == "always":
            # 前提成立就报一次，没有额外条件（用于「只要出现 X 就问一句」）
            hits.append((ck, {"label": label, "line": 0, "snippet": ""}))
        elif kind == "missing":
            if not re.search(ck["pattern"], bare):
                hits.append((ck, {"label": label, "line": 0, "snippet": ""}))
        elif kind == "present":
            m = re.search(ck["pattern"], bare)
            if m:
                ln = _line_of(bare, m.start())
                hits.append((ck, {"label": label, "line": ln,
                                  "snippet": _line_text(bare, ln)[:110]}))
        elif kind == "same_line":
            for ln, line in enumerate(bare.splitlines(), 1):
                if all(re.search(p, line) for p in ck["patterns"]):
                    hits.append((ck, {"label": label, "line": ln,
                                      "snippet": line.strip()[:110]}))
                    break
    return hits


def distinct_key(kind, e):
    """汇总型规则用：一条命中拿哪个字段去重。"""
    if kind == "fact":
        return str(e.name)
    for k in ("dataset", "ctor", "model", "process"):
        if e.get(k):
            return str(e[k])
    return "-"


def fmt_energy(e, which):
    if f"before_{which}" in e:
        return f"{float(e[f'before_{which}']):.6g} {which}", \
               f"{float(e[f'after_{which}']):.6g} {which}"
    return "?", "?"


def build_ctx(kind, e):
    if kind == "op":
        return {"op": e.get("op", ""), "process": e.get("process", "-"),
                "particle": e.get("particle", "-"), "model": e.get("model", ""),
                "ctor": e.get("ctor", "")}
    if e.get("field") == "Emin":
        b, a = fmt_energy(e, "eV")
    elif e.get("field") == "Emax":
        b, a = fmt_energy(e, "MeV")
    else:
        b, a = e.get("before", "?"), e.get("after", "?")
    return {"process": e.get("process", "-"), "particle": e.get("particle", "-"),
            "model": e.get("model", ""), "before": b, "after": a,
            "where": e.get("where", "")}


def safe_format(template, ctx):
    """模板占位符缺失时不要炸 —— 缺什么就原样留着，让人能看见。"""
    out = template
    for k, v in ctx.items():
        out = out.replace("{" + k + "}", str(v))
    return out


# ───────────────────────── 主流程 ─────────────────────────
TIER_ORDER = {"violation": 0, "question": 1, "notice": 2}
TIER_LABEL = {"violation": "发现问题", "question": "必须问你", "notice": "你该知道"}


def main():
    ap = argparse.ArgumentParser(description="Antigravity Geant4 Guardrail & Inspection Engine")
    ap.add_argument("--rules", default=str(ROOT / "compiled" / "rules.json"))
    ap.add_argument("--snapshot", default=None, help="Runtime configuration snapshot JSON")
    ap.add_argument("--records", default=None, help="Runtime hook operations .rec file")
    ap.add_argument("--task-type", default="all")
    ap.add_argument("--source", "--code", dest="source", default=None,
                    help="被检查程序的源码（.cc/.cpp）。可独立提供进行通用三层代码与物理诊断。")
    ap.add_argument("--intent", default=None,
                    help="环 0 需求确认回执文件路径（含 guardrail-intent 块的 markdown 或 JSON）。")
    ap.add_argument("--require-intent", action="store_true",
                    help="强制要求提供环 0 需求确认回执，缺位直接判 FAIL (Specification Gap 拦截)。")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args()

    # 模式 1：如果未提供运行期快照，但提供了源码，自动启动五大正交守恒总线形式化核验引擎
    if a.source and (not a.snapshot or not a.records):
        try:
            from engine.universal_engine import UniversalGuardrailEngine
        except (ImportError, ValueError):
            from guardrail.engine.universal_engine import UniversalGuardrailEngine
        
        src_path = Path(a.source)
        if not src_path.exists():
            print(f"Error: source file not found: {src_path}", file=sys.stderr)
            sys.exit(1)
        code_text = src_path.read_text(encoding="utf-8")
        
        intent_text = None
        if a.intent:
            intent_p = Path(a.intent)
            if intent_p.exists():
                intent_text = intent_p.read_text(encoding="utf-8")
            else:
                intent_text = a.intent

        engine = UniversalGuardrailEngine()
        res = engine.inspect_source(code_text, filename=src_path.name,
                                    intent_text=intent_text,
                                    require_intent=a.require_intent)
        if a.json:
            print(json.dumps(res, indent=2, ensure_ascii=False))
        else:
            print(engine.format_markdown_report(res))
        return

    if not a.snapshot or not a.records:
        print("Error: 必须提供 --source 单独检查源码，或者同时提供 --snapshot 与 --records 进行运行时检查。", file=sys.stderr)
        sys.exit(1)

    rules = json.loads(Path(a.rules).read_text(encoding="utf-8"))["rules"]
    snap = json.loads(Path(a.snapshot).read_text(encoding="utf-8"))
    facts = flatten_snapshot(snap)
    ops, drifts, marks = load_records(a.records)
    src = None
    if a.source:
        try:
            src = Path(a.source).read_text(encoding="utf-8", errors="replace")
        except OSError:
            src = None

    results, skipped, no_hit = [], [], []
    for r in rules:
        if r["status"] != "implemented":
            skipped.append(r)
            continue
        if a.task_type != "all" and "all" not in r["applies_to"] \
                and a.task_type not in r["applies_to"]:
            skipped.append(r)
            continue

        if r["detect"] == "snapshot":
            hits = [("fact", f) for f in match_snapshot(r, facts)]
        elif r["detect"] == "drift":
            hits = match_drift(r, ops, drifts)
        elif r["detect"] == "code":
            hits = [("code", c[1]) for c in match_code(r, src)]
        else:
            hits = [(None, None)]

        if not hits:
            no_hit.append(r)
            continue

        if r["trigger"].get("summarize"):
            # 汇总型：一口气几百条的东西（比如"全过程挂了哪些截面数据集"）
            # 不该刷屏，压成一条：共 N 处、涉及 M 种，把名字列出来。
            seen, uniq = set(), []
            for kind, e in hits:
                v = distinct_key(kind, e)
                if v not in seen:
                    seen.add(v)
                    uniq.append(v)
            ctxs = [{"count": len(hits), "distinct": len(uniq),
                     "list": "、".join(uniq[:MAX_LIST]) + ("…" if len(uniq) > MAX_LIST else "")}]
            extra = 0
        else:
            if r["detect"] == "snapshot":
                ctxs = [{"name": f.name, "value": f.value} for _, f in hits]
            elif r["detect"] == "drift":
                ctxs = [build_ctx(k, e) for k, e in hits]
            elif r["detect"] == "code":
                ctxs = [dict(e) for _, e in hits]
            else:
                ctxs = [{}]
            extra = max(0, len(hits) - MAX_ITEMS_PER_RULE)

        items = []
        for ctx in ctxs[:MAX_ITEMS_PER_RULE]:
            body = safe_format(r["verdict"].strip(), ctx)
            item = {"text": body}
            if r["output"] == "question" and r.get("question"):
                item["ask"] = safe_format(r["question"].strip(), ctx)
            items.append(item)
        results.append({"rule": r, "items": items, "extra": extra})

    results.sort(key=lambda x: (TIER_ORDER[x["rule"]["output"]],
                                {"high": 0, "medium": 1, "low": 2}[x["rule"]["severity"]],
                                x["rule"]["id"]))

    n_fired = len(results)
    n_applicable = n_fired + len(no_hit)
    n_code_rules = sum(1 for r in rules if r["detect"] == "code")
    report = {
        "task_type": a.task_type,
        "source": a.source,
        "snapshot": {"physics_list": snap.get("physics_list"),
                     "processes": snap.get("process_count"),
                     "models": snap.get("model_count")},
        "stats": {
            "rules_total": len(rules),
            "rules_applicable": n_applicable,
            "rules_fired": n_fired,
            "rules_not_fired": len(no_hit),
            "rules_skipped": len(skipped),
            "code_rules_total": n_code_rules,
            "code_rules_judged": n_code_rules if src else 0,
            "ops_seen": len(ops),
            "drifts_seen": len(drifts),
            "list_ready": bool(marks),
        },
        "results": results,
    }

    if a.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return

    st = report["stats"]
    print("=" * 72)
    print(f"  护栏检查报告    任务类型: {a.task_type}")
    print(f"  物理列表: {report['snapshot']['physics_list']}"
          f"    过程 {report['snapshot']['processes']}    强子模型 {report['snapshot']['models']}")
    print("=" * 72)
    print()
    if not results:
        print("  ✅ 没有发现需要你确认的事项。")
        print()
    for tier in ("violation", "question", "notice"):
        group = [r for r in results if r["rule"]["output"] == tier]
        if not group:
            continue
        mark = {"violation": "❗", "question": "❓", "notice": "ℹ️ "}[tier]
        print(f"───── {mark} {TIER_LABEL[tier]}（{len(group)} 项）─────")
        print()
        for res in group:
            r = res["rule"]
            print(f"  [{r['id']}] {r['title']}")
            print(f"        严重度 {r['severity']} · 机制 {r['mechanism']} · 出处 {r['sources'][0]}")
            for it in res["items"]:
                for ln in it["text"].splitlines():
                    print(f"        {ln}")
                if "ask" in it:
                    print("        ┌─ 要问你的话（专家写死，原样输出）")
                    for ln in it["ask"].splitlines():
                        print(f"        │ {ln}")
                    print("        └─")
            if res["extra"]:
                print(f"        （另有 {res['extra']} 条同类，已省略）")
            print()
    print("─" * 72)
    print(f"  规则 {st['rules_total']} 条  |  本次适用 {st['rules_applicable']}"
          f"  |  触发 {st['rules_fired']}  |  未触发 {st['rules_not_fired']}"
          f"  |  不适用/未实现 {st['rules_skipped']}")
    print(f"  读到的动作 {st['ops_seen']} 条  |  漂移 {st['drifts_seen']} 条")
    if not marks:
        print("  ⚠ 本次记录里没有「物理列表交付完成」标记 —— PHYS-0002（列表之外额外加构造器）")
        print("    这一条本次无法判定。注意：不是「没装」，是「没看见」。")
        print("    手写物理列表（不经过 G4PhysListFactory）时就会这样。")
        print("─" * 72)
    if n_code_rules and not src:
        print(f"  ⚠ 本次没有提供程序源码（--source）—— {n_code_rules} 条「查代码写法」规则")
        print("    全部无法判定。注意：不是「没写错」，是「没看代码」。")
        print("─" * 72)


if __name__ == "__main__":
    main()
