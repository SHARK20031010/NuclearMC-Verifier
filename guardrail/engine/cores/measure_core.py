#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
measure_core.py —— 相空间测度与随机抽样守恒总线 (Phase Space & Measure Core)
覆盖微分几何流形上的测度守恒、雅可比行列式、角动量立体角抽样、微剂量平均弦长与径迹弯转积分。
"""

import re
from typing import Dict, List, Any


def strip_cpp_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


class MeasureCore:
    """相空间测度总线：拦截点源退化平行束、圆盘抽样未开方、高斯展宽混淆 FWHM、微剂量平均弦长错位等测度畸变。"""

    def audit(self, code_text: str) -> List[Dict[str, Any]]:
        clean = strip_cpp_comments(code_text)
        issues = []
        issues.extend(self._check_point_source_isotropy(clean))
        issues.extend(self._check_disk_area_sampling(clean))
        issues.extend(self._check_gaussian_broadening_fwhm(clean))
        issues.extend(self._check_cosmic_ray_zenith_measure(clean))
        issues.extend(self._check_energy_spread_nonnegative_guard(clean))
        issues.extend(self._check_microdosimetry_mean_chord(clean))
        issues.extend(self._check_rotational_source_alignment(clean))
        issues.extend(self._check_magnetic_chord_finder(clean))
        issues.extend(self._check_beam_flattening_and_spatial_profile(clean))
        return issues

    def _check_point_source_isotropy(self, clean: str) -> List[Dict[str, Any]]:
        """立体角测度守恒：点辐射源必须在 4pi 立体角内按 dOmega = sin(theta) dtheta dphi 各向同性抽样。"""
        issues = []
        is_point_source = bool(re.search(r"(?:point|isotrop|点源|各向同性|NaI|HPGe).*?(?:source|放射源|gun|射源)", clean, re.IGNORECASE))
        has_fixed_dir = bool(re.search(r"SetParticleMomentumDirection\s*\(\s*(?:G4ThreeVector)?\s*\(\s*0\s*,\s*0\s*,\s*1\s*\)\s*\)", clean))
        has_iso_sampling = bool(re.search(r"G4RandomDirection|cosTheta|sinTheta|ang/type\s*iso", clean, re.IGNORECASE))

        if is_point_source and has_fixed_dir and not has_iso_sampling:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "POINT_SOURCE_ISOTROPY_CONTRACT",
                "severity": "CRITICAL",
                "defect_name": "点辐射源发射方向退化为固定单向平行束 (0,0,1)",
                "description": "代码中模拟点放射源，但粒子发射动量方向被硬编码为固定的垂直单向向量 (0,0,1)。在真实物理中，点源在全空间 (4pi) 各向同性发射，固定单向束会导致探测器立体角人为放大数十倍，峰效率超几何物理上限。",
                "remedy": "改用 4pi 各向同性立体角抽样：gun->SetParticleMomentumDirection(G4RandomDirection());"
            })
        return issues

    def _check_disk_area_sampling(self, clean: str) -> List[Dict[str, Any]]:
        """面积测度守恒：圆形/圆盘面源的极坐标几何测度为 r dr dtheta，半径抽样必须开平方 r = R * sqrt(xi)。"""
        issues = []
        has_linear_r = bool(re.search(r"(?:r|radius)\s*=\s*(?:R|radius|fRadius|5\.0\s*\*?\s*cm)\s*\*\s*(?:G4UniformRand\(\)|drand48\(\)|rand\(\)|xi)", clean))
        has_sqrt_r = bool(re.search(r"(?:std::)?sqrt\s*\(", clean))

        if has_linear_r and not has_sqrt_r:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "DISK_AREA_SAMPLING_CONTRACT",
                "severity": "HIGH",
                "defect_name": "圆形扩展面源半径抽样未开平方导致中心聚拢",
                "description": "极坐标微元面积为 dA = r dr dtheta。对平坦均匀圆形面源抽样时，若直接使用线性随机数 r = R * xi，会导致中心处产生概率密度呈现 1/r 奇点聚拢（中心虚假过密、边缘稀疏）。",
                "remedy": "根据雅可比反变换引入平方根测度映射：G4double r = R * std::sqrt(G4UniformRand());"
            })
        return issues

    def _check_gaussian_broadening_fwhm(self, clean: str) -> List[Dict[str, Any]]:
        """高斯正态分布数学物理契约：半高全宽 FWHM 与抽样标准差 sigma 满足 FWHM = 2.35482 * sigma。"""
        issues = []
        has_broadening = bool(re.search(r"(?:G4RandGauss|CLHEP::RandGauss).*?(?:shoot|shootEngine)", clean))
        has_fwhm_str = bool(re.search(r"(?:fwhm|resolution|半高全宽|展宽)", clean, re.IGNORECASE))
        has_2355 = bool(re.search(r"2\.35[45]|2\.35", clean))

        if has_broadening and has_fwhm_str and not has_2355:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "GAUSSIAN_BROADENING_FWHM_CONTRACT",
                "severity": "HIGH",
                "defect_name": "高斯能量展宽混淆 FWHM 与标准差 sigma",
                "description": "在对探测器能量沉积进行高斯仪器展宽时，直接将半高宽 (FWHM) 作为高斯抽样的标准差 sigma 代入。正态分布中 FWHM = 2*sqrt(2*ln2)*sigma ≈ 2.355 * sigma。混淆两者会导致模拟出的能峰被虚假过度展宽 2.355 倍，严重破坏能量分辨率比对。",
                "remedy": "将 FWHM 正确换算为抽样标准差：G4double sigma = fwhm / 2.355; G4double e_smeared = G4RandGauss::shoot(edep, sigma);"
            })
        return issues

    def _check_cosmic_ray_zenith_measure(self, clean: str) -> List[Dict[str, Any]]:
        """宇宙射线相空间测度契约：海平面/地下缪子天顶角分布正比于 cos^2(theta)。"""
        issues = []
        is_cosmic = bool(re.search(r"(?:cosmic|muon|宇宙线|缪子)", clean, re.IGNORECASE))
        has_zenith_cos2 = bool(re.search(r"pow\s*\(\s*cos|cos.*?2|cosTheta", clean, re.IGNORECASE) or "/gps/ang/type cos" in clean)
        has_vertical_only = bool(re.search(r"SetParticleMomentumDirection\s*\(\s*G4ThreeVector\s*\(\s*0\s*,\s*0\s*,\s*-\s*1\s*\)\s*\)", clean))

        if is_cosmic and has_vertical_only and not has_zenith_cos2:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "COSMIC_RAY_ZENITH_CONTRACT",
                "severity": "HIGH",
                "defect_name": "宇宙线缪子源天顶角分布退化为垂直束",
                "description": "海平面与地下宇宙线缪子通量具有强烈的天顶角依赖性，在低天顶角时严格遵循 dN/dOmega ∝ cos^2(theta)。退化为固定垂直束将导致倾斜穿透岩石层的几何路程被低估，漏记大角度穿透粒子。",
                "remedy": "对天顶角引入 cos^2(theta) 接受拒绝抽样或使用逆变换法抽样天顶角 theta。"
            })
        return issues

    def _check_energy_spread_nonnegative_guard(self, clean: str) -> List[Dict[str, Any]]:
        """相空间物理边界契约：高斯能展动能抽样必须包含非负截断 Ek > 0 保护。"""
        issues = []
        has_gauss_energy = bool(re.search(r"(?:G4RandGauss|RandGauss).*?(?:energy|Ek|momentum)", clean, re.IGNORECASE))
        has_truncation = bool(re.search(r"(?:Ek\s*>\s*0|energy\s*>\s*0|std::max\s*\(\s*0|while\s*\(\s*E\s*<\s*0)", clean, re.IGNORECASE))

        if has_gauss_energy and not has_truncation:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "ENERGY_SPREAD_NONNEGATIVE_CONTRACT",
                "severity": "MEDIUM",
                "defect_name": "束流高斯能展动能抽样缺少非负物理截断",
                "description": "高斯正态分布的定义域为 (-inf, +inf)。当束流能展较大时，高斯尾巴存在极小概率抽中负能量 (Ek < 0)，导致粒子运动学与动量计算崩溃。",
                "remedy": "添加动能非负物理截断循环：do { Ek = G4RandGauss::shoot(meanE, sigmaE); } while (Ek <= 0.0);"
            })
        return issues

    def _check_microdosimetry_mean_chord(self, clean: str) -> List[Dict[str, Any]]:
        """微剂量线能测度契约：微球腔平均弦长满足 Cauchy 定理 l_bar = 4V/S = 4r/3 ≈ 0.667 d。"""
        issues = []
        is_microdosimetry = bool(re.search(r"(?:tepc|lineal|microdosim|线能)", clean, re.IGNORECASE))
        # 检查分母是否错误使用微球直径 d (如 1.0*um) 而非平均弦长 4r/3
        has_diameter_chord = bool(re.search(r"/\s*(?:d|diam|1\.0\s*\*?\s*um|2\.0\s*\*?\s*r)\b", clean) and 
                                 not re.search(r"0\.66[67]|4\.0\s*/\s*3\.0|4\s*\*\s*r\s*/\s*3|meanChord", clean))

        if is_microdosimetry and has_diameter_chord:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "MICRODOSIMETRY_MEAN_CHORD_CONTRACT",
                "severity": "HIGH",
                "defect_name": "微剂量学线能 y 计算分母误用直径而非平均弦长 4r/3",
                "description": "ICRU 36 规定微剂量线能定义为单事件能量沉积除以微敏感体积的平均弦长 l_mean。对于凸体几何，柯西平均弦长定理给出 l_bar = 4V/S。对微球腔，l_bar = 4r/3 = 2d/3 ≈ 0.667 d。误将球体直径代入分母会导致计算出的微剂量线能谱 y 与饱和剂量严重低估 1.5 倍。",
                "remedy": "将线能分母换算为球体平均弦长：G4double meanChord = 4.0 * radius / 3.0; G4double y = edep / meanChord;"
            })
        return issues

    def _check_rotational_source_alignment(self, clean: str) -> List[Dict[str, Any]]:
        """旋转粒子源相空间动量对准契约：源空间位置旋转时动量方向必须同步旋转。"""
        issues = []
        has_rotation_pos = bool(re.search(r"(?:cos\s*\(|sin\s*\().*?(?:pos|SetParticlePosition)", clean))
        has_fixed_dir = bool(re.search(r"SetParticleMomentumDirection\s*\(\s*(?:G4ThreeVector)?\s*\(\s*0\s*,\s*0\s*,\s*1\s*\)\s*\)", clean))

        if has_rotation_pos and has_fixed_dir:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "ROTATIONAL_SOURCE_ALIGNMENT_CONTRACT",
                "severity": "HIGH",
                "defect_name": "旋转照射源空间位置旋转但发射动量方向未同步旋转",
                "description": "在模拟等中心旋转放疗或机架旋转照射时，源位置沿圆周旋转（x = R*cos(theta), y = R*sin(theta)），但粒子发射动量方向仍固定为常数 (0,0,1)。这导致旋转到侧面和后方的光子全部射向外部虚空，未能对准中心靶区。",
                "remedy": "使动量方向始终对准中心原点：G4ThreeVector dir = (-pos).unit(); gun->SetParticleMomentumDirection(dir);"
            })
        return issues

    def _check_magnetic_chord_finder(self, clean: str) -> List[Dict[str, Any]]:
        """磁场轨道数值积分步长契约：强偏转磁场必须显式设置 G4ChordFinder 最大弦长。"""
        issues = []
        is_magnetic = bool(re.search(r"G4UniformMagField|G4FieldManager", clean))
        has_chord = bool(re.search(r"ChordFinder|SetDeltaChord", clean))

        if is_magnetic and not has_chord:
            issues.append({
                "core": "MEASURE_CORE",
                "contract": "MAGNETIC_CHORD_STEP_INTEGRATION_CONTRACT",
                "severity": "HIGH",
                "defect_name": "强偏转磁场未配置 G4ChordFinder 步进弦长限制",
                "description": "在弯转磁场或偶极磁铁中，粒子沿圆弧轨道偏转。Geant4 默认步进器最大弦长容差较大，在强磁场小偏转半径下会导致粒子单步飞跃弯管边界直接穿入真空壁丢弃。",
                "remedy": "显式配置磁场步进器最大容差：fieldMgr->GetChordFinder()->SetDeltaChord(0.1 * mm);"
            })
        return issues

    def _check_beam_flattening_and_spatial_profile(self, clean: str) -> List[Dict[str, Any]]:
        """相空间束流均整与空间剖面契约：加速器韧致辐射照射水箱需包含均整过滤与空间剖面分箱。"""
        issues = []
        is_linac_target = bool(
            (re.search(r"\b(?:Target|target|W_Target|w_target)\b", clean) and re.search(r"\b(?:G4_W|G4_Ta|tungsten|tantalum)\b", clean, re.IGNORECASE)) and
            re.search(r"(?:e-|electron)", clean, re.IGNORECASE)
        )
        has_water_phantom = bool(re.search(r"(?:Water|Phantom|water|phantom)", clean))
        has_flattening = bool(re.search(r"(?:Flattening|flattening|G4Cons|Filter|filter)", clean))
        has_spatial_binning = bool(re.search(r"(?:PDD|pdd|Profile|profile|zBin|xBin|depth|flatness)", clean, re.IGNORECASE))

        if is_linac_target and has_water_phantom:
            if not has_flattening:
                issues.append({
                    "core": "MEASURE_CORE",
                    "contract": "BEAM_FLATTENING_FILTER_CONTRACT",
                    "severity": "HIGH",
                    "defect_name": "加速器韧致辐射水箱照射缺少锥形均整板 (Flattening Filter)",
                    "description": "电子轰击重金属靶产生的高能轫致辐射角分布具有强烈的前向尖峰特性。若直接照射大水箱而未引入圆锥形均整器（如 G4Cons），横向剂量分布中心严重过热凸起，无法获得临床放疗所需的均匀平坦射野。",
                    "remedy": "在靶后放置圆锥形均整器 (G4Cons) 对前向中心高光子通量进行吸收整形：new G4Cons(\"Filter\", 0, 0, 0, 3*cm, 1.5*cm, 0, 360*deg);"
                })
            if not has_spatial_binning:
                issues.append({
                    "core": "MEASURE_CORE",
                    "contract": "SPATIAL_DOSE_PROFILE_BINNING_CONTRACT",
                    "severity": "HIGH",
                    "defect_name": "宏观放疗水箱剂量退化为单一标量累加（缺少 PDD 深度与横向 Profile 分箱）",
                    "description": "代码在宏观水箱中仅通过单个浮点标量累加总沉积能量，丢失了沿深度衰减的百分深度剂量 (PDD) 与离轴横向剂量展宽 (Lateral Profile) 的关键空间特征。",
                    "remedy": "对水箱沿 Z 轴（深度）与 X 轴（离轴）进行网格化分箱累加：int zBin = (z + halfZ) / dz; pdd[zBin] += edep;"
                })
        return issues

