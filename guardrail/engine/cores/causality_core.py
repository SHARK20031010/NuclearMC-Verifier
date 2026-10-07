#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
causality_core.py —— 因果与时钟总线 (Causality & Timing Core)
覆盖相对论因果律、全局绝对时钟、探测器死时间到达间隔、流体停留时间与生物清除常数。
"""

import re
from typing import Dict, List, Any


def strip_cpp_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


class CausalityCore:
    """因果与时钟总线：拦截时钟源误读、符合时序倒错、流体停留时间缺失、死时间到达差缺失等缺陷。"""

    def audit(self, code_text: str) -> List[Dict[str, Any]]:
        clean = strip_cpp_comments(code_text)
        issues = []
        issues.extend(self._check_time_clock_source(clean))
        issues.extend(self._check_coincidence_timing_window(clean))
        issues.extend(self._check_pulse_dose_rate_normalization(clean))
        issues.extend(self._check_fluid_flow_residence_time(clean))
        issues.extend(self._check_internal_dosimetry_clearance(clean))
        issues.extend(self._check_detector_dead_time_interval(clean))
        return issues

    def _check_time_clock_source(self, clean: str) -> List[Dict[str, Any]]:
        """时钟契约：飞行时间 (TOF) 或事件到达时序必须读取绝对全局时钟 GetGlobalTime()。"""
        issues = []
        is_timing_context = bool(re.search(r"(?:tof|flight|arrival|coincid|prompt|time_of_flight|飞行时间|到达时间|符合)", clean, re.IGNORECASE))
        has_local_time = bool(re.search(r"GetLocalTime\s*\(\s*\)", clean))
        has_global_time = bool(re.search(r"GetGlobalTime\s*\(\s*\)", clean))

        if is_timing_context and has_local_time and not has_global_time:
            issues.append({
                "core": "CAUSALITY_CORE",
                "contract": "TIME_METRICS_CONTRACT",
                "severity": "CRITICAL",
                "defect_name": "符合测量或飞行时间误用局部时钟 GetLocalTime()",
                "description": "代码中涉及飞行时间 (TOF)、符合测量或到达时序，但读取了 GetLocalTime()。在 Geant4 中，GetLocalTime() 是粒子自生成以来的局域寿命时钟（粒子跨过边界或次级生成时容易清零）。用于测量粒子从源到探测器的绝对飞行时间或双探头时间差将导致时钟基准丢失。",
                "remedy": "改用绝对全局实验室时钟：G4double t = step->GetPreStepPoint()->GetGlobalTime();"
            })
        return issues

    def _check_coincidence_timing_window(self, clean: str) -> List[Dict[str, Any]]:
        """时序契约：双探头符合测量必须在 Event 级别比较时间差 |tA - tB| <= tau。"""
        issues = []
        is_pet_or_coinc = bool(re.search(r"(?:coincid|pet|annihil|符合|湮灭)", clean, re.IGNORECASE) and 
                              re.search(r"(?:DetA|det1|crystal1|left).*?(?:DetB|det2|crystal2|right)", clean, re.IGNORECASE))
        if not is_pet_or_coinc:
            return issues

        has_stepping_coinc = bool(re.search(r"UserSteppingAction[\s\S]*?(?:coincid\+\+|gCoincidences\+\+)", clean, re.IGNORECASE))
        has_event_coinc = bool(re.search(r"EndOfEventAction[\s\S]*?(?:abs\s*\(\s*t[AB12]|delta_t|dt)", clean, re.IGNORECASE))

        if has_stepping_coinc and not has_event_coinc:
            issues.append({
                "core": "CAUSALITY_CORE",
                "contract": "COINCIDENCE_EVENT_TIMING_CONTRACT",
                "severity": "HIGH",
                "defect_name": "符合测量跨探头时序判定倒错",
                "description": "代码试图在单个 UserSteppingAction 步点中同时判断两个独立探头的击中状态，这违背了 Geant4 步进时序（单步只能感知当前径迹在当前探头的步进）。",
                "remedy": "应在 UserSteppingAction 中分别记录各探头的到达时刻与沉积能量，并在 UserEventAction::EndOfEventAction 中计算两个探测器首个脉冲到达时间差。"
            })
        return issues

    def _check_pulse_dose_rate_normalization(self, clean: str) -> List[Dict[str, Any]]:
        """脉冲剂量时序契约：脉冲峰值吸收剂量率必须归一化至微观脉冲持续时间 (如 5 us)。"""
        issues = []
        is_pulse_context = bool(re.search(r"(?:pulse|flash|脉冲).*?(?:dose_rate|剂量率)", clean, re.IGNORECASE))
        if is_pulse_context:
            has_pulse_width = bool(re.search(r"(?:5(?:\.0)?\s*\*?\s*(?:us|microsecond)|5e-6|pulse_width|tau|duration)", clean, re.IGNORECASE))
            if not has_pulse_width:
                issues.append({
                    "core": "CAUSALITY_CORE",
                    "contract": "PULSE_DURATION_NORMALIZATION_CONTRACT",
                    "severity": "HIGH",
                    "defect_name": "脉冲峰值剂量率时间归一化分母错误",
                    "description": "计算高剂量率脉冲辐射的峰值吸收剂量率时，未除以单脉冲微观持续时间 (如 5 微秒)，误除以宏观周期或宏观秒，导致脉冲峰值剂量率被严重低估数十万倍。",
                    "remedy": "将单脉冲总吸收剂量严格归一化至脉冲持续时间：G4double peakDoseRate = dosePerPulse / (5.0 * microsecond);"
                })
        return issues

    def _check_fluid_flow_residence_time(self, clean: str) -> List[Dict[str, Any]]:
        """流动介质活化因果契约：循环流动水体反应率必须计入堆芯停留时间 t = L/v 与回路衰变。"""
        issues = []
        is_fluid_activation = bool(re.search(r"(?:flow|water|fluid|coolant|N-16|N16|冷却水|流体).*?(?:activat|活化|回路)", clean, re.IGNORECASE))
        has_residence_time = bool(re.search(r"L\s*/\s*v|residence|tau|flow_time|停留时间", clean, re.IGNORECASE))

        if is_fluid_activation and not has_residence_time:
            issues.append({
                "core": "CAUSALITY_CORE",
                "contract": "FLUID_FLOW_RESIDENCE_TIME_CONTRACT",
                "severity": "HIGH",
                "defect_name": "循环流动流体活化未引入堆芯流动停留时间 t = L/v 修正",
                "description": "流动水体（如压水堆一回路水活化生成 N-16）在堆芯中处于持续流动状态。活化产额严格受限于流体在堆芯辐照区的有效停留时间 t_core = L / v。若将其简化为静态水体无限照射饱和活化，活化比活度将被高估数十倍。",
                "remedy": "引入流体堆芯流动停留时间：G4double t_core = core_length / flow_velocity; A = sigma * phi * N * (1.0 - std::exp(-lambda * t_core));"
            })
        return issues

    def _check_internal_dosimetry_clearance(self, clean: str) -> List[Dict[str, Any]]:
        """体内放射性代谢清除契约：内照射器官滞留活度必须耦合物理衰变常数与生物代谢清除常数。"""
        issues = []
        is_internal_dose = bool(re.search(r"(?:inhalation|lung|ingestion|retention|clearance|吸入|内照射|滞留)", clean, re.IGNORECASE))
        has_bio_clearance = bool(re.search(r"lambda_b|lambda_biol|biological|clearance|lambda_eff", clean, re.IGNORECASE))

        if is_internal_dose and not has_bio_clearance:
            issues.append({
                "core": "CAUSALITY_CORE",
                "contract": "INTERNAL_DOSIMETRY_CLEARANCE_CONTRACT",
                "severity": "HIGH",
                "defect_name": "体内器官放射性滞留活度仅用物理半衰期 (漏除生理清除常数)",
                "description": "吸入或食入放射性核素在人体器官中的有效排除速率由有效衰变常数 lambda_eff 决定：lambda_eff = lambda_phys + lambda_biol。如果仅采用物理衰变常数而忽略人体的生理代谢清除常数，长寿命或强代谢核素在靶器官的累积活度与待积剂量将被虚高数倍到数十倍。",
                "remedy": "耦合物理与生物双清除常数：G4double lambda_eff = lambda_phys + lambda_biol; A(t) = A0 * std::exp(-lambda_eff * t);"
            })
        return issues

    def _check_detector_dead_time_interval(self, clean: str) -> List[Dict[str, Any]]:
        """探测器死时间因果筛选契约：高计数率死时间损失必须判断相邻粒子到达时间间隔 delta_t < tau。"""
        issues = []
        is_dead_time_context = bool(re.search(r"(?:dead_time|pileup|paralyzable|死时间|计数损失)", clean, re.IGNORECASE))
        has_time_interval = bool(re.search(r"(?:dt|delta_t|t_diff|time_diff)\s*<\s*(?:tau|dead_time)", clean, re.IGNORECASE) or
                                re.search(r"tau|dead_time", clean, re.IGNORECASE) and re.search(r"t_last|last_time|prev_time", clean, re.IGNORECASE))

        if is_dead_time_context and not has_time_interval:
            issues.append({
                "core": "CAUSALITY_CORE",
                "contract": "DETECTOR_DEAD_TIME_INTERVAL_CONTRACT",
                "severity": "HIGH",
                "defect_name": "探测器死时间模拟未实现相邻事件到达时间差 delta_t < tau 判定",
                "description": "探测器死时间计数损失的微观物理机理是相邻两个脉冲到达时间间隔 delta_t 小于探测器电子学分辨率时间 tau。代码中声明了死时间模拟，但未记录上一事件到达时刻或未判断相邻事件到达时间差，导致死时间计数损失恒为零。",
                "remedy": "记录前一脉冲到达时间并进行死时间筛选：if (current_time - last_event_time < dead_time_tau) { /* 丢弃该计数 (死时间损失) */ } else { last_event_time = current_time; }"
            })
        return issues
