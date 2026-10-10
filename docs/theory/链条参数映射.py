#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
链条参数映射：把 Geant4 11.2.2 的全部可设置项，逐条塞进"原理链条"的 18 问。

输入：
  /tmp/g4cmd_clean.tsv   实测命令树（421 条，两列：目录路径 \t 命令名）
  /tmp/em_setters.txt    G4EmParameters 的 81 个 Set* 接口
  /tmp/had_setters.txt   G4HadronicParameters 的 27 个 Set* 接口

输出：
  每问各有多少条
  落不进 18 问的条目（这些才是有信息的输出）
  "程序里根本没有槽位"的问（S6/R4/R5/W1 等）单独标记

分类代码：
  S1..S6 源  T1..T6 输运  R1..R5 计分  W1 权重记账
  C      计算控制（不是物理决策：种子/事件数/打印/宏/性能）
"""
import collections, sys

QDESC = {
    "S1": "什么粒子、什么电荷态",
    "S2": "能量怎么来",
    "S3": "方向怎么来",
    "S4": "从哪儿出发",
    "S5": "什么时候出发",
    "S6": "绝对强度与归一（程序无槽位）",
    "T1": "几何",
    "T2": "材料",
    "T3": "作用类型（过程与模型）",
    "T4": "数据来源（评价库/数据文件/物理表）",
    "T5": "边界与截断",
    "T6": "偏置手法",
    "R1": "记什么量",
    "R2": "在哪记",
    "R3": "怎么加权",
    "R4": "归一到什么（程序无槽位）",
    "R5": "误差多大（程序无槽位）",
    "W1": "权重记账",
    "C":  "计算控制（非物理）",
    "?":  "落不进去",
}

# 规则表：(路径前缀, 命令名或 None=全部, 归到哪问, 备注)
RULES = [
    # ---------- S 源 ----------
    ("/gps/particle", None, "S1", ""),
    ("/gps/ion", None, "S1", ""),
    ("/gps/ionLvl", None, "S1", "亚稳态序号"),
    ("/gps/polarization", None, "S1", "极化"),
    ("/gps/energy", None, "S2", ""),
    ("/gps/ene/", None, "S2", ""),
    ("/gps/hist/", None, "S2", "直方图源（能量/位置/角度共用）"),
    ("/gps/direction", None, "S3", ""),
    ("/gps/ang/", None, "S3", ""),
    ("/gps/position", None, "S4", ""),
    ("/gps/pos/", None, "S4", ""),
    ("/gps/time", None, "S5", ""),
    ("/gps/source/intensity", None, "S6", "★相对强度，不是绝对源强"),
    ("/gps/source/", None, "S", "多源管理"),
    ("/gps/number", None, "C", "每源发几个"),
    ("/gps/List", None, "C", ""),
    ("/gps/verbose", None, "C", ""),
    ("/gps/", None, "S", "GPS 其余"),

    # ---------- T3 作用类型 ----------
    ("/process/activate", None, "T3", ""),
    ("/process/inactivate", None, "T3", ""),
    ("/process/em/setBiasingFactor", None, "T6", ""),
    ("/process/em/setDirectionalSplitting", None, "T6", ""),
    ("/process/em/setForcedInteraction", None, "T6", ""),
    ("/process/em/setSecBiasing", None, "T6", ""),
    ("/process/em/", None, "T3", "EM 参数（44 条）"),
    ("/process/eLoss/", None, "T3", "能损（20 条）"),
    ("/process/msc/", None, "T3", "多次散射（17 条）"),
    ("/process/gconv/", None, "T3", ""),
    ("/process/dna/", None, "T3", ""),
    ("/process/had/rdm/", None, "T3", "放射性衰变模块"),
    ("/process/had/decay", None, "S3", "衰变方向"),
    ("/process/had/correlatedGamma", None, "S3", "角关联（默认关）"),
    ("/process/had/", None, "T3", "强子（26 条）"),
    ("/physics_lists/em/", None, "T3", "高级电磁过程开关（15 条，默认关）"),
    ("/physics_lists/factory/", None, "T3", "给列表加构造器"),
    ("/particle/property/", None, "T3", "寿命/分支比/稳定标志"),
    ("/particle/create", None, "T3", ""),
    ("/particle/select", None, "C", ""),
    ("/particle/find", None, "C", ""),
    ("/particle/list", None, "C", ""),
    ("/particle/verbose", None, "C", ""),
    ("/particle/", None, "T3", ""),

    # ---------- T4 数据来源 ----------
    ("/process/had/rdm/setRadioactiveDecayFile", None, "T4", "★衰变数据文件"),
    ("/process/had/rdm/setPhotoEvaporationFile", None, "T4", "★光蒸发数据文件"),
    ("/process/had/rdm/setIC", None, "T4", "内转换数据"),
    ("/process/had/rdm/readICdata", None, "T4", ""),
    ("/process/had/selectTune", None, "T4", "★强子模型调参版本"),
    ("/process/had/nucleusLimits", None, "T4", "核素范围"),
    ("/run/particle/storePhysicsTable", None, "T4", "存物理表"),
    ("/run/particle/retrievePhysicsTable", None, "T4", "★复用预计算物理表"),
    ("/run/particle/setStoredInAscii", None, "T4", ""),
    ("/process/em/LivermoreData", None, "T4", ""),
    ("/process/em/fluoDirectory", None, "T4", ""),
    ("/process/em/MuDataFromFile", None, "T4", ""),
    ("/process/em/fluoANSTO", None, "T4", ""),
    ("/process/em/fluoBearden", None, "T4", ""),

    # ---------- T5 边界与截断 ----------
    ("/cuts/", None, "T5", "产生截断（5 条）"),
    ("/run/setCut", None, "T5", ""),
    ("/run/getCutForAGivenParticle", None, "T5", ""),
    ("/run/particle/applyCuts", None, "T5", "★ApplyCuts 开关"),
    ("/process/had/rdm/thresholdForVeryLongDecayTime", None, "T5", "★寿命超限直接不受理"),
    ("/process/had/rdm/decay", None, "T5", ""),
    ("/particle/nuclideTable/min_halflife", None, "T5", "★短于此视为稳定"),
    ("/particle/nuclideTable/min_meanlife", None, "T5", ""),
    ("/particle/nuclideTable/level_tolerance", None, "T5", ""),
    ("/process/had/threshold", None, "T5", ""),
    ("/process/had/maxEnergy", None, "T5", ""),
    ("/process/em/lowestElectronEnergy", None, "T5", ""),
    ("/process/em/lowestMuHadEnergy", None, "T5", ""),
    ("/process/eLoss/minKinEnergy", None, "T5", ""),
    ("/process/eLoss/maxKinEnergy", None, "T5", ""),
    ("/process/eLoss/maxKinEnergyCSDA", None, "T5", ""),
    ("/process/eLoss/CSDARange", None, "T5", ""),
    ("/process/eLoss/useCutAsFinalRange", None, "T5", ""),
    ("/process/eLoss/linLossLimit", None, "T5", ""),
    ("/process/eLoss/bremThreshold", None, "T5", ""),
    ("/process/eLoss/bremMuHadThreshold", None, "T5", ""),
    ("/process/eLoss/LambdaFactor", None, "T5", ""),
    ("/process/msc/EnergyLimit", None, "T5", ""),
    ("/process/msc/LambdaLimit", None, "T5", ""),
    ("/process/msc/ThetaLimit", None, "T5", ""),
    ("/process/msc/RangeFactor", None, "T5", ""),
    ("/process/msc/Skin", None, "T5", "步长限制参数"),
    ("/process/msc/StepLimit", None, "T5", "★步长限制类型"),
    ("/process/msc/GeomFactor", None, "T5", ""),
    ("/process/msc/SafetyFactor", None, "T5", ""),
    ("/process/msc/FactorForAngleLimit", None, "T5", ""),
    ("/process/msc/ScreeningFactor", None, "T5", ""),
    ("/process/em/binsPerDecade", None, "T5", ""),
    ("/process/em/nForFreeVector", None, "T5", ""),
    ("/process/em/max5DMuPairEnergy", None, "T5", ""),
    ("/process/em/lowestTripletEnergy", None, "T5", ""),

    # ---------- T1 几何 ----------
    ("/geometry/test/", None, "T1", "几何检查参数（分辨率/容差）"),
    ("/geometry/navigator/", None, "T1", "导航器行为"),
    ("/geometry/", None, "T1", ""),

    # ---------- T2 材料 ----------
    ("/material/nist/", None, "T2", ""),
    ("/material/g4/", None, "T2", "★密度效应开关"),
    ("/material/", None, "T2", ""),

    # ---------- R 计分 ----------
    ("/score/create/", None, "R2", "网格/探头定义"),
    ("/score/mesh/", None, "R2", ""),
    ("/score/probe/", None, "R2", ""),
    ("/score/quantity/", None, "R1", "★记什么量（22 种）"),
    ("/score/quantity/unit", None, "R1", "单位"),
    ("/score/filter/", None, "R1", "滤波器"),
    ("/score/dumpAllQuantitiesWithFactor", None, "R4", "★归一化因子"),
    ("/score/dumpQuantityWithFactor", None, "R4", ""),
    ("/score/dumpAllQuantitiesToFile", None, "R4", ""),
    ("/score/dumpQuantityToFile", None, "R4", ""),
    ("/score/", None, "R4", "计分框架其余（开关/输出）"),
    ("/score/colorMap/", None, "C", ""),

    # ---------- C 计算控制 ----------
    ("/random/", None, "C", "★可复现性（不在 18 问内）"),
    ("/control/cout/", None, "C", ""),
    ("/control/", None, "C", "宏语言与文件控制（32 条）"),
    ("/profiler/", None, "C", "性能剖析（20 条）"),
    ("/tracking/storeTrajectory", None, "R2", "存径迹"),
    ("/tracking/", None, "C", ""),
    ("/event/", None, "C", ""),
    ("/units/", None, "C", ""),
    ("/run/beamOn", None, "R5", "★事件数（决定误差）"),
    ("/run/printProgress", None, "R5", ""),
    ("/run/eventModulo", None, "C", ""),
    ("/run/numberOfThreads", None, "C", ""),
    ("/run/useMaximumLogicalCores", None, "C", ""),
    ("/run/pinAffinity", None, "C", ""),
    ("/run/workersProcessCmds", None, "C", ""),
    ("/run/", None, "C", "运行控制其余（initialize/abort/dump*）"),
    ("/process/list", None, "C", ""),
    ("/process/dump", None, "C", ""),
    ("/process/verbose", None, "C", ""),
    ("/process/setVerbose", None, "C", ""),
    ("/process/", None, "T3", "其余过程命令"),
    ("/process/em/printParameters", None, "C", ""),
    ("/process/em/verbose", None, "C", ""),
    ("/process/em/workerVerbose", None, "C", ""),
    ("/process/em/Add", None, "T3", "★按区域加过程/模型"),
]


# 兜底规则（前缀太短、什么都吃）。命中兜底的条目要单独报，
# 否则「0 条落不进去」是兜底造出来的假象。
FALLBACK = {
    "/gps/", "/process/", "/particle/", "/run/", "/score/",
    "/process/had/", "/process/em/", "/physics_lists/", "/material/",
}


def classify(path, name):
    """先按前缀长度从长到短试（最具体的规则先赢），
    同名规则优先于前缀规则。"""
    full = path + name
    cands = []
    for idx, (pref, nm, q, note) in enumerate(RULES):
        if nm is not None and nm != name:
            continue
        if full.startswith(pref) or path.startswith(pref):
            # 排序键：显式点名 > 前缀更长 > 规则表里更靠前
            cands.append(((1 if nm is not None else 0), len(pref), -idx, pref, q, note))
    if not cands:
        return "?", "", False
    cands.sort(reverse=True)
    _, _, _, pref, q, note = cands[0]
    return q, note, (pref in FALLBACK)


def main():
    rows = []
    for line in open("/tmp/g4cmd_clean.tsv", encoding="utf-8", errors="replace"):
        p = line.rstrip("\n").split("\t")
        if len(p) >= 2 and p[0].startswith("/"):
            rows.append((p[0], p[1]))

    by_q = collections.defaultdict(list)
    notes = collections.defaultdict(list)
    generic = collections.defaultdict(list)
    for path, name in rows:
        q, note, is_fb = classify(path, name)
        by_q[q].append(path + name)
        if is_fb:
            generic[q].append(path + name)
        if note:
            notes[q].append((path + name, note))

    print("=" * 78)
    print(f"命令树条目：{len(rows)}")
    print("=" * 78)
    print(f"{'问':<4}{'条数':>5}{'兜底':>6}  {'内容'}")
    print("-" * 78)
    for q in ["S1","S2","S3","S4","S5","S6","T1","T2","T3","T4","T5","T6",
              "R1","R2","R3","R4","R5","W1","C","?"]:
        n = len(by_q.get(q, []))
        if n == 0 and q not in ("?","W1","R3","S6","R4","R5"):
            continue
        fb = len(generic.get(q, []))
        print(f"{q:<4}{n:>5}{('[%d]' % fb) if fb else '':>6}  {QDESC[q]}")
    print("-" * 78)
    tot = sum(len(v) for v in by_q.values())
    phys = sum(len(by_q.get(q, [])) for q in QDESC if q not in ("C", "?"))
    print(f"{'合计':<4}{tot:>5}      其中落在物理链条（S/T/R/W）里：{phys}")
    print(f"{'':<4}{'':>5}      计算控制 C：{len(by_q.get('C', []))}，落不进去：{len(by_q.get('?', []))}")

    # ---- 空的问：程序里根本没槽位 ----
    print()
    print("=" * 78)
    print("★ 在命令树里一条都对不上的问（= 程序不提供的环节）")
    print("=" * 78)
    for q in ["S6", "R3", "R4", "R5", "W1"]:
        n = len(by_q.get(q, []))
        mark = "【空】" if n == 0 else f"{n} 条"
        print(f"  {q:<4}{QDESC[q]:<30}{mark}")
        for c in by_q.get(q, []):
            print(f"        {c}")

    # ---- 落不进去的 ----
    if by_q.get("?"):
        print()
        print("=" * 78)
        print("落不进 18 问的条目（要么漏了一问，要么跟物理无关）")
        print("=" * 78)
        for c in by_q["?"]:
            print("  " + c)

    # ---- 带 ★ 的重点 ----
    print()
    print("=" * 78)
    print("★ 标记的条目（每一条都是链条上一个具体决策点）")
    print("=" * 78)
    for q in ["S1","S2","S3","S4","S5","S6","T1","T2","T3","T4","T5","T6",
              "R1","R2","R3","R4","R5","W1","C"]:
        for c, nt in notes.get(q, []):
            if "★" in nt:
                print(f"  [{q}] {c:<60} {nt}")


if __name__ == "__main__":
    main()
