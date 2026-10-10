#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
把 24 题实测出来的 14 个错，按「原理链条的 18 问」重新归类，
再跟现有 30 条护栏规则对照，看规则押在哪、错出在哪。

输入：
  /home/shark/胡思乱想/1/测量方案/results/审读/verdicts.tsv   （24 行，3 列：题/判定/说明）
  /home/shark/胡思乱想/1/guardrail/rules/PHYS-*.md            （规则 front matter）

输出：18 问 × (错数, 规则数) 的错配表
"""
import glob, os, re

QDESC = {
    "S1": "什么粒子、什么电荷态",
    "S2": "能量怎么来",
    "S3": "方向怎么来",
    "S4": "从哪儿出发",
    "S5": "什么时候出发",
    "S6": "绝对强度与归一",
    "T1": "几何",
    "T2": "材料",
    "T3": "作用类型",
    "T4": "数据来源",
    "T5": "边界与截断",
    "T6": "偏置手法",
    "R1": "记什么量",
    "R2": "在哪记（位置与时刻）",
    "R3": "记账口径",
    "R4": "归一到什么",
    "R5": "统计与可复现",
    "W1": "权重记账",
}
QORDER = list(QDESC)

# ---- 14 个错 → 归到哪一问（每一题一个主因；括号里是判据）----
ERR = {
    "T1-2": ("T3", "作用类型：少装中子弹性散射"),
    "T1-4": ("R2", "在哪记：用前步点判几何边界，记了错的步"),
    "T3-1": ("S6", "绝对强度：少乘束流面积；活化方程多除 λ"),
    "T3-2": ("R3", "记账口径：用错 API 判断产物核来源 → 空表"),
    "T3-3": ("S2", "能量怎么来：源项漏 Na-24 的 2.754 MeV 主线"),
    "T3-4": ("S2", "能量怎么来：γ 强度表抄错（Co-60 写 0.60 应 1.00）"),
    "T4-1": ("S4", "从哪儿出发：点源写成零发散平行束"),
    "T4-2": ("S4", "从哪儿出发：点源写成零发散平行束"),
    "T4-3": ("T2", "材料：缺 Birks 猝灭系数"),
    "T4-4": ("R2", "在哪记：死层沉积也算「探测到」"),
    "T5-1": ("R1", "记什么量：线能分母用直径，应 4r/3"),
    "T6-1": ("R2", "在哪记：用 track 首步后点能量当「产生时刻」"),
    "T6-2": ("R3", "记账口径：去重集合跨事件没清空"),
    "T6-4": ("S2", "能量怎么来：Ir-192 强度表 604.4 keV 写成 82.2"),
}

# ---- 30 条规则 → 管哪一问 ----
RULE_Q = {
    "PHYS-0001": "T3", "PHYS-0002": "T3", "PHYS-0003": "T5", "PHYS-0004": "T5",
    "PHYS-0005": "T5", "PHYS-0006": "T5", "PHYS-0007": "T3", "PHYS-0008": "T4",
    "PHYS-0009": "T2", "PHYS-0010": "T3", "PHYS-0011": "T3", "PHYS-0012": "T5",
    "PHYS-0013": "T6", "PHYS-0014": "T5", "PHYS-0015": "T3", "PHYS-0016": "T3",
    "PHYS-0017": "T3", "PHYS-0018": "T3", "PHYS-0019": "R2", "PHYS-0020": "R2",
    "PHYS-0021": "R3", "PHYS-0022": "S2", "PHYS-0023": "R4", "PHYS-0024": "S4",
    "PHYS-0025": "T2", "PHYS-0026": "R2", "PHYS-0027": "R1", "PHYS-0028": "R3",
    "PHYS-0029": "R5", "PHYS-0030": "R4",
}


def load_verdicts(path):
    out = {}
    for line in open(path, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2:
            out[p[0]] = p[1]
    return out


def main():
    v = load_verdicts("/home/shark/胡思乱想/1/测量方案/results/审读/verdicts.tsv")
    wrong = [t for t, x in v.items() if x.strip() == "错"]
    right = [t for t, x in v.items() if x.strip() == "对"]
    print(f"实测：共 {len(v)} 题，错 {len(wrong)}，对 {len(right)}")
    assert len(wrong) == 14, f"错题数不是 14：{len(wrong)}"
    assert set(wrong) == set(ERR), "verdicts.tsv 的错题与 ERR 表对不上"

    err_by_q = {q: [] for q in QORDER}
    for t in wrong:
        q, why = ERR[t]
        err_by_q[q].append(t)

    rule_by_q = {q: [] for q in QORDER}
    for f in sorted(glob.glob("/home/shark/胡思乱想/1/guardrail/rules/PHYS-*.md")):
        rid = os.path.basename(f)[:-3]
        if rid in RULE_Q:
            rule_by_q[RULE_Q[rid]].append(rid)

    print()
    print("=" * 88)
    print("18 问 × 错数 × 规则数")
    print("=" * 88)
    print(f"{'问':<4}{'内容':<24}{'错的题':>7}{'规则':>6}{'规则/错':>9}   评")
    print("-" * 88)
    tot_e = tot_r = 0
    for q in QORDER:
        ne, nr = len(err_by_q[q]), len(rule_by_q[q])
        tot_e += ne
        tot_r += nr
        if ne and nr:
            ratio = f"{nr/ne:.2f}"
            judge = "✓ 对上了"
        elif ne and not nr:
            ratio = "0"
            judge = "★ 有错没规则"
        elif nr and not ne:
            ratio = "∞"
            judge = "☆ 空转"
        else:
            ratio = "—"
            judge = "都没碰"
        flag = "★" if (ne and (not nr or nr / ne < 0.5)) else ""
        print(f"{q:<4}{QDESC[q]:<24}{ne:>7}{nr:>6}{ratio:>9}   {judge}{flag}")
    print("-" * 88)
    print(f"{'合计':<4}{'':<24}{tot_e:>7}{tot_r:>6}")

    # ---- 按三个环节汇总 ----
    print()
    print("=" * 88)
    print("按三个环节汇总")
    print("=" * 88)
    print(f"{'环节':<8}{'错数':>6}{'占比':>9}{'规则数':>8}{'占比':>9}")
    print("-" * 88)
    for name, qs in [("S 源", ["S1","S2","S3","S4","S5","S6"]),
                     ("T 输运", ["T1","T2","T3","T4","T5","T6"]),
                     ("R 计分", ["R1","R2","R3","R4","R5"]),
                     ("W 权重", ["W1"])]:
        ne = sum(len(err_by_q[q]) for q in qs)
        nr = sum(len(rule_by_q[q]) for q in qs)
        print(f"{name:<8}{ne:>6}{ne/tot_e*100:>8.0f}%{nr:>8}{nr/tot_r*100:>8.0f}%")

    # ---- 分批对照 ----
    print()
    print("=" * 88)
    print("两批规则各自的落点")
    print("=" * 88)
    for label, ids in [("第一批 0001-0017（照参数总表写的）",
                        [f"PHYS-{i:04d}" for i in range(1, 18)]),
                       ("第二批 0018-0030（照实测的错写的）",
                        [f"PHYS-{i:04d}" for i in range(18, 31)])]:
        c = {"S": 0, "T": 0, "R": 0, "W": 0}
        for rid in ids:
            q = RULE_Q.get(rid)
            if q:
                c[q[0]] += 1
        print(f"  {label}")
        print(f"     S 源 {c['S']} 条 / T 输运 {c['T']} 条 / R 计分 {c['R']} 条 "
              f"（共 {sum(c.values())} 条）")
    print(f"  而错误分布：S 源 {sum(len(err_by_q[q]) for q in ['S1','S2','S3','S4','S5','S6'])} / "
          f"T 输运 {sum(len(err_by_q[q]) for q in ['T1','T2','T3','T4','T5','T6'])} / "
          f"R 计分 {sum(len(err_by_q[q]) for q in ['R1','R2','R3','R4','R5'])}")

    # ---- 逐条列出 ----
    print()
    print("=" * 88)
    print("逐问明细")
    print("=" * 88)
    for q in QORDER:
        if not err_by_q[q] and not rule_by_q[q]:
            continue
        print(f"\n[{q}] {QDESC[q]}")
        if err_by_q[q]:
            for t in err_by_q[q]:
                print(f"    错  {t:<6} {ERR[t][1]}")
        if rule_by_q[q]:
            print(f"    规则 {' '.join(rule_by_q[q])}")


if __name__ == "__main__":
    main()
