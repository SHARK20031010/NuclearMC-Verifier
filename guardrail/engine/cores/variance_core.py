#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
variance_core.py —— 权重流与方差缩减守恒总线 (Variance & Weight Flux Core)
覆盖粒子统计权重守恒律、相空间几何重要性分裂、权重窗与加权统计方差计算。
"""

import re
from typing import Dict, List, Any


def strip_cpp_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


class VarianceCore:
    """权重流守恒总线：拦截几何重要性分裂漏乘权重、加权发散、权重丢失等灾难性数值事故。"""

    def audit(self, code_text: str) -> List[Dict[str, Any]]:
        clean = strip_cpp_comments(code_text)
        issues = []
        issues.extend(self._check_variance_reduction_weight_multiplication(clean))
        issues.extend(self._check_deep_penetration_necessity(clean))
        return issues

    def _check_variance_reduction_weight_multiplication(self, clean: str) -> List[Dict[str, Any]]:
        """权重流守恒契约：使用几何重要性分裂或权重窗时，计分累加必须严格乘入 GetWeight()。"""
        issues = []
        clean_no_deadlayer = re.sub(r'Dead\s*Layer|dead_layer|死层', '', clean, flags=re.IGNORECASE)
        has_vr = bool(
            re.search(r"G4GeometrySampler|G4WeightWindow|G4IStore|importance|split|roulette|layer|Layer|方差缩减|减方差", clean_no_deadlayer, re.IGNORECASE)
        )
        has_scoring = bool(
            re.search(r"(?:edep|flux|current|tally|dose|count|gCoil|gWeighted|gFlux)\s*(?:\+=|\=.*?\+)", clean, re.IGNORECASE)
        )
        has_weight_call = bool(re.search(r"GetWeight\s*\(\s*\)", clean))

        if has_vr and has_scoring and not has_weight_call:
            issues.append({
                "core": "VARIANCE_CORE",
                "contract": "WEIGHT_FLUX_CONSERVATION_CONTRACT",
                "severity": "CRITICAL",
                "defect_name": "方差缩减计分漏乘粒子权重 (GetWeight() 缺失导致数值虚高数百万倍)",
                "description": "代码中启用了分层几何重要性分裂（Splitting）或权重窗（Weight Window）以加速深穿透模拟。在粒子分裂后，每个分裂粒子的统计权重 w 会按几何重要性比率降低（如 w = 1/2^k）。如果计分器直接写 count++ 或 edep += dE，相当于将低权重粒子全部按初始全权重 (w=1) 累加，计算出的穿透通量与发热率将严重虚高数百万至数千万倍！",
                "remedy": "在所有计分步点必须乘入粒子当前动态权重：G4double w = step->GetTrack()->GetWeight(); flux += w; 或 edep += step->GetTotalEnergyDeposit() * w;"
            })
        return issues

    def _check_deep_penetration_necessity(self, clean: str) -> List[Dict[str, Any]]:
        """深穿透可行性契约：极厚屏蔽层 (如 >= 40 cm 铁/钢/水/混凝土) 常规模拟出射粒子为零，必须引入方差缩减或权重统计。"""
        issues = []
        # 检测是否包含大厚度中子/伽马屏蔽体 (如 40cm, 50cm, 60cm, 80cm, 100cm)
        is_thick_shield = bool(
            re.search(r"(?:40|50|60|70|80|90|100|1\s*\*?\s*m)\s*\*?\s*cm?", clean) and 
            re.search(r"\b(?:shield|shielding|iron|steel|G4_Fe|G4_Pb|G4_CONCRETE|G4_STAINLESS-STEEL|G4_POLYETHYLENE|屏蔽|不锈钢)\b", clean, re.IGNORECASE) and
            re.search(r"\b(?:deep|penetrat|深穿透|flux|current|transmission|fluence|attenuat|穿透|透射|衰减|出射|通量|流强|shield|shielding|屏蔽)\b", clean, re.IGNORECASE)
        )
        is_duct = bool(re.search(r"\b(?:duct|pipe|channel|maze|streaming|孔道|管道|迷宫)\b", clean, re.IGNORECASE))
        has_vr = bool(re.search(r"G4GeometrySampler|G4WeightWindow|G4IStore|importance|split|roulette|layer|Layer", clean, re.IGNORECASE))
        has_weight = bool(re.search(r"GetWeight", clean))

        if is_thick_shield and not is_duct and not has_weight and not has_vr:
            issues.append({
                "core": "VARIANCE_CORE",
                "contract": "DEEP_PENETRATION_VARIANCE_REDUCTION_CONTRACT",
                "severity": "HIGH",
                "defect_name": "深穿透厚屏蔽模拟未乘权重或未配置方差缩减",
                "description": "厚度达 40~100 cm 的重金属或含氢复合屏蔽体对快中子和伽马射线的宏观衰减通常跨越数个数量级。深穿透通量计分必须严格乘入粒子权重 GetWeight()，否则在引入重要性抽样或深穿透模拟时结果严重失真。",
                "remedy": "为深穿透通量计分乘入粒子权重：gFlux += 1.0 * step->GetTrack()->GetWeight(); 并根据需要划分几何重要性。"
            })
        return issues
