#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
slot_inquisitor.py —— 基于「五动作 × 31 槽位」的通用槽位驱动元逼问器

解决核心痛点：
即使面对从未见过的第 61 题、第 100 题，只要代码具有蒙特卡洛计算的特征，
该引擎就能自动识别代码在 31 个槽位上的激活图谱，并依据激活的槽位动态生成
针对性的物理自证元逼问（Meta-Inquiry Checklist），彻底告别对孤立题目规则的硬依赖。
"""

import re
from typing import Dict, List, Set, Any
try:
    from .api_contracts import strip_cpp_comments
except (ImportError, ValueError):
    from api_contracts import strip_cpp_comments


class SlotInquisitor:
    def __init__(self):
        # 31 维核心动作槽位的特征探测签名与形式化元质询 (Formal Meta-Inquiries)
        self.slot_definitions = {
            # === 【定】Define ===
            "D1_GEOM_SOLID": {
                "action": "Define",
                "name": "几何实体构造",
                "patterns": [r"G4Box", r"G4Tubs", r"G4Sphere", r"G4Orb", r"G4Cons"],
                "inquiry": "几何尺寸元问：探测器与样品实体关键特征尺寸（半径/厚度）是否与图纸严格一致？是否存在微米级死层或壁厚遗漏？"
            },
            "D4_MATERIAL_PROPERTIES": {
                "action": "Define",
                "name": "材料物理属性表挂载",
                "patterns": [r"G4MaterialPropertiesTable", r"SetBirksConstant", r"RINDEX", r"ABSLENGTH", r"SCINTILLATIONTIMECONSTANT"],
                "inquiry": "材料属性元问：发光介质是否完整配置了随波长折射率 RINDEX？有机闪烁体是否显式设置了 Birks 猝灭常数？"
            },
            "D5_PHYSICS_CONSTRUCTOR": {
                "action": "Define",
                "name": "物理列表与构造器挂载",
                "patterns": [r"RegisterPhysics", r"G4VModularPhysicsList", r"G4Em", r"G4Hadron", r"G4OpticalPhysics"],
                "inquiry": "物理配置元问：物理列表是否成对包含强子弹性 (Elastic) 与非弹性 (Inelastic)？光核反应/辐射化学是否显式挂载了专属物理包？"
            },
            # === 【抽】Sample ===
            "S1_SOURCE_POSITION": {
                "action": "Sample",
                "name": "源空间位置抽样",
                "patterns": [r"SetParticlePosition", r"G4UniformRand", r"RandFlat"],
                "inquiry": "空间抽样元问：面源或体源空间抽样是否包含几何测度雅可比变换（如圆面源半径抽样是否采用 r = R * sqrt(xi)）？"
            },
            "S2_SOURCE_DIRECTION": {
                "action": "Sample",
                "name": "源动量方向抽样",
                "patterns": [r"SetParticleMomentumDirection", r"G4RandomDirection", r"cosTheta", r"sinTheta"],
                "inquiry": "方向抽样元问：点源是否在全立体角各向同性发射（G4RandomDirection）？宇宙线源是否正确抽样天顶角 cos^2(theta) 分布？"
            },
            "S3_SOURCE_ENERGY": {
                "action": "Sample",
                "name": "源能量谱抽样",
                "patterns": [r"SetParticleEnergy", r"G4RandGauss", r"Watt", r"CLHEP::RandGeneral"],
                "inquiry": "能谱抽样元问：连续谱抽样或高斯能展是否设置了非负截断 (E_k > 0)？离散伽马线强度与分支比是否与 ENSDF 评价数据真值完全一致？"
            },
            # === 【取】Fetch ===
            "F2_STEP_KINEMATICS": {
                "action": "Fetch",
                "name": "步进动能与动量读取",
                "patterns": [r"GetKineticEnergy", r"GetTotalEnergy", r"GetMomentum"],
                "inquiry": "状态读取元问：初级/次级粒子初生动能是在产生时刻（PreStepPoint）读取，还是在经历输运能损后的推进步点读取？"
            },
            "F3_STEP_TIMING": {
                "action": "Fetch",
                "name": "步进时钟选取",
                "patterns": [r"GetGlobalTime", r"GetLocalTime", r"GetProperTime"],
                "inquiry": "时钟标尺元问：飞行时间 (TOF) 或事件到达时序读取的是源项发射绝对时钟 (GetGlobalTime)，还是本径迹局域时钟 (GetLocalTime)？"
            },
            "F4_PROCESS_PROVENANCE": {
                "action": "Fetch",
                "name": "物理反应道追溯",
                "patterns": [r"GetProcessDefinedStep", r"GetCreatorProcess"],
                "inquiry": "反应道溯源元问：次级核或次级粒子产生来源是否通过 track->GetCreatorProcess() 追溯？是否误用了推进步长限制器 GetProcessDefinedStep()？"
            },
            "F5_GEOM_BOUNDARY": {
                "action": "Fetch",
                "name": "几何边界状态获取",
                "patterns": [r"fGeomBoundary", r"GetStepStatus"],
                "inquiry": "边界判定元问：几何边界跨越与流穿计数是否在后步点 (GetPostStepPoint) 上进行判定？"
            },
            # === 【记】Accumulate ===
            "A1_ENERGY_DEPOSIT": {
                "action": "Accumulate",
                "name": "能量沉积求和",
                "patterns": [r"GetTotalEnergyDeposit", r"edep\s*\+=", r"eDep\s*\+="],
                "inquiry": "沉积能量元问：灵敏区能量记账是否彻底剔除了前表面死层、钝化层或反射层的无效能量沉积？"
            },
            "A4_WEIGHT_MULTIPLICATION": {
                "action": "Accumulate",
                "name": "方差缩减权重记账",
                "patterns": [r"GetWeight", r"importance", r"split", r"roulette", r"G4WeightWindow"],
                "inquiry": "权重记账元问：启用了几何分裂/重要性方差缩减后，所有计数与通量累加是否严格乘入了粒子动态权重 GetWeight()？"
            },
            "A5_CONTAINER_RESET": {
                "action": "Accumulate",
                "name": "统计容器生命周期清空",
                "patterns": [r"std::set", r"std::vector", r"trackId", r"TrackID"],
                "inquiry": "生命周期元问：用于径迹去重或次级粒子产额统计的容器，是否在每个事件的 BeginOfEventAction 中显式调用了 clear()？次级顶点是否限定首步？"
            },
            "A6_CHARGE_STATISTICS": {
                "action": "Accumulate",
                "name": "探测器电荷与本征涨落",
                "patterns": [r"variance", r"sigma", r"fano", r"eh_pair", r"carrier"],
                "inquiry": "载流子统计元问：半导体电荷产生涨落方差是否乘入了材料 Fano 因子（Si ~0.115, Ge ~0.08）？是否误用了纯泊松统计？"
            },
            # === 【换】Convert ===
            "C1_DOSE_CONVERSION": {
                "action": "Convert",
                "name": "吸收剂量与剂量率换算",
                "patterns": [r"gray", r"dose", r"Gy", r"MeV\s*/\s*g", r"joule\s*/\s*kg"],
                "inquiry": "剂量分母元问：吸收剂量计算中分母质量是否由体密度乘真实体积严格求得？高剂量率脉冲剂量率分母是脉冲持续时间还是重复周期？"
            },
            "C3_MICRODOSIMETRY": {
                "action": "Convert",
                "name": "微剂量学线能换算",
                "patterns": [r"lineal", r"lineic", r"keV\s*/\s*um", r"mean_chord"],
                "inquiry": "微剂量分母元问：微球或球形腔体内的微剂量线能 y = edep / l 计算中，平均弦长分母是否采用柯西定理严格真值 4r/3？"
            },
            "C4_ACTIVATION_CONVERSION": {
                "action": "Convert",
                "name": "活化饱和度与衰变活度换算",
                "patterns": [r"activity", r"becquerel", r"Bq", r"lambda", r"decay_constant", r"half_life"],
                "inquiry": "活度量纲元问：停照活度公式 A = R*(1 - exp(-lambda*t)) 计算中是否误除了衰变常数 lambda？母子衰变链是否采用了严密 Bateman 解？"
            },
            "C5_DETECTION_EFFICIENCY": {
                "action": "Convert",
                "name": "探测效率归一化",
                "patterns": [r"efficiency", r"peak_eff", r"total_eff", r"nEvents"],
                "inquiry": "效率归一化元问：探测全能峰效率的分母是放射源发射粒子总数（绝对效率），还是投射到晶体前表面的粒子数（本征效率）？"
            }
        }

    def analyze_code_slots(self, code_text: str) -> Dict[str, Any]:
        """分析输入代码并返回激活的槽位列表及其生成的元质询清单。"""
        clean_code = strip_cpp_comments(code_text)
        active_slots = []
        inquiries = []

        for slot_id, info in self.slot_definitions.items():
            is_active = any(re.search(pat, clean_code) for pat in info["patterns"])
            if is_active:
                active_slots.append({
                    "slot_id": slot_id,
                    "action": info["action"],
                    "name": info["name"]
                })
                inquiries.append({
                    "slot_id": slot_id,
                    "action": info["action"],
                    "name": info["name"],
                    "question": info["inquiry"]
                })

        return {
            "total_slots_scanned": len(self.slot_definitions),
            "active_slot_count": len(active_slots),
            "active_slots": active_slots,
            "meta_inquiries": inquiries
        }


if __name__ == "__main__":
    inq = SlotInquisitor()
    sample = """
    void UserSteppingAction(const G4Step* step) {
        G4double edep = step->GetTotalEnergyDeposit();
        G4double t = step->GetPostStepPoint()->GetGlobalTime();
    }
    """
    res = inq.analyze_code_slots(sample)
    import json
    print(json.dumps(res, indent=2, ensure_ascii=False))
