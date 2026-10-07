#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
universal_engine.py —— 五大正交物理守恒不变式形式化核验与诊断引擎
完全脱离题号与任务先验，基于第一性原理守恒量与 Geant4 框架底层语义契约，
对任意输入的未标定 Geant4 C++ 源代码进行确定性诊断与针对性自证质询。
"""

import os
import sys
from pathlib import Path
from typing import Dict, List, Any, Optional

_HERE = Path(__file__).resolve().parent

try:
    from .cores.causality_core import CausalityCore
    from .cores.measure_core import MeasureCore
    from .cores.variance_core import VarianceCore
    from .cores.lifecycle_core import LifecycleCore
    from .cores.nuclear_core import NuclearCore
    from .cores.intent_core import IntentCore
    from .slot_inquisitor import SlotInquisitor
except (ImportError, ValueError):
    sys.path.insert(0, str(_HERE))
    from cores.causality_core import CausalityCore
    from cores.measure_core import MeasureCore
    from cores.variance_core import VarianceCore
    from cores.lifecycle_core import LifecycleCore
    from cores.nuclear_core import NuclearCore
    from cores.intent_core import IntentCore
    from slot_inquisitor import SlotInquisitor


class UniversalGuardrailEngine:
    """五大正交守恒防护总线统一调度引擎（含环 0 上游意图联动防错总线）。"""

    def __init__(self):
        self.causality_core = CausalityCore()
        self.measure_core = MeasureCore()
        self.variance_core = VarianceCore()
        self.lifecycle_core = LifecycleCore()
        self.nuclear_core = NuclearCore()
        self.intent_core = IntentCore()
        self.slot_inquisitor = SlotInquisitor()

    def inspect_source(self, code_text: str, filename: str = "code.cc", intent_text: Optional[str] = None, require_intent: bool = False) -> Dict[str, Any]:
        """对任意输入的 C++ 源码及环 0 回执进行全面审计（零任务ID、零考题剧透）。"""
        # 0. 环 0 需求确认与代码意图联动审计总线
        intent_audit = self.intent_core.audit(code_text, intent_text=intent_text, require_intent=require_intent)
        intent_issues = intent_audit.get("issues", [])

        # 1. 因果与时钟总线
        causality_issues = self.causality_core.audit(code_text)

        # 2. 相空间测度总线
        measure_issues = self.measure_core.audit(code_text)

        # 3. 权重流与方差缩减总线
        variance_issues = self.variance_core.audit(code_text)

        # 4. 步进生命周期与边界总线
        lifecycle_issues = self.lifecycle_core.audit(code_text)

        # 5. 核素衰变链与材料常数总线
        nuclear_issues = self.nuclear_core.audit(code_text)
        detected_isotopes = self.nuclear_core.detect_isotopes(code_text)

        all_defects = (
            intent_issues +
            causality_issues +
            measure_issues +
            variance_issues +
            lifecycle_issues +
            nuclear_issues
        )

        critical_defects = [d for d in all_defects if d.get("severity") in ("CRITICAL", "HIGH")]

        # 生成槽位驱动的物理自证元质询
        slot_analysis = self.slot_inquisitor.analyze_code_slots(code_text)

        return {
            "target_file": filename,
            "detected_isotopes": detected_isotopes,
            "intent_status": {
                "has_intent": intent_audit.get("has_intent", False),
                "stats": intent_audit.get("stats", {}),
                "defect_count": len(intent_issues),
            },
            "verdict": "FAIL" if len(critical_defects) > 0 else "PASS",
            "critical_defect_count": len(critical_defects),
            "total_defect_count": len(all_defects),
            "defects_by_core": {
                "intent_core": intent_issues,
                "causality_core": causality_issues,
                "measure_core": measure_issues,
                "variance_core": variance_issues,
                "lifecycle_core": lifecycle_issues,
                "nuclear_core": nuclear_issues,
            },
            "all_defects": all_defects,
            "slot_meta_inquiries": slot_analysis.get("meta_inquiries", [])
        }


    def format_slot_inquiry_report(self, result: Dict[str, Any]) -> str:
        """
        生成严格符合科学规范的【Level 3: 第一性原理纯抽象物理守恒与测度不变性质询】。
        严格纪律：
        1. 零代码与零 API 引用：严禁出现 GetCurrentStepNumber, SetDeltaChord, GetLocalTime 等任何函数名；
        2. 零经验公式与专有名词剧透：严禁出现 Birks, Fano, TSL, 雅可比公式 等任何具体经验模型名称；
        3. 零题目针对性私货：完全基于 5 大普遍性物理守恒律与几何/概率测度不变性发起抽象自证提问；
        由大模型基于第一性原理，自行倒查其数学建模、时间基准、拓扑生命周期与概率测度是否闭环。
        """
        critical_count = result.get("critical_defect_count", 0)
        isotopes = result.get("detected_isotopes", [])

        if critical_count == 0:
            iso_str = f"（自动识别核素：{', '.join(isotopes)}）" if isotopes else ""
            return f"【认知护栏检测结果】：\n✓ 物理守恒与相空间测度不变性核验通过 {iso_str}。"

        intent_issues = result.get("defects_by_core", {}).get("intent_core", [])
        ring0_inquiry = ""
        if intent_issues:
            ring0_inquiry = """
0. 【环 0 目标物理量与时空基准对齐（意图与规范抽象）】
   - 目标可观测量定义与量纲：所计算的量是微观能量沉积率还是宏观剂量当量？归一化基准是相对每源粒子还是依赖外部绝对活度/束流？探测几何是无限表面还是局域平均体积？代码中的分母换算是否与用户真实物理目标闭环对齐？
"""

        return f"""【第一性原理认知护栏 · 静态物理守恒与测度不变性审计结果】：
🚨 状态：系统检测到当前代码存在未闭环的物理守恒律破损、相空间测度畸变或时空生命周期不变量违背。

## 第一性原理「守恒、不变性与意图对齐」纯抽象物理质询：
请依据以下不依赖任何具体经验公式或框架实现的普遍性第一性原理，逐项倒查代码中的数学建模与微观物理逻辑：{ring0_inquiry}
1. 【微分相空间测度不变性（几何与分布抽象）】
   - 相空间体积元守恒：空间坐标、动量方向或能谱相空间从连续物理真实向离散抽样映射时，微分微元是否在坐标变换下保持测度不变？是否存在非线性变换导致的测度畸变（如极坐标、柱坐标或曲面映射下的虚假空间聚集/发散）？

2. 【因果律与绝对时钟单调性（时间基准抽象）】
   - 物理时间单调性：飞行时间、符合时序或时间响应所依赖的物理时间标尺，是否满足全局因果单调递增律？是否存在因局部几何界面切换或输运步进截断而导致的参考系非物理原点重置？

3. 【时空产生奇点与连续输运线积分的生命周期隔离（拓扑与测度抽象）】
   - 产生点 vs 输运态：物理相互作用导致的粒子/核素创生事件，是属于时空微观反应的局部脉冲奇点（点事件），还是粒子在介质中持续滑移的自由程积分？当前计分生命周期是将单次创生奇点与后续的连续输运状态混为一谈导致了高阶多重计数，还是严格实现了产生与输运的拓扑隔离？

4. 【概率测度守恒与估计量期望无偏性（蒙特卡洛权重流抽象）】
   - 期望无偏性：在非模拟（Non-analog）输运、空间区域分裂、轮盘赌或多重方差缩减作用下，可观测量的统计累加是否维持数学期望无偏？每次相空间密度的人工调整是否在权重流上进行了严格守恒的代数补偿？

5. 【微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）】
   - 能量响应非线性：微观能量沉积转化为宏观观测信号时，是否隐含假设了完全理想的线性比例关系？高电离激发密度下的非辐射耗散、微观载流子产生的亚泊松离散相关性、以及介质微观热态/晶格散射通道是否完备闭环？

请对照上述第一性原理抽象质询，对代码中的数理逻辑与物理建模进行全局重审与自主修复。"""

    def format_interrogation_report(self, result: Dict[str, Any], pure_inquiry: bool = True) -> str:
        """生成高信噪比的第一性原理物理守恒量自证质询清单（零代码泄漏模式）。"""
        if pure_inquiry:
            return self.format_slot_inquiry_report(result)

        fname = result.get("target_file", "code.cc")
        critical_count = result.get("critical_defect_count", 0)
        defects = result.get("all_defects", [])
        isotopes = result.get("detected_isotopes", [])
        inquiries = result.get("slot_meta_inquiries", [])
        intent_stat = result.get("intent_status", {})

        if critical_count == 0:
            iso_str = f"（自动识别核素：{', '.join(isotopes)}）" if isotopes else ""
            intent_str = ""
            if intent_stat.get("has_intent"):
                st = intent_stat.get("stats", {})
                intent_str = f" [Ring-0 规约回执就绪：用户决策 {st.get('user_decisions', 0)}, AI 代理决策 {st.get('ai_decisions', 0)}]"
            return f"【形式化物理核验结果】：\n✓ 五大守恒不变式总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 {iso_str}{intent_str}。"

        lines = [
            f"【形式化物理核验结果 - 拦截到 {critical_count} 项物理与框架契约违约】："
        ]

        if intent_stat.get("has_intent"):
            st = intent_stat.get("stats", {})
            lines.append(f"📋 Ring-0 前置输入规约契约已挂载：用户决策 {st.get('user_decisions', 0)} 项，AI 代理决策 {st.get('ai_decisions', 0)} 项（AI 决策占比：{st.get('ai_decision_ratio', 'N/A')}）")

        if isotopes:
            lines.append(f"🔍 物理指纹识别核素：`{', '.join(isotopes)}`")

        for idx, d in enumerate(defects, start=1):
            sev = d.get("severity", "HIGH")
            dname = d.get("defect_name", d.get("contract", "物理缺陷"))
            desc = d.get("description", "")
            remedy = d.get("remedy", "")
            lines.append(f"\n🚨 [{idx}] 严重度：`{sev}` · 【{dname}】\n  - **缺陷诊断**：{desc}\n  - **物理守恒针对性质询与修正建议**：{remedy}")

        if inquiries:
            lines.append("\n## 基于「五动作 × 31 槽位」的物理自证清单：")
            for inq in inquiries[:4]:
                q_str = inq.get("question", str(inq)) if isinstance(inq, dict) else str(inq)
                lines.append(f"- {q_str}")

        return "\n".join(lines)

    def format_markdown_report(self, result: Dict[str, Any]) -> str:
        """输出可直接呈现给人类工程师或大模型的 Markdown 格式完整审计报告。"""
        return self.format_interrogation_report(result, pure_inquiry=False)


# 模块全局默认实例
default_engine = UniversalGuardrailEngine()


def inspect(code_text: str, filename: str = "code.cc", intent_text: Optional[str] = None, require_intent: bool = False) -> Dict[str, Any]:
    return default_engine.inspect_source(code_text, filename, intent_text=intent_text, require_intent=require_intent)


def format_report(result: Dict[str, Any], pure_inquiry: bool = True) -> str:
    return default_engine.format_interrogation_report(result, pure_inquiry=pure_inquiry)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        path = sys.argv[1]
        code = open(path, encoding="utf-8").read()
        res = inspect(code, os.path.basename(path))
        print(format_report(res))
    else:
        print("Universal Guardrail Engine is ready.")
