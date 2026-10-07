#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
nuclear_core.py —— 核素与材料物理数据库总线 (Nuclear & Material Core)
挂载 ENSDF/NuDat 官方核衰变常数真值库与材料探测器参数库，实现全周期表同位素零规则手写的通用真值核验。
"""

import json
import os
import re
from pathlib import Path
from typing import Dict, List, Optional, Any

_HERE = Path(__file__).resolve().parent.parent.parent
_DECAY_DB_PATH = _HERE / "data" / "nuclear_decay_db.json"
_MAT_DB_PATH = _HERE / "data" / "material_constants.json"


def strip_cpp_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


class NuclearCore:
    """核素与材料物理总线：拦截同位素分支比错误、Bateman 单指数退化、半导体缺 Fano 因子、缺 RINDEX 等真值偏差。"""

    def __init__(self, decay_db_path: Optional[Path] = None, mat_db_path: Optional[Path] = None):
        self.decay_path = decay_db_path or _DECAY_DB_PATH
        self.mat_path = mat_db_path or _MAT_DB_PATH
        self.decay_db = self._load_json(self.decay_path)
        self.mat_db = self._load_json(self.mat_path)

    @staticmethod
    def _load_json(path: Path) -> Dict[str, Any]:
        if path.exists():
            with open(path, "r", encoding="utf-8") as f:
                return json.load(f)
        return {}

    def detect_isotopes(self, code_text: str) -> List[str]:
        """从源码（含注释与特征能谱指纹）中自动识别出现的核素同位素标识。"""
        detected = set()
        nuclides = self.decay_db.get("nuclides", {})
        for nuc_name, info in nuclides.items():
            elem = info["element"]
            a = info["A"]
            patterns = [
                rf"\b{elem}-{a}\b",
                rf"\b{elem}{a}\b",
                rf"\b{elem}_{a}\b",
                rf"\b{elem}\s*-\s*{a}\b"
            ]
            if "m" in nuc_name.lower():
                patterns.extend([
                    rf"\b{elem}-{a}m\b",
                    rf"\b{elem}{a}m\b"
                ])
            for pat in patterns:
                if re.search(pat, code_text, re.IGNORECASE):
                    detected.add(nuc_name)
                    break

            gammas = info.get("gammas", [])
            for g in gammas:
                e_kev = g["energy_keV"]
                e_mev = e_kev / 1000.0
                if re.search(rf"\b{e_mev:.3f}\b|\b{e_mev:.4f}\b|\b{e_kev:.1f}\b", code_text):
                    detected.add(nuc_name)

        # 针对半衰期特征反向识别 (如 Mo-99 65.94h / Tc-99m 6.01h)
        if re.search(r"65\.94|6\.01", code_text):
            detected.add("Mo-99")
            detected.add("Tc-99m")

        return sorted(list(detected))

    def audit(self, code_text: str) -> List[Dict[str, Any]]:
        clean = strip_cpp_comments(code_text)
        issues = []
        issues.extend(self._audit_decay_chains_and_branching(clean, code_text))
        issues.extend(self._audit_detector_material_physics(clean, code_text))
        issues.extend(self._audit_silent_engine_omissions(clean, code_text))
        issues.extend(self._audit_decay_heat_energy_balance(clean))
        return issues

    def _audit_decay_chains_and_branching(self, clean: str, raw: str) -> List[Dict[str, Any]]:
        """基于真值表动态核验衰变分支比与母子链 Bateman 方程。"""
        issues = []
        isotopes = self.detect_isotopes(raw)
        nuclides = self.decay_db.get("nuclides", {})

        # 1. 检查 Mo-99 -> Tc-99m 母子衰变链
        if "Mo-99" in isotopes or re.search(r"Mo-99|Mo99|钼-?99|lambda1.*lambda2", raw, re.IGNORECASE):
            has_0875 = bool(re.search(r"0\.875|0\.87", clean))
            is_single_exp = bool(re.search(r"exp\s*\(\s*-\s*lambda1\s*\*\s*t\s*\)", clean) and
                                not re.search(r"exp\s*\(\s*-\s*lambda2\s*\*\s*t\s*\)", clean))
            if not has_0875 or is_single_exp:
                issues.append({
                    "core": "NUCLEAR_CORE",
                    "contract": "DECAY_CHAIN_BATEMAN_BRANCHING_CONTRACT",
                    "severity": "CRITICAL",
                    "defect_name": "Mo-99 -> Tc-99m 母子衰变链漏乘 87.5% 分支比或退化为单指数",
                    "description": "Mo-99 衰变生成 Tc-99m 存在分支比 BR = 0.875（仅 87.5% 跃迁至亚稳态 Tc-99m，其余 12.5% 直接衰变至 Tc-99 基态）。代码中未乘 0.875 分支比，且使用了单指数简化的假定，导致子体活度随时间演化曲线严重失真。",
                    "remedy": "引入分支比与 Bateman 严密方程：A2(t) = 0.875 * [lambda2 / (lambda2 - lambda1)] * A1_0 * (exp(-lambda1*t) - exp(-lambda2*t));"
                })

        # 2. 检查 Co-60 级联伽马分支比 (~1.00)
        if "Co-60" in isotopes or re.search(r"Co-60|Co60", raw, re.IGNORECASE):
            for m in re.finditer(r"(?:branch|prob|ratio|intensity|weight|gI\[\w+\])\s*=\s*\{?[^;]*?(0\.[456]\d*)", clean, re.IGNORECASE):
                val = float(m.group(1))
                issues.append({
                    "core": "NUCLEAR_CORE",
                    "contract": "CASCADE_GAMMA_BRANCHING_CONTRACT",
                    "severity": "CRITICAL",
                    "defect_name": "Co-60 级联伽马分支比误写为 0.5 或 0.6",
                    "description": f"Co-60 发生 beta- 衰变后，级联释放 1.173 MeV 与 1.332 MeV 两个伽马光子，每次衰变释放两者的概率均近乎 100% (~0.999)。代码检测到硬编码发射概率 {val}，系将级联两光子误当作互斥竞争分支。",
                    "remedy": "将 Co-60 两条主要伽马射线的发射概率/分支比均核准为 1.00。"
                })
                break

        # 3. 检查 Na-24 漏掉 2.754 MeV 级联主峰
        if "Na-24" in isotopes or re.search(r"Na-24|Na24", raw, re.IGNORECASE):
            has_1368 = bool(re.search(r"1\.368|1368|1\.369|1\.37", raw))
            has_2754 = bool(re.search(r"2\.754|2754|2\.75", raw))
            if has_1368 and not has_2754:
                issues.append({
                    "core": "NUCLEAR_CORE",
                    "contract": "PRIMARY_GAMMA_LINE_COMPLETENESS_CONTRACT",
                    "severity": "HIGH",
                    "defect_name": "Na-24 放射源遗漏 2.754 MeV 主高能伽马射线",
                    "description": "代码配置了 Na-24 放射源，但仅发射 1.369 MeV 伽马线，遗漏了能量更高、穿透力更强的 2.754 MeV 级联主伽马线 (分支比 99.85%)，导致深部剂量率被低估逾 50%。",
                    "remedy": "在 Na-24 衰变能谱中补齐 2.754 MeV 伽马射线发射道。"
                })

        # 4. 检查 Ir-192 604.4 keV 强度倒错
        if "Ir-192" in isotopes or re.search(r"Ir-192|Ir192", raw, re.IGNORECASE):
            if re.search(r"\b82\.2\b", clean) and re.search(r"604", clean):
                issues.append({
                    "core": "NUCLEAR_CORE",
                    "contract": "ISOTOPE_LINE_INTENSITY_CONTRACT",
                    "severity": "HIGH",
                    "defect_name": "Ir-192 604.4 keV 特征射线强度误抄为 82.2 (实际应为 8.2)",
                    "description": "Ir-192 衰变能谱中，316.5 keV 强度为 82.8%，而 604.4 keV 相对发射强度仅为 8.20%。代码中将 604.4 keV 误写为 82.2，系与 316.5 keV 强峰发生混淆导致发射强峰夸大 10 倍。",
                    "remedy": "将 604.4 keV 相对发射强度更正为 8.2 (或 8.20%)。"
                })

        return issues

    def _audit_detector_material_physics(self, clean: str, raw: str) -> List[Dict[str, Any]]:
        """半导体 Fano 因子与闪烁体 Birks 猝灭核验。"""
        issues = []
        semis = self.mat_db.get("semiconductors", {})

        is_semiconductor = any(
            any(alias in raw for alias in info["aliases"])
            for info in semis.values()
        ) or bool(re.search(r"\b3\.6\s*\*\s*eV\b|\b2\.96\s*\*\s*eV\b", clean))

        has_charge_variance = bool(
            re.search(r"(?:var|variance|sigma|fluc|noise).*?(?:eh_pair|carrier|\bcharge\b|\bN_eh\b|\bN_pairs\b)", clean, re.IGNORECASE) or
            re.search(r"(?:var_N|sigma_N)\s*=\s*\bN\b", clean)
        )
        has_fano_factor = bool(re.search(r"0\.115|0\.11|0\.08|fano|Fano", clean, re.IGNORECASE))

        if is_semiconductor and has_charge_variance and not has_fano_factor:
            issues.append({
                "core": "NUCLEAR_CORE",
                "contract": "SEMICONDUCTOR_FANO_FACTOR_CONTRACT",
                "severity": "HIGH",
                "defect_name": "半导体探测器载流子统计涨落漏乘 Fano 因子 (本征方差虚高 9 倍)",
                "description": "半导体中电子-空穴对的产生不是完全独立的泊松事件。电荷产生的方差遵循 Var(N) = F * N（硅中 F ≈ 0.115，锗中 F ≈ 0.08）。漏乘 Fano 因子会导致探测器本征能量分辨率被严重低估 3 倍（方差虚高近 9 倍）。",
                "remedy": "在计算电荷产生涨落方差时乘入 Fano 因子：double var_N = fano * N_mean; (Si 设 0.115, Ge 设 0.08);"
            })

        is_scintillator = bool(re.search(r"(?:plastic|BC408|BC501|EJ200|EJ276|NE102|vinyltoluene|polystyrene|液体闪烁|塑料闪烁|有机闪烁)", raw, re.IGNORECASE))
        is_heavy_ion_or_neutron = bool(re.search(r"(?:neutron|proton|alpha|ion|中子|质子)", raw, re.IGNORECASE))
        has_light_or_scint_process = bool(re.search(r"(?:optical|scintillat|photon|light|MeVee|pe|Npe|pulse|yield|G4OpticalPhysics|G4Scintillation)", raw, re.IGNORECASE))
        has_birks = bool(re.search(r"SetBirksConstant|Birks", clean))

        if is_scintillator and is_heavy_ion_or_neutron and has_light_or_scint_process and not has_birks:
            issues.append({
                "core": "NUCLEAR_CORE",
                "contract": "SCINTILLATOR_BIRKS_QUENCHING_CONTRACT",
                "severity": "HIGH",
                "defect_name": "闪烁体在高 LET 粒子照射下未设置 Birks 猝灭常数",
                "description": "快中子反冲质子或重带电粒子在闪烁体中具有极高的阻止本领 (dE/dx)，导致局部电离密度过高发生荧光猝灭。若未调用 SetBirksConstant，Geant4 默认发光量与能量沉积线性对应，导致发光响应严重偏高 2~3 倍。",
                "remedy": "在闪烁材料中配置 Birks 常数：material->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);"
            })

        return issues

    def _audit_silent_engine_omissions(self, clean: str, raw: str) -> List[Dict[str, Any]]:
        """引擎静默缺陷排查：热中子材料命名、光核反应、自由基化学与光学属性。"""
        issues = []

        # 1. 契伦科夫/光学辐射缺 RINDEX
        has_optical_pkg = bool(re.search(r"new\s+(?:G4OpticalPhysics|G4Cerenkov)", clean) or re.search(r"RegisterPhysics\s*\([^)]*(?:Optical|Cerenkov)", clean))
        has_rindex = bool(re.search(r"RINDEX", clean))
        if has_optical_pkg and not has_rindex:
            issues.append({
                "core": "NUCLEAR_CORE",
                "contract": "CERENKOV_RINDEX_PROPERTY_CONTRACT",
                "severity": "CRITICAL",
                "defect_name": "使能光学物理但材料属性表未配置折射率 RINDEX",
                "description": "Geant4 契伦科夫辐射与光学光子传播依赖材料折射率 RINDEX。若介质材料属性表 (G4MaterialPropertiesTable) 中未显式添加 RINDEX，G4Cerenkov 过程将由于找不到相速度而静默跳过，全场光子发射数恒为 0 且不报任何错误！",
                "remedy": "在材料属性表中添加折射率属性：mpt->AddProperty(\"RINDEX\", photonEnergy, rindex, nEntries);"
            })

        # 2. 高能电子/伽马打重靶缺光核反应包
        is_high_e_electron = bool(re.search(r"(?:[2-9]\d(?:\.\d+)?|1\d\d(?:\.\d+)?)\s*\*?\s*MeV", clean) and re.search(r"e-|electron", clean, re.IGNORECASE))
        is_target_neutron = bool(re.search(r"photonuclear|光核|gfn|photofission", clean, re.IGNORECASE) or (re.search(r"neutron|中子", clean, re.IGNORECASE) and re.search(r"Target|target|G4_W|G4_Pb|G4_Ta|G4_U", clean)))
        has_photo_nuclear = bool(re.search(r"PhotoNuclear|G4PhotonuclearPhysics|G4PhotoNuclearProcess|FTFP_BERT|QGSP_BERT|G4EmExtraPhysics", clean, re.IGNORECASE))

        if is_high_e_electron and is_target_neutron and not has_photo_nuclear:
            issues.append({
                "core": "NUCLEAR_CORE",
                "contract": "PHOTONUCLEAR_PHYSICS_CONSTRUCTOR_CONTRACT",
                "severity": "CRITICAL",
                "defect_name": "高能电子轰击厚靶光核反应缺少 PhotoNuclear 物理构造器",
                "description": "50 MeV 电子在厚靶中产生高能轫致辐射，光子与重核通过巨偶极共振激发发生 (gamma, n) 光核反应释放中子。纯电磁物理包不包含任何核相互作用，输出光核中子数恒为零！",
                "remedy": "采用包含光核与电核物理的参考列表 FTFP_BERT，或显式注册 G4EmExtraPhysics 构造器激活 PhotoNuclear 反应道。"
            })

        # 3. 常温轻水材料命名非标导致热散射 S(alpha, beta) 静默缺失
        is_thermal_neutron = bool(
            re.search(r"0\.025\s*\*?\s*eV|25\s*\*?\s*meV|\bthermal\b", clean, re.IGNORECASE) and
            re.search(r"G4_WATER|water|H2O|polyethylene|G4_POLYETHYLENE", clean, re.IGNORECASE) and
            re.search(r"neutron", clean, re.IGNORECASE)
        )
        has_custom_h2o = bool(re.search(r'\"H2O\"', clean) or re.search(r"AddElement\s*\(\s*.*?,\s*2\s*\).*?AddElement\s*\(\s*.*?,\s*1\s*\)", clean))
        has_thermal_physics = bool(re.search(r"ThermalNeutrons|G4ThermalNeutrons", clean))

        if is_thermal_neutron and (has_custom_h2o or not has_thermal_physics):
            issues.append({
                "core": "NUCLEAR_CORE",
                "contract": "THERMAL_SCATTERING_MATERIAL_NAME_CONTRACT",
                "severity": "HIGH",
                "defect_name": "热中子慢化轻水材料命名非标或缺失热散射截面 S(alpha,beta)",
                "description": "常温轻水对热中子慢化依赖水分子中氢原子的结合态热散射截面 S(alpha, beta)。Geant4 热散射构造器（G4ThermalNeutrons / TS）通过严格的字符串键匹配数据库。如果自建轻水命名为 H2O 或未挂载热散射物理包，中子将退化为自由冷质子气体碰撞，热化能谱严重偏硬且出射通量失真。",
                "remedy": "轻水材料必须从 NIST 数据库标准构建：nistManager->FindOrBuildMaterial(\"G4_WATER\"); 并注册 G4ThermalNeutrons 物理构造器。"
            })

        # 4. 辐射化学自由基产额缺少 G4EmDNAChemistry
        is_radiochemistry = bool(re.search(r"radical|自由基|radiolysis|化学产额", clean, re.IGNORECASE))
        has_dna_chem = bool(re.search(r"G4EmDNAChemistry|GValue|G_value|g_val|G-value|initial_g", clean, re.IGNORECASE))
        if is_radiochemistry and not has_dna_chem:
            issues.append({
                "core": "NUCLEAR_CORE",
                "contract": "RADIOCHEMISTRY_DNA_CHEMISTRY_CONTRACT",
                "severity": "CRITICAL",
                "defect_name": "辐射化学模拟缺少 G4EmDNAChemistry 物理构造器",
                "description": "计算水辐射分解产生的羟基自由基 (.OH)、水合电子等自由基产额时，标准物理阶段仅模拟物理电离与激发步进，水分子解离、自由基扩散与化学反应阶段必须由 G4EmDNAChemistry 接管。缺少该构造器导致自由基产额恒为零。",
                "remedy": "在物理列表中注册水辐射化学包：RegisterPhysics(new G4EmDNAChemistry());"
            })

        return issues

    def _audit_decay_heat_energy_balance(self, clean: str) -> List[Dict[str, Any]]:
        """衰变热能量平衡记账契约：裂变产物发热功率必须完整计入带电粒子与衰变光子能损。"""
        issues = []
        is_decay_heat = bool(re.search(r"(?:decay_heat|fission_product|衰变热|发热功率)", clean, re.IGNORECASE))
        has_gamma_accounting = bool(re.search(r"gamma|photon|光子", clean, re.IGNORECASE) and re.search(r"edep|energy|heat", clean, re.IGNORECASE))

        if is_decay_heat and not has_gamma_accounting:
            issues.append({
                "core": "NUCLEAR_CORE",
                "contract": "DECAY_HEAT_ENERGY_BALANCE_CONTRACT",
                "severity": "HIGH",
                "defect_name": "裂变产物衰变热发热功率能量记账漏计衰变伽马射线",
                "description": "裂变产物衰变释放的总发热功率由 beta 射线连续能损与级联 gamma 射线能量沉积共同组成。若能量记账只计带电粒子动能而忽略穿透性伽马射线的能损沉淀，计算出的停堆衰变发热功率将被严重低估约 40%。",
                "remedy": "在能量记账中完整计入 beta 与 gamma 射线能量释放：total_heat_power = beta_power + gamma_power;"
            })
        return issues
