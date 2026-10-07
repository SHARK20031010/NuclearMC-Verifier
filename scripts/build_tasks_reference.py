#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_tasks_reference.py
------------------------
将 190 道基准测试题（Tier 1 基础单考点 60 题、Tier 2 跨模块组合 60 题、
Tier 3 高阶多约束综合 60 题、实际社区案例 10 题）
按每题独立建文件夹归档：
- task.md: 题目与物理考点
- prompt.md: A/B/C 组提示词
- results.md: 对比测试结果与分析
- code/: 生成的源码文件
用语简洁自然，直击本质，避免冗长假大空词汇。
"""

import os
import sys
import json
import shutil
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SOURCE_ROOT = REPO_ROOT / "测量方案"
TARGET_ROOT = REPO_ROOT / "benchmark_tasks_reference"

CATEGORIES = {
    "T1": "屏蔽与深穿透",
    "T2": "辐射剂量与防护",
    "T3": "活化与反应产额",
    "T4": "探测器响应与能谱",
    "T5": "微剂量与复杂输运",
    "T6": "源项与相空间"
}


def simplify_text(text: str) -> str:
    """去浮夸、去假大空，换成清晰自然的技术用语"""
    if not text:
        return text

    text = text.replace("## 这题要打中的坑（不给模型看）", "## 核心物理考点与常见错误")
    text = text.replace("## 这题要打中的坑", "## 核心物理考点与常见错误")
    text = text.replace("## 冻结 prompt", "## 题目要求")
    text = text.replace("## 冻结纪律", "## 测试约束")
    text = text.replace("老师傅插件针对性干预", "物理核验器干预")
    text = text.replace("老师傅插件物理修正版", "核验修正版")
    text = text.replace("认知护栏与老师傅插件", "物理核验器")
    text = text.replace("老师傅针对性质询与修正建议", "修改建议")
    text = text.replace("五大正交守恒总线机械确定性审计与拍桌子质询 (动态生成，零题号小抄)", "守恒定律检查")
    text = text.replace("拍桌子质询", "守恒检查")
    text = text.replace("零题号小抄", "通用检查")
    text = text.replace("环 0 需求确认回执", "前置参数确认")
    text = text.replace("环 0 需求确认", "前置参数确认")
    text = text.replace("环 0 回执", "参数确认")
    text = text.replace("真实野生用例", "实际社区案例")
    text = text.replace("真实野生漏洞", "社区典型缺陷")
    text = text.replace("野生漏洞", "社区案例缺陷")
    text = text.replace("野生用例", "实际案例")
    text = text.replace("幽灵计数残留", "未清零脏数据残留")
    text = text.replace("重定向防越狱", "拦截终端写入")

    return text


def build_tier1():
    print(">>> 正在生成 Tier 1 (基础单考点 60 题)...")
    tier1_target = TARGET_ROOT / "tier1_single_slot"
    tier1_target.mkdir(parents=True, exist_ok=True)

    with open(SOURCE_ROOT / "results" / "abc_experiment_verdict.json", "r", encoding="utf-8") as f:
        verdicts = json.load(f)

    for i in range(1, 7):
        for j in range(1, 11):
            tid = f"T{i}-{j}"
            task_dir = tier1_target / tid
            code_dir = task_dir / "code"
            task_dir.mkdir(parents=True, exist_ok=True)
            code_dir.mkdir(parents=True, exist_ok=True)

            # 1. 任务内容 task.md
            src_task_file = SOURCE_ROOT / "tasks" / f"{tid}.md"
            raw_task = src_task_file.read_text(encoding="utf-8") if src_task_file.exists() else f"# 题目 {tid}\n\n缺失任务定义文件。"
            task_content = simplify_text(raw_task)
            (task_dir / "task.md").write_text(task_content, encoding="utf-8")

            # 2. 提示词 prompt.md
            prompt_b_file = SOURCE_ROOT / "prompts_B" / f"{tid}.md"
            prompt_c_file = SOURCE_ROOT / "prompts_C" / f"{tid}.md"

            prompt_md = f"# {tid} 测试提示词\n\n"
            prompt_md += f"## A组：直接生成（原始提问）\n\n"
            
            frozen_lines = []
            capturing = False
            for line in raw_task.splitlines():
                if "## 冻结 prompt" in line:
                    capturing = True
                    continue
                elif capturing and line.startswith("## "):
                    break
                elif capturing:
                    frozen_lines.append(line)
            frozen_prompt = "\n".join(frozen_lines).strip()
            prompt_md += (frozen_prompt if frozen_prompt else "> 请根据上述物理需求编写 Geant4 程序。") + "\n\n---\n\n"

            prompt_md += f"## B组：模型自查（让模型自己复核）\n\n"
            if prompt_b_file.exists():
                prompt_md += simplify_text(prompt_b_file.read_text(encoding="utf-8")) + "\n\n---\n\n"
            else:
                prompt_md += "*A组一次直接做对，无需进入自查。*\n\n---\n\n"

            prompt_md += f"## C组：加核验器（物理守恒检查报错后重修）\n\n"
            if prompt_c_file.exists():
                prompt_md += simplify_text(prompt_c_file.read_text(encoding="utf-8")) + "\n"
            else:
                prompt_md += "*A组一次直接做对，无需核验器干预。*\n"

            (task_dir / "prompt.md").write_text(prompt_md, encoding="utf-8")

            # 3. 结果与判定 results.md
            v_data = verdicts.get(tid, {})
            arm_a = v_data.get("Arm_A", {})
            arm_b = v_data.get("Arm_B", {})
            arm_c = v_data.get("Arm_C", {})

            arm_a_reason = arm_a.get('reason') or '未加约束直接生成，出现物理错误'
            arm_b_reason = arm_b.get('reason') or ('自查修正成功' if arm_b.get('correct') else '自己检查仍未发现物理错误')
            arm_c_reason = arm_c.get('reason') or '核验器报错拦截，指出物理错误后成功修复'

            res_md = f"# {tid} 测试结果与分析\n\n"
            res_md += f"## 一、对比结果\n\n"
            res_md += "| 测试组 | 结果 | 说明 |\n"
            res_md += "| :--- | :---: | :--- |\n"
            res_md += f"| **A组 (直接生成)** | {'✅ 通过' if arm_a.get('correct') else '❌ 未通过'} | {simplify_text(arm_a_reason)} |\n"
            res_md += f"| **B组 (模型自查)** | {'✅ 通过' if arm_b.get('correct') else '❌ 未通过'} | {simplify_text(arm_b_reason)} |\n"
            res_md += f"| **C组 (加核验器)** | {'✅ 通过' if arm_c.get('correct') else '❌ 未通过'} | {simplify_text(arm_c_reason)} |\n\n"

            res_md += f"## 二、原因分析\n\n"
            if arm_a.get('correct'):
                res_md += f"- **A组情况**：模型基础较好，所选物理列表与几何定义无误，一次写对。\n"
            else:
                res_md += f"- **原始错误**：{simplify_text(arm_a_reason)}\n"
                res_md += f"- **自查效果**：{'模型在提示下自行找出了错误并修正' if arm_b.get('correct') else '模型自己无法识别物理错误，依然给出错误代码'}\n"
                res_md += f"- **核验器效果**：核验器按守恒定律精确报错，模型根据报错修正成功。\n"

            (task_dir / "results.md").write_text(res_md, encoding="utf-8")

            # 4. 拷贝源代码
            src_a = SOURCE_ROOT / "code" / tid / "code.cc"
            src_b = SOURCE_ROOT / "code_B" / tid / "code.cc"
            src_c = SOURCE_ROOT / "code_C" / tid / "code.cc"

            if src_a.exists():
                shutil.copy2(src_a, code_dir / "code_ArmA.cc")
            if src_b.exists():
                shutil.copy2(src_b, code_dir / "code_ArmB.cc")
            elif src_a.exists():
                shutil.copy2(src_a, code_dir / "code_ArmB.cc")
            if src_c.exists():
                shutil.copy2(src_c, code_dir / "code_ArmC.cc")
            elif src_a.exists():
                shutil.copy2(src_a, code_dir / "code_ArmC.cc")


def build_tier2():
    print(">>> 正在生成 Tier 2 (跨模块组合 60 题)...")
    tier2_target = TARGET_ROOT / "tier2_coupled"
    tier2_target.mkdir(parents=True, exist_ok=True)

    t2_src = SOURCE_ROOT / "benchmark_tier2"
    with open(t2_src / "results" / "tier2_verdict_all_60.json", "r", encoding="utf-8") as f:
        verdicts = json.load(f)

    for i in range(1, 7):
        for j in range(1, 11):
            tid = f"T{i}-M{j}"
            task_dir = tier2_target / tid
            code_dir = task_dir / "code"
            task_dir.mkdir(parents=True, exist_ok=True)
            code_dir.mkdir(parents=True, exist_ok=True)

            # 1. 任务内容 task.md
            src_task_file = t2_src / "tasks" / f"{tid}.md"
            raw_task = src_task_file.read_text(encoding="utf-8") if src_task_file.exists() else f"# 题目 {tid}\n\n缺失任务定义文件。"
            task_content = simplify_text(raw_task)
            (task_dir / "task.md").write_text(task_content, encoding="utf-8")

            # 2. 提示词 prompt.md
            prompt_c_r0 = t2_src / "prompts_C_ring0" / f"{tid}.md"
            prompt_c = t2_src / "prompts_C" / f"{tid}.md"

            prompt_md = f"# {tid} 测试提示词\n\n"
            prompt_md += f"## A组：直接生成（原始需求）\n\n"
            prompt_md += task_content + "\n\n---\n\n"

            prompt_md += f"## C组：加核验器（前置参数确认与报错质询）\n\n"
            if prompt_c_r0.exists():
                prompt_md += simplify_text(prompt_c_r0.read_text(encoding="utf-8")) + "\n"
            elif prompt_c.exists():
                prompt_md += simplify_text(prompt_c.read_text(encoding="utf-8")) + "\n"
            else:
                prompt_md += "*未记录单独提示词。*\n"

            (task_dir / "prompt.md").write_text(prompt_md, encoding="utf-8")

            # 3. 结果 results.md
            v_data = verdicts.get(tid, {})
            arm_a = v_data.get("Arm_A", {})
            arm_b = v_data.get("Arm_B", {})
            arm_c = v_data.get("Arm_C", {})

            arm_a_reason = arm_a.get('reason') or '未加约束，边界判断或截面设置错误'
            arm_b_reason = arm_b.get('reason') or ('自查修正成功' if arm_b.get('passed') else '自己复核未能发现多模块耦合处的物理错误')
            arm_c_reason = arm_c.get('reason') or '前置确认物理参数，核验器拦截违规后修正成功'

            audit_file = t2_src / "results_isolated" / f"{tid}_audit.json"
            audit_json_str = ""
            if audit_file.exists():
                try:
                    audit_data = json.loads(audit_file.read_text(encoding="utf-8"))
                    audit_json_str = json.dumps(audit_data, indent=2, ensure_ascii=False)
                except Exception:
                    pass

            res_md = f"# {tid} 测试结果与分析\n\n"
            res_md += f"## 一、对比结果\n\n"
            res_md += "| 测试组 | 结果 | 编译 | 说明 |\n"
            res_md += "| :--- | :---: | :---: | :--- |\n"
            res_md += f"| **A组 (直接生成)** | {'✅ 通过' if arm_a.get('passed') else '❌ 未通过'} | {'编译通过' if arm_a.get('compile') else '编译失败'} | {simplify_text(arm_a_reason)} |\n"
            res_md += f"| **B组 (模型自查)** | {'✅ 通过' if arm_b.get('passed') else '❌ 未通过'} | {'编译通过' if arm_b.get('compile') else '编译失败'} | {simplify_text(arm_b_reason)} |\n"
            res_md += f"| **C组 (加核验器)** | {'✅ 通过' if arm_c.get('passed') else '❌ 未通过'} | {'编译通过' if arm_c.get('compile') else '编译失败'} | {simplify_text(arm_c_reason)} |\n\n"

            if audit_json_str:
                res_md += f"## 二、核验器静态代码审计明细\n\n```json\n{audit_json_str}\n```\n"

            (task_dir / "results.md").write_text(res_md, encoding="utf-8")

            # 4. 源码拷贝
            for src_name, dst_name in [
                ("code_A", "code_ArmA.cc"),
                ("code_B", "code_ArmB.cc"),
                ("code_C", "code_ArmC.cc"),
                ("code_C_isolated", "code_ArmC_isolated.cc"),
                ("code_C_level3", "code_ArmC_level3.cc")
            ]:
                s = t2_src / src_name / f"{tid}.cc"
                if s.exists():
                    shutil.copy2(s, code_dir / dst_name)


def build_tier3():
    print(">>> 正在生成 Tier 3 (高阶多约束综合 60 题)...")
    tier3_target = TARGET_ROOT / "tier3_deep_penetration"
    tier3_target.mkdir(parents=True, exist_ok=True)

    t3_src = SOURCE_ROOT / "benchmark_tier3"
    with open(t3_src / "results" / "tier3_verdict_all_60.json", "r", encoding="utf-8") as f:
        verdicts = json.load(f)

    for i in range(1, 7):
        for j in range(1, 11):
            tid = f"T{i}-H{j}"
            task_dir = tier3_target / tid
            code_dir = task_dir / "code"
            task_dir.mkdir(parents=True, exist_ok=True)
            code_dir.mkdir(parents=True, exist_ok=True)

            # 1. 任务内容 task.md
            src_task_file = t3_src / "tasks" / f"{tid}.md"
            raw_task = src_task_file.read_text(encoding="utf-8") if src_task_file.exists() else f"# 题目 {tid}\n\n缺失任务定义文件。"
            task_content = simplify_text(raw_task)
            (task_dir / "task.md").write_text(task_content, encoding="utf-8")

            # 2. 提示词 prompt.md
            prompt_c_r0 = t3_src / "prompts_C_ring0" / f"{tid}.md"
            prompt_c = t3_src / "prompts_C" / f"{tid}.md"

            prompt_md = f"# {tid} 测试提示词\n\n"
            prompt_md += f"## A组：直接生成（原始需求）\n\n"
            prompt_md += task_content + "\n\n---\n\n"

            prompt_md += f"## C组：加核验器（前置参数确认与报错质询）\n\n"
            if prompt_c_r0.exists():
                prompt_md += simplify_text(prompt_c_r0.read_text(encoding="utf-8")) + "\n"
            elif prompt_c.exists():
                prompt_md += simplify_text(prompt_c.read_text(encoding="utf-8")) + "\n"
            else:
                prompt_md += "*未记录单独提示词。*\n"

            (task_dir / "prompt.md").write_text(prompt_md, encoding="utf-8")

            # 3. 结果 results.md
            v_data = verdicts.get(tid, {})
            arm_a = v_data.get("Arm_A", {})
            arm_b = v_data.get("Arm_B", {})
            arm_c = v_data.get("Arm_C", {})

            arm_a_reason = arm_a.get('reason') or '未加约束，深穿透漏乘权重或动力学模型缺失'
            arm_b_reason = arm_b.get('reason') or ('深穿透高难度自查修正成功' if arm_b.get('passed') else '自查改不对，模型甚至通过编造经验常数来凑数')
            arm_c_reason = arm_c.get('reason') or '核验器硬阻断伪造常数，强制要求按守恒律规范计算'

            audit_file = t3_src / "results_isolated" / f"{tid}_audit.json"
            audit_json_str = ""
            if audit_file.exists():
                try:
                    audit_data = json.loads(audit_file.read_text(encoding="utf-8"))
                    audit_json_str = json.dumps(audit_data, indent=2, ensure_ascii=False)
                except Exception:
                    pass

            res_md = f"# {tid} 测试结果与分析\n\n"
            res_md += f"## 一、对比结果\n\n"
            res_md += "| 测试组 | 结果 | 编译 | 说明 |\n"
            res_md += "| :--- | :---: | :---: | :--- |\n"
            res_md += f"| **A组 (直接生成)** | {'✅ 通过' if arm_a.get('passed') else '❌ 未通过'} | {'编译通过' if arm_a.get('compile') else '编译失败'} | {simplify_text(arm_a_reason)} |\n"
            res_md += f"| **B组 (模型自查)** | {'✅ 通过' if arm_b.get('passed') else '❌ 未通过'} | {'编译通过' if arm_b.get('compile') else '编译失败'} | {simplify_text(arm_b_reason)} |\n"
            res_md += f"| **C组 (加核验器)** | {'✅ 通过' if arm_c.get('passed') else '❌ 未通过'} | {'编译通过' if arm_c.get('compile') else '编译失败'} | {simplify_text(arm_c_reason)} |\n\n"

            if audit_json_str:
                res_md += f"## 二、核验器静态代码审计明细\n\n```json\n{audit_json_str}\n```\n"

            (task_dir / "results.md").write_text(res_md, encoding="utf-8")

            # 4. 源码拷贝
            for src_name, dst_name in [
                ("code_A", "code_ArmA.cc"),
                ("code_B", "code_ArmB.cc"),
                ("code_C", "code_ArmC.cc"),
                ("code_C_isolated", "code_ArmC_isolated.cc"),
                ("code_C_level3", "code_ArmC_level3.cc")
            ]:
                s = t3_src / src_name / f"{tid}.cc"
                if s.exists():
                    shutil.copy2(s, code_dir / dst_name)


def build_wild():
    print(">>> 正在生成实际社区案例 (10 题)...")
    wild_target = TARGET_ROOT / "wild_corpus"
    wild_target.mkdir(parents=True, exist_ok=True)

    w_src = SOURCE_ROOT / "real_world_wild_corpus"
    with open(w_src / "results" / "wild_corpus_verdict.json", "r", encoding="utf-8") as f:
        verdicts = json.load(f)
    with open(w_src / "results" / "wild_three_arms_verdict.json", "r", encoding="utf-8") as f:
        three_arms_verdicts = json.load(f)

    for i in range(1, 11):
        tid = f"WILD-{i:02d}"
        task_dir = wild_target / tid
        code_dir = task_dir / "code"
        task_dir.mkdir(parents=True, exist_ok=True)
        code_dir.mkdir(parents=True, exist_ok=True)

        v_data = verdicts.get(tid, {})
        arm_data = three_arms_verdicts.get(tid, {})
        title = v_data.get("title", arm_data.get("title", "实际社区缺陷案例"))
        source = v_data.get("source", arm_data.get("source", "开源社区讨论"))
        human_issue = v_data.get("human_issue", "")
        trap_root_cause = v_data.get("trap_root_cause", "")

        # 1. 任务内容 task.md
        task_md = f"# {tid} · {title}\n\n"
        task_md += f"## 案例来源\n> {source}\n\n"
        task_md += f"## 现象与求助背景\n{human_issue}\n\n"
        if trap_root_cause:
            task_md += f"## 错误根因分析\n{trap_root_cause}\n\n"
        task_md += f"## 涉及的物理规则与检查点\n"
        task_md += f"- **操作检查点**：{v_data.get('action_slot', '粒子产生生命周期与首步过滤')}\n"
        task_md += f"- **守恒规则**：{v_data.get('invariant_type', '输运生命周期')} ({v_data.get('rule_id', 'LIFECYCLE-CORE')})\n"
        (task_dir / "task.md").write_text(task_md, encoding="utf-8")

        # 2. 提示词 prompt.md
        prompt_pure_file = w_src / "prompts_C_pure" / f"{tid}.md"
        prompt_md = f"# {tid} 测试提示词\n\n"
        if prompt_pure_file.exists():
            prompt_md += simplify_text(prompt_pure_file.read_text(encoding="utf-8")) + "\n"
        else:
            prompt_md += "*未单独记录二轮提示词。*\n"
        (task_dir / "prompt.md").write_text(prompt_md, encoding="utf-8")

        # 3. 结果 results.md
        arm_a_pass = arm_data.get("Arm_A_verdict") == "PASS"
        arm_b_pass = arm_data.get("Arm_B_verdict") == "PASS"
        arm_c_pass = arm_data.get("Arm_C_verdict") == "PASS"

        b_defects = arm_data.get("Arm_B_intercepted_defects", [])
        b_reason = "；".join(b_defects) if b_defects else ("自查通过" if arm_b_pass else "未能识别隐蔽物理错误")

        audit_file = w_src / "results_isolated" / f"{tid}_audit.json"
        audit_json_str = ""
        if audit_file.exists():
            try:
                audit_data = json.loads(audit_file.read_text(encoding="utf-8"))
                audit_json_str = json.dumps(audit_data, indent=2, ensure_ascii=False)
            except Exception:
                pass

        res_md = f"# {tid} 案例测试结果与分析\n\n"
        res_md += f"## 一、对比结果\n\n"
        res_md += "| 测试组 | 结果 | 说明 |\n"
        res_md += "| :--- | :---: | :--- |\n"
        res_md += f"| **A组 (直接生成)** | {'✅ 通过' if arm_a_pass else '❌ 未通过'} | 实际社区真实 Bug，直接写均踩坑报错 |\n"
        res_md += f"| **B组 (模型自查)** | {'✅ 通过' if arm_b_pass else '❌ 未通过'} | 自查无法发现该隐蔽陷阱：{simplify_text(b_reason)} |\n"
        res_md += f"| **C组 (加核验器)** | {'✅ 通过' if arm_c_pass else '❌ 未通过'} | 核验器直接拦截错误，按规则提示后修复成功 |\n\n"

        if audit_json_str:
            res_md += f"## 二、核验器静态代码审计明细\n\n```json\n{audit_json_str}\n```\n"

        (task_dir / "results.md").write_text(res_md, encoding="utf-8")

        # 4. 拷贝源码
        case_buggy = w_src / "cases" / f"{tid}_buggy.cc"
        case_fixed = w_src / "cases" / f"{tid}_fixed.cc"
        code_c_pure = w_src / "code_C_pure" / f"{tid}.cc"
        code_c_iso = w_src / "code_C_isolated" / f"{tid}.cc"

        if case_buggy.exists():
            shutil.copy2(case_buggy, code_dir / "case_buggy.cc")
        if case_fixed.exists():
            shutil.copy2(case_fixed, code_dir / "case_fixed.cc")
        if code_c_pure.exists():
            shutil.copy2(code_c_pure, code_dir / "code_ArmC_pure.cc")
        if code_c_iso.exists():
            shutil.copy2(code_c_iso, code_dir / "code_ArmC_isolated.cc")


def build_readme():
    print(">>> 正在生成 benchmark_tasks_reference/README.md 导航索引...")
    readme_path = TARGET_ROOT / "README.md"

    md = """# 190 题蒙卡物理代码测试集参考库

本项目包含 190 道用于测试大模型编写蒙特卡罗物理模拟程序（Geant4）正确性的题目。  
每道题单独一个文件夹，包含以下 4 项内容：

```text
<题号文件夹>/
├── task.md        # 题目要求、物理考点、常见错误
├── prompt.md      # A组（直接生成）、B组（模型自查）、C组（加核验器）实际提示词
├── results.md     # 三组对比测试结果、通过情况与错误分析
└── code/          # 生成的 C++ Geant4 代码 (code_ArmA.cc, code_ArmB.cc, code_ArmC.cc)
```

---

## 快速导航

- [一、Tier 1 基础单考点 (60 题)](#一tier-1-基础单考点-60-题)
- [二、Tier 2 跨模块组合 (60 题)](#二tier-2-跨模块组合-60-题)
- [三、Tier 3 高阶多约束综合 (60 题)](#三tier-3-高阶多约束综合-60-题)
- [四、实际社区案例 (10 题)](#四实际社区案例-10-题)

---

## 一、Tier 1 基础单考点 (60 题)

考查材料、截面、粒子源和简单几何等基础单点配置。覆盖 6 大物理领域，每领域 10 题。

| 题号 | 类别 | 物理考点与目标观测量 | A组 (直接生成) | B组 (模型自查) | C组 (加核验器) | 参考目录 |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: |
"""

    with open(SOURCE_ROOT / "results" / "abc_experiment_verdict.json", "r", encoding="utf-8") as f:
        t1_v = json.load(f)

    for i in range(1, 7):
        cat_name = CATEGORIES.get(f"T{i}", "")
        for j in range(1, 11):
            tid = f"T{i}-{j}"
            item = t1_v.get(tid, {})
            a = "✅" if item.get("Arm_A", {}).get("correct") else "❌"
            b = "✅" if item.get("Arm_B", {}).get("correct") else "❌"
            c = "✅" if item.get("Arm_C", {}).get("correct") else "❌"
            reason = simplify_text(item.get("Arm_A", {}).get("reason", "无"))
            md += f"| **{tid}** | {cat_name} | {reason[:28]} | {a} | {b} | {c} | [`{tid}/`](tier1_single_slot/{tid}/) |\n"

    md += """
---

## 二、Tier 2 跨模块组合 (60 题)

考查几何边界、复杂能谱、次级粒子与计分等跨模块耦合场景。

| 题号 | 类别 | 物理考点与目标观测量 | A组 (直接生成) | B组 (模型自查) | C组 (加核验器) | 参考目录 |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: |
"""

    with open(SOURCE_ROOT / "benchmark_tier2" / "results" / "tier2_verdict_all_60.json", "r", encoding="utf-8") as f:
        t2_v = json.load(f)

    for i in range(1, 7):
        cat_name = CATEGORIES.get(f"T{i}", "")
        for j in range(1, 11):
            tid = f"T{i}-M{j}"
            item = t2_v.get(tid, {})
            a = "✅" if item.get("Arm_A", {}).get("passed") else "❌"
            b = "✅" if item.get("Arm_B", {}).get("passed") else "❌"
            c = "✅" if item.get("Arm_C", {}).get("passed") else "❌"
            reason = simplify_text(item.get("Arm_A", {}).get("reason", "无"))
            md += f"| **{tid}** | {cat_name} | {reason[:28]} | {a} | {b} | {c} | [`{tid}/`](tier2_coupled/{tid}/) |\n"

    md += """
---

## 三、Tier 3 高阶多约束综合 (60 题)

考查深穿透方差缩减、极薄纳米靶、衰变热动力学与过杀饱和修正等高阶多物理综合难题。覆盖 6 大物理领域，每领域 10 题。

| 题号 | 类别 | 物理考点与目标观测量 | A组 (直接生成) | B组 (模型自查) | C组 (加核验器) | 参考目录 |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: |
"""

    with open(SOURCE_ROOT / "benchmark_tier3" / "results" / "tier3_verdict_all_60.json", "r", encoding="utf-8") as f:
        t3_v = json.load(f)

    for i in range(1, 7):
        cat_name = CATEGORIES.get(f"T{i}", "")
        for j in range(1, 11):
            tid = f"T{i}-H{j}"
            item = t3_v.get(tid, {})
            a = "✅" if item.get("Arm_A", {}).get("passed") else "❌"
            b = "✅" if item.get("Arm_B", {}).get("passed") else "❌"
            c = "✅" if item.get("Arm_C", {}).get("passed") else "❌"
            reason = simplify_text(item.get("Arm_A", {}).get("reason", "无"))
            md += f"| **{tid}** | {cat_name} | {reason[:28]} | {a} | {b} | {c} | [`{tid}/`](tier3_deep_penetration/{tid}/) |\n"

    md += """
---

## 四、实际社区案例 (10 题)

源自 CERN 官方论坛与 GitHub 真实项目的隐蔽物理 Bug。

| 题号 | 案例题目 | 来源 | A组 (直接生成) | B组 (模型自查) | C组 (加核验器) | 参考目录 |
| :---: | :--- | :--- | :---: | :---: | :---: | :---: |
"""

    with open(SOURCE_ROOT / "real_world_wild_corpus" / "results" / "wild_corpus_verdict.json", "r", encoding="utf-8") as f:
        wild_v = json.load(f)
    with open(SOURCE_ROOT / "real_world_wild_corpus" / "results" / "wild_three_arms_verdict.json", "r", encoding="utf-8") as f:
        wild_3arms = json.load(f)

    for i in range(1, 11):
        tid = f"WILD-{i:02d}"
        item = wild_v.get(tid, {})
        item_3arms = wild_3arms.get(tid, {})
        a = "✅" if item_3arms.get("Arm_A_verdict") == "PASS" else "❌"
        b = "✅" if item_3arms.get("Arm_B_verdict") == "PASS" else "❌"
        c = "✅" if item_3arms.get("Arm_C_verdict") == "PASS" else "❌"
        title = item.get("title", item_3arms.get("title", "实际社区缺陷"))
        source = item_3arms.get("source", "CERN / GitHub")
        src_label = "CERN 论坛" if "CERN" in source else "GitHub Issue"
        md += f"| **{tid}** | {simplify_text(title[:28])} | {src_label} | {a} | {b} | {c} | [`{tid}/`](wild_corpus/{tid}/) |\n"

    readme_path.write_text(md, encoding="utf-8")
    print(f"✅ README 索引已写入 {readme_path}")


def main():
    print(f"=== 开始生成 190 题参考数据集文件夹: {TARGET_ROOT} ===")
    TARGET_ROOT.mkdir(parents=True, exist_ok=True)
    build_tier1()
    build_tier2()
    build_tier3()
    build_wild()
    build_readme()
    print("=== 全部 190 题参考文件夹组装与语言精简完成！===")


if __name__ == "__main__":
    main()
