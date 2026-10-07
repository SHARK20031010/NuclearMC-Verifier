#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成第三批规则（PHYS-0031 ~ PHYS-0066 · 60题基准）。
全面覆盖 36 道扩展任务中的高阶输运、微剂量学、几何边界、探测器信号与衰变反应道陷阱。
"""
from pathlib import Path
import yaml

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "rules"

RULES_DATA = [
    # ── T1 扩展 (PHYS-0031 ~ PHYS-0036) ──────────────────────────────────
    ("PHYS-0031", "热中子散射截面库 (TSL) 与材料名规范", 4, "M1", "high", "violation",
     """source: code
present:
- G4ThermalNeutrons
- G4NeutronHPThermalScattering
checks:
- kind: always
  label: 涉及热中子材料散射库与命名规范""",
     "水或聚乙烯等含氢减速剂中，热中子输运必须注册 G4ThermalNeutrons 物理构造器，且材料名称必须使用 NIST 规范名（如 G4_WATER、G4_POLYETHYLENE）。若自建材料命名非标准或未挂载 S(a,b) 库，中子能谱将严重偏硬。",
     None,
     ["Geant4 Physics Reference Manual: Thermal Neutron Scattering", "NIST Material Database"],
     """## 现象与机理
热中子在常温轻水或聚乙烯中输运时，分子束缚态产生的热散射截面 S(α,β) 决定了热化能谱。若材料命名为自定义 H2O 或物理列表缺少 G4ThermalNeutrons，系统将回退至自由气体模型，导致热中子穿透通量偏硬且失真。"""),

    ("PHYS-0032", "方差缩减几何重要性分裂权重记账", 7, "M3", "high", "violation",
     """source: code
present:
- GetWeight
checks:
- kind: always
  label: 方差缩减计分是否累加粒子权重""",
     "在采用几何重要性分裂或权重窗等方差缩减技术时，计分器累加物理量必须乘粒子动态权重 GetWeight()。若直接统计粒子数，出射通量将被错误虚高数千倍。",
     None,
     ["Geant4 Book for Application Developers: Variance Reduction", "Monte Carlo Weight Window Principles"],
     """## 现象与机理
分裂产生的新粒子权重按分裂比减小（W = W0 / N）。计分时若只累加粒子个数 ++n 而未乘以 track->GetWeight()，深穿透通量将被严重高估。"""),

    ("PHYS-0033", "超薄靶低能光子次级电子产生截断", 4, "M5", "high", "violation",
     """source: code
present:
- SetCutValue
- SetCut
- mm
- um
checks:
- kind: always
  label: 超薄靶微米级产生截断设置""",
     "靶件厚度小于 1 mm 时（如 0.1 mm 铜箔），Geant4 默认的 0.7 mm 产生截断会导致低能光电效应次级电子无法作为独立粒子产生，其能量就地沉积，造成假穿透与透射电子数为零。",
     None,
     ["Geant4 Users Guide for Application Developers: Range Cuts"],
     """## 现象与机理
当 Range Cut 空间阈值大于物理几何尺寸时，次级电子被截断抑制，次级粒子输运特征完全丢失。必须显式将截断距离降低至靶厚的一半以下。"""),

    ("PHYS-0034", "多层同心几何共面重叠与卡步校验", 2, "M2", "high", "violation",
     """source: code
present:
- pSurfChk
- G4PVPlacement
checks:
- kind: always
  label: 检查同心几何边界重叠与重叠检测参数""",
     "多层同心圆柱或球体放置时，内层外边界与外层内边界若存在共面浮点重叠，会导致导航仪卡步死锁或丢失粒子。必须采用层级嵌套结构并开启 pSurfChk=true。",
     None,
     ["Geant4 Geometry Navigator: Overlapping Volumes"],
     """## 现象与机理
并列兄弟体在相同坐标放置且边界重合时，数值舍入可能导致步进点同时落在两个体中。推荐使用母子嵌套几何或保留微小安全间隙。"""),

    ("PHYS-0035", "大气宇宙线天顶角与动量角抽样", 5, "M4", "high", "violation",
     """source: code
present:
- cos
- theta
- ang
checks:
- kind: always
  label: 宇宙线源天顶角平方余弦角分布抽样""",
     "海平面大气宇宙线缪子天顶角严格遵循 cos^2(theta) 分布。将其简化为单向垂直平行束会低估斜向入射粒子的几何斜程吸收，导致地下深处探测通量虚高 2 倍以上。",
     None,
     ["Particle Data Group (PDG): Cosmic Rays in the Atmosphere"],
     """## 现象与机理
天顶角分布在深地输运中尤为关键，斜向粒子的等效穿透厚度为 d/cos(theta)，垂直束大大低估了总体衰减。"""),

    ("PHYS-0036", "穿墙圆柱管道直通流与反散射解耦", 7, "M12", "medium", "question",
     """source: code
present:
- duct
- uncollided
- scatter
checks:
- kind: always
  label: 管道出口通量直通项与散射项解耦计分""",
     "穿墙管道出口处中子通量包含直通准直分量与管壁反散射分量。需确认计分器是否解耦统计了弹道未碰撞粒子与管壁散射粒子。",
     "请确认计分器是否区分了未经碰撞的直通几何立体角流强与经过孔壁反散射的慢化流强？",
     ["NCRP Report No. 51: Radiation Protection Design for Particle Accelerator Facilities"],
     """## 现象与机理
管道屏蔽计算中，直通分量由几何立体角决定，散射分量由阿尔贝多反散射决定，混淆两者将无法进行正确的屏蔽结构优化。"""),

    # ── T2 扩展 (PHYS-0037 ~ PHYS-0042) ──────────────────────────────────
    ("PHYS-0037", "超短高剂量率脉冲时间归一化链条", 8, "M3", "high", "violation",
     """source: code
present:
- dose_rate
- pulse
- us
- second
checks:
- kind: always
  label: 脉冲瞬时剂量率时间归一化""",
     "高剂量率脉冲束轰击时，单脉冲瞬时峰值吸收剂量率分母必须除以脉冲持续时间 tau（如 5 us），严禁错误除以宏观重复周期（如 1 s），否则低估数十万倍。",
     None,
     ["FLASH Radiotherapy Beam Parameter Definitions: Instantaneous vs Average Dose Rate"],
     """## 现象与机理
FLASH 放疗和高功率脉冲辐射场中，瞬时剂量率与平均剂量率相差数万倍，生物效应直接取决于脉冲内的瞬时吸收剂量率。"""),

    ("PHYS-0038", "宽谱中子辐射权重因子 w_R(E) 积分", 8, "M12", "medium", "question",
     """source: code
present:
- w_R
- weight
- sievert
- dose_equiv
checks:
- kind: always
  label: 中子当量剂量连续辐射权重积分""",
     "中子当量剂量换算中，辐射权重因子 w_R(E) 随动能非线性剧烈变化。直接乘以固定常数（如 20）会导致中子当量剂量严重高估。",
     "你的代码计算中子当量剂量时，是将 w_R 设为了固定常数 20，还是根据中子能谱进行了连续积分加权？",
     ["ICRP Publication 103: The 2007 Recommendations of the International Commission on Radiological Protection"],
     """## 现象与机理
ICRP 103 规定热中子 w_R 仅为 2.5，高能中子为 5~10，只有 1 MeV 附近达到峰值 20。硬编码 20 属于过度保守且失真的算法。"""),

    ("PHYS-0039", "纳米尺度电离簇尺寸与宏观能损辨识", 7, "M12", "medium", "question",
     """source: code
present:
- cluster
- ionization
- edep
checks:
- kind: always
  label: 纳米靶区离散电离碰撞数统计""",
     "在 DNA 纳米尺度（~2 nm），放射生物损伤取决于单事件离散电离对数分布（ICSD），不能用宏观连续能量损耗 dE 除以平均电离能 W 近似。",
     "请确认纳米靶区内的观测量是单次电离事件的离散碰撞计数，还是连续能量沉积的经验折算？",
     ["ICRU Report 90: Nanodosimetry in Radiation Therapy"],
     """## 现象与机理
纳米尺度下单次穿过可能只有 0、1、2 个离散电离点，连续阻碍本领在微观纳米靶区不再成立。"""),

    ("PHYS-0040", "重金属纳米微颗粒界面微步长限制", 4, "M6", "high", "violation",
     """source: code
present:
- SetUserLimits
- G4StepLimiter
- UserLimits
checks:
- kind: always
  label: 纳米颗粒界面处设置微米/纳米级最大步长""",
     "重金属金颗粒与水介质交界面两侧光电截面突变数百倍。未在界面处挂载 G4StepLimiter 限制最大步长，会导致低能 Auger 电子跳过纳米界面，剂量增强比（DEF）计算严重失真。",
     None,
     ["Nanoparticle Radiation Enhancement: Micro-scale Step Size Effects"],
     """## 现象与机理
Geant4 标准凝聚算法默认单步推进距离在水中有数微米，而金表面出射的次级光电子和俄歇电子射程仅几十纳米，跳步会导致界面剂量峰丢失。"""),

    ("PHYS-0041", "旋转照射动量方向与源坐标同步变换", 5, "M4", "high", "violation",
     """source: code
present:
- cos
- sin
- SetParticleMomentumDirection
- SetParticlePosition
checks:
- kind: always
  label: 旋转源坐标变换与动量指向同步更新""",
     "模拟源绕中心旋转 360 度照射时，源位置 (x, y) 旋转的同时，粒子动量方向必须同步旋转并始终指向中心靶区 (-cos(theta), -sin(theta), 0)。动量方向保持常数会导致射线射向虚空。",
     None,
     ["Radiation Therapy Isocentric Gantry Source Modeling"],
     """## 现象与机理
源位置更新而未旋转动量方向向量是旋转放疗模型中最常见的一阶几何失误。"""),

    ("PHYS-0042", "高 LET 辐射微剂量学饱和修正因子", 8, "M12", "medium", "question",
     """source: code
present:
- y_sat
- saturation
- overkill
checks:
- kind: always
  label: 微剂量学高线能 y* 饱和修正""",
     "在重离子与质子布拉格峰末端，超高线能（y > 100 keV/um）会导致过量杀灭（Overkill）。生物有效剂量计算应引入饱和有效线能 y* 修正，直接线性外推会导致剂量高估 40%。",
     "请确认在布拉格峰末端计算 RBE 加权剂量时，是否考虑了微剂量学饱和参数 y* (y0 ~ 125 keV/um)？",
     ["Microdosimetric Biological Effect Modeling: The Dual Action Theory"],
     """## 现象与机理
当能量沉积密度超过细胞杀灭饱和阈值后，额外的电离能量被浪费在已失活细胞中，表现为生物学效应饱和。"""),

    # ── T3 扩展 (PHYS-0043 ~ PHYS-0048) ──────────────────────────────────
    ("PHYS-0043", "放射性衰变母子链 Bateman 方程与分支比", 8, "M12", "high", "question",
     """source: code
present:
- 0.87
- 0.88
- bateman
- lambda
checks:
- kind: always
  label: 母子衰变链 Bateman 方程与核素分支比核对""",
     "Mo-99 衰变至 Tc-99m 存在分支比（BR ≈ 87.5%），且子体活度演化必须采用二阶 Bateman 差分方程。当作 100% 分支比或单指数衰变会导致峰值活度与达峰时间严重失真。",
     "请确认代码中 Tc-99m 活度演化公式是否使用了含分支比（0.875）的 Bateman 二阶解？",
     ["NNDC Nuclear Decay Data: Mo-99 / Tc-99m Decay Chain"],
     """## 现象与机理
母体半衰期 66 小时，子体 6 小时，属于典型的暂态平衡母子链，单指数或漏乘分支比是该领域的经典失真点。"""),

    ("PHYS-0044", "快中子门槛反应 (n,2n) 截面与阈能衔接", 4, "M1", "high", "violation",
     """source: code
present:
- GetAtomicNumber
- GetAtomicMass
- Cu62
checks:
- kind: always
  label: 快中子门槛反应次级产物核精准识别""",
     "14 MeV 快中子打铜产生 Cu-62 属于门槛反应 (n,2n)（阈能约 10.9 MeV）。统计产物核时必须严格匹配原子序数与原子量（Z=29, A=62），严禁将散射退激的 Cu-63 混入活化产额。",
     None,
     ["ENDF/B-VIII.0: Cu-63(n,2n)Cu-62 Cross Section Evaluation"],
     """## 现象与机理
快中子非弹性散射产生的退激 Cu-63 核数量远大于 (n,2n) 产物，若未按质量数过滤，活化产物产额将被高估数十倍。"""),

    ("PHYS-0045", "高能电子/伽马光核反应中子发射物理", 4, "M12", "high", "violation",
     """source: code
present:
- G4PhotonuclearPhysics
- G4PhotoNuclearProcess
checks:
- kind: always
  label: 高能光子打靶物理列表注册光核物理""",
     "高能电子轰击厚靶产生轫致辐射并通过巨共振发生 (gamma, n) 光核反应放出中子。物理列表若未显式注册 G4PhotoNuclearPhysics，系统将完全无法产生光核中子，输出恒为 0。",
     None,
     ["Geant4 Physics Reference Manual: Photonuclear Reactions"],
     """## 现象与机理
标准电磁包 G4EmStandardPhysics 仅负责轻子和光子电磁作用，光核与轻子核物理属于独立构造器，必须显式挂载。"""),

    ("PHYS-0046", "循环流动介质活化停留时间与有效体积", 8, "M12", "medium", "question",
     """source: code
present:
- flow
- residence
- velocity
- tau
checks:
- kind: always
  label: 循环冷却水活化回路流体动力学停留时间计算""",
     "堆芯冷却水受中子活化（如 O-16(n,p)N-16）时，流体处于高速流动状态。有效辐照时间取决于单次通过堆芯的停留时间 t = L/v，且在堆外回路输运时快速衰变。不能套用静态靶长期辐照公式。",
     "请确认冷却水 N-16 比活度计算中是否计入了水流流速 v 决定的堆芯停留时间与回路衰变因子？",
     ["Nuclear Reactor Engineering: Reactor Coolant System Activation"],
     """## 现象与机理
N-16 半衰期仅 7.13 秒，流速每变化 10%，出口饱和活度产生显著非线性变化，静态公式导致出口活度偏差数倍。"""),

    ("PHYS-0047", "裂变产物衰变热发热功率能量记账", 7, "M12", "high", "question",
     """source: code
present:
- decay_heat
- gamma
- beta
checks:
- kind: always
  label: 裂变产物衰变热完整能量记账包含带电粒子与光子""",
     "裂变产物停堆后的衰变发热功率由带电粒子（主要是 beta 粒子及内转换电子）与延迟伽马射线共同贡献。记账时若只计 beta 能量而忽略伽马光子能损，发热功率将被低估约 40%。",
     "请确认衰变发热功率记账中是否同时包含了带电轻粒子自吸收与伽马辐射的能量沉积？",
     ["ANSI/ANS-5.1-2014: Decay Heat Power in Light Water Reactors"],
     """## 现象与机理
衰变初期的短寿命裂变产物释放大量穿透性很强的 1~3 MeV 伽马光子，必须结合输运模型计算几何自吸收比率。"""),

    ("PHYS-0048", "放射性体内滞留生物清除有效半衰期", 8, "M12", "medium", "question",
     """source: code
present:
- lambda_biol
- lambda_eff
- clearance
checks:
- kind: always
  label: 内照射生物清除代谢常数与物理衰变耦合""",
     "放射性核素在器官（如肺部）的滞留活度受物理衰变与生物代谢生理清除的双重控制：lambda_eff = lambda_p + lambda_b。若仅使用物理半衰期，累积滞留活度将被高估 5~8 倍。",
     "请确认代码计算器官累积活度时，是否计入了组织器官对该核素的生物代谢清除常数 lambda_b？",
     ["ICRP Publication 119: Compendium of Dose Coefficients based on ICRP Publication 60"],
     """## 现象与机理
许多核素的生物排泄速度远快于其物理半衰期（如氚水、某些放射性胶体），脱离生理清除的内照射模型是不完备的。"""),

    # ── T4 扩展 (PHYS-0049 ~ PHYS-0054) ──────────────────────────────────
    ("PHYS-0049", "探测器高斯能量展宽 FWHM 与标准差转换", 7, "M3", "high", "violation",
     """source: code
present:
- 2.355
- 2.3548
- fwhm
checks:
- kind: always
  label: 高斯展宽抽样标准差除以 2.355""",
     "探测器能量分辨率通常给出半高宽 FWHM。在调用高斯随机抽样展宽时，标准差必须输入 sigma = FWHM / 2.355。若直接传入 sigma = FWHM，全能峰宽将被放大 2.355 倍（方差大 5.5 倍）。",
     None,
     ["Knoll: Radiation Detection and Measurement (4th Ed), Peak Broadening Formula"],
     """## 现象与机理
高斯分布定义中 FWHM = 2 * sqrt(2*ln(2)) * sigma ≈ 2.35482 * sigma。直接将 FWHM 当作 sigma 是探测器仿真中的常发错误。"""),

    ("PHYS-0050", "探测器粒子飞行时间 TOF 步点时钟选取", 7, "M13", "high", "violation",
     """source: code
present:
- GetLocalTime
- GetGlobalTime
checks:
- kind: always
  label: 飞行时间测量读取全局时间 GetGlobalTime""",
     "在飞行时间法（TOF）测量中，记录从源点发射到探测器的时间必须调用 step->GetPostStepPoint()->GetGlobalTime()。误读 GetLocalTime() 将只返回当前径迹的局部生存时间，绝对飞行时间丢失。",
     None,
     ["Geant4 Track and Step Timing Classes: GlobalTime vs LocalTime vs ProperTime"],
     """## 现象与机理
粒子在空气或靶中发生任何次级重置或散射更新步时，LocalTime 会从 0 开始重新计时，只有 GlobalTime 忠实记录自初级粒子发射以来的绝对累积时间。"""),

    ("PHYS-0051", "闪烁晶体外表面光学光学边界属性", 2, "M12", "high", "violation",
     """source: code
present:
- G4OpticalSurface
- G4LogicalBorderSurface
- G4LogicalSkinSurface
checks:
- kind: always
  label: 光学输运晶体表面配置光学表面属性""",
     "闪烁晶体折射率高（~1.8），若出光面未配置 G4OpticalSurface 与反射/漫反射模型，光学光子在界面处会由于纯折射率全反射而被无限滞留在晶体内自吸收，出光效率降为 0。",
     None,
     ["Geant4 Optical Physics User Guide: Boundary Process and Optical Surfaces"],
     """## 现象与机理
Geant4 中光学光子穿过两个未定义 G4OpticalSurface 的逻辑体交界时，默认直接按 Snell 定律处理，晶体-空气界面全反射角极小，导致几乎无光逸出。"""),

    ("PHYS-0052", "半导体探测器电荷涨落 Fano 因子加权", 7, "M12", "medium", "question",
     """source: code
present:
- fano
- 0.115
- 0.11
checks:
- kind: always
  label: 半导体探测器电荷涨落引入 Fano 因子""",
     "硅/锗半导体探测器中产生电子空穴对的起伏受 Fano 因子抑制（硅中 F ≈ 0.115）。若使用纯泊松方差 sigma^2 = N，计算出的电荷本征涨落方差将虚高 9 倍，极限分辨严重恶化。",
     "请确认在计算电离电荷信号统计涨落时，方差是否乘以了半导体材料的 Fano 因子（~0.115）？",
     ["Knoll: Radiation Detection and Measurement, The Fano Factor in Semiconductors"],
     """## 现象与机理
能量沉积产生载流子的过程并不完全统计独立，部分能量转化为声子热振动，使得电离对涨落显著低于纯泊松统计。"""),

    ("PHYS-0053", "高计数率脉冲堆积与可麻痹死时间判据", 7, "M3", "high", "violation",
     """source: code
present:
- dead_time
- tau
- dt
checks:
- kind: always
  label: 探测器死时间相邻脉冲到达时间间隔判断""",
     "在高计数率下模拟死时间计数损失，必须记录相邻粒子到达探测器的时间戳并判断 delta_t < tau。未实现脉冲到达时间差判定会导致无论粒子到达多密集都不发生任何死时间损失。",
     None,
     ["Knoll: Radiation Detection and Measurement, Paralyzable and Nonparalyzable Dead Time Models"],
     """## 现象与机理
死时间堆积使得成形放大器输出波形重叠，丢失后续脉冲。模拟时必须结合全局到达时间戳实现麻痹/非麻痹筛选。"""),

    ("PHYS-0054", "介质材料折射率属性与契伦科夫辐射使能", 3, "M12", "high", "violation",
     """source: code
present:
- RINDEX
- MaterialPropertiesTable
- AddProperty
checks:
- kind: always
  label: 契伦科夫介质材料属性表中配置 RINDEX""",
     "触发契伦科夫辐射的前提是材料的 G4MaterialPropertiesTable 中必须注册折射率属性 RINDEX。即使在物理列表中注册了 G4Cerenkov 构造器，若介质无 RINDEX，产生的契伦科夫光子数恒为 0。",
     None,
     ["Geant4 User Guide: Cerenkov Process Prerequisites"],
     """## 现象与机理
G4Cerenkov 过程在步进时根据粒子的 beta 与材料当前的 RINDEX(E) 计算 cos(theta_c) = 1/(beta*n)，若未找到 RINDEX 属性表，该过程静默跳过发光。"""),

    # ── T5 扩展 (PHYS-0055 ~ PHYS-0060) ──────────────────────────────────
    ("PHYS-0055", "随机细胞群多敏感体积单事件击中解耦", 7, "M11", "high", "violation",
     """source: code
present:
- GetCopyNo
- copyNo
- map
checks:
- kind: always
  label: 细胞群多敏感体积按 CopyNo 独立解耦累加""",
     "在微观细胞群模拟中，统计各个细胞核被粒子径迹击中的泊松分布时，必须按各个微球的复制编号（CopyNo）分别独立累加沉积能。若使用全局单一累加器，会将整个细胞群混淆为单一靶区。",
     None,
     ["Microdosimetry and Cell Survival Modeling in Complex Geometries"],
     """## 现象与机理
多靶结构中单次事件只能击中空间中一部分敏感靶，全局单变量累加会导致靶间能量交叉污染，完全破坏了微观多靶泊松击中统计。"""),

    ("PHYS-0056", "BNCT 伴随粒子产生顶点对齐", 5, "M4", "high", "violation",
     """source: code
present:
- SetParticlePosition
- G4ThreeVector
checks:
- kind: always
  label: BNCT 伴随粒子 alpha 与 Li-7 共享同一反应顶点坐标""",
     "热中子硼中子俘获反应 B-10(n, alpha)Li-7 放出的 alpha 与 Li-7 属于同一反应点的伴随粒子。两粒子必须共享完全相同的局部初始空间坐标并呈背对背动量发射，严禁分别独立随机抽样发源。",
     None,
     ["IAEA-TECDOC-1223: Current Status of Neutron Capture Therapy"],
     """## 现象与机理
两体反应在核尺度同时发生，alpha 与锂核在单细胞尺度内的微观能量沉积关联极强，若分别随机发源，细胞自吸收能量关联性彻底丧失。"""),

    ("PHYS-0057", "纳米低能电子径迹结构法 (Track Structure)", 4, "M6", "medium", "violation",
     """source: code
present:
- G4EmDNAPhysics
- dna
- StepLimiter
checks:
- kind: always
  label: 低能电子纳剂量学启用微步长或 Geant4-DNA""",
     "10 keV 以下低能电子在组织中输运时，标准凝聚历史算法（Condensed History）若使用粗步长，会彻底抹平电子在射程末端的剧烈 Bragg 尖峰电离。应使用 Geant4-DNA 或超精细微步长限制。",
     None,
     ["Geant4-DNA: Simulation of Early DNA Damage by Low-energy Electrons"],
     """## 现象与机理
凝聚随机游走步长假设单步内发生多次微小弹性散射，但在电子慢化至数百电子伏时该假设破裂，必须采用分步离散相互作用模型。"""),

    ("PHYS-0058", "辐射生物学线性二次模型 (LQ) 单击/双击分量", 8, "M12", "medium", "question",
     """source: code
present:
- lq
- survival
- exp
checks:
- kind: always
  label: LQ 存活率模型宏观吸收剂量 D 与微观比能 z 辨识""",
     "线性二次模型（LQ）存活率公式 S = exp(-alpha*D - beta*D^2) 中的 D 是宏观靶区平均吸收剂量。若将单粒子单事件微观比能 z 直接代入该公式计算细胞存活率，会导致多次事件卷积效应脱落。",
     "请确认在评估细胞存活率时，是否严格区分了单事件微观比能谱 f1(z) 与宏观累积吸收剂量 D？",
     ["Radiobiology for the Radiologist (Hall & Giaccia), Linear-Quadratic Model"],
     """## 现象与机理
微观比能 z 在单径迹击中时起伏极大，宏观剂量 D 则是大量径迹的泊松叠加，直接代入微观量混淆了双击致死损伤的跨事件累积。"""),

    ("PHYS-0059", "放射性微球近距离放疗连续 Beta 能谱抽样", 5, "M4", "high", "violation",
     """source: code
present:
- beta
- fermi
- G4RadioactiveDecay
checks:
- kind: always
  label: Y-90 放射源连续 Beta 射线谱抽样""",
     "钇-90（Y-90）微球发射的是连续三体 Beta 衰变谱（最大能量 2.28 MeV，平均能量约 0.93 MeV）。将其简化为固定单能电子发源会导致微球近壁超高剂量率失真近 3 倍。",
     None,
     ["ICRU Report 72: Dosimetry of Beta Rays and Other Low-Energy Electrons"],
     """## 现象与机理
Beta 衰变中电子与反中微子分享能量，形成连续谱。单能电子发源抹杀了大量近程低能电子在界面附近的超大阻止本领沉积。"""),

    ("PHYS-0060", "辐射化学自由基扩散反应阶段注册", 4, "M12", "high", "violation",
     """source: code
present:
- G4EmDNAChemistry
- chemistry
- radical
checks:
- kind: always
  label: 水自由基产额模拟显式注册 G4EmDNAChemistry""",
     "水被质子照射产生水自由基（如 ·OH、eaq-）的化学产额（G 值）属于辐射化学扩散反应产物。物理列表中未注册 G4EmDNAChemistry 构造器时，输运结束后没有任何化学物种，产额全为 0。",
     None,
     ["Geant4-DNA: Water Radiolysis and Radical Diffusion Chemistry"],
     """## 现象与机理
物理输运步仅完成物理激发与电离；化学阶段（皮秒到微秒）才发生解离碰撞与自由基双分子反应扩散，化学包不可或缺。"""),

    # ── T6 扩展 (PHYS-0061 ~ PHYS-0066) ──────────────────────────────────
    ("PHYS-0061", "强偏转磁场最大弦长与弦积分器精度配置", 2, "M6", "high", "violation",
     """source: code
present:
- G4ChordFinder
- SetDeltaChord
- chord
checks:
- kind: always
  label: 磁场偏转弯管设置 G4ChordFinder 最大弦长""",
     "高能质子在强偏转磁场中发生圆弧弯转，Geant4 步进积分器采用弦线逼近圆弧。未配置 G4ChordFinder 限制最大弦偏差（SetDeltaChord）会导致大步长弦线直接穿透弯曲管壁飞跃虚空。",
     None,
     ["Geant4 User Guide: Magnetic Field Stepper and Chord Finder Configuration"],
     """## 现象与机理
弦积分器若允许的弦高差过大，数值步长在弯管处直接跨过弯曲边界，导致粒子发生非物理几何穿透与出射束斑位置严重偏离。"""),

    ("PHYS-0062", "放射性衰变延迟中子通道与前驱核寿命", 4, "M1", "medium", "violation",
     """source: code
present:
- delayed
- G4RadioactiveDecay
checks:
- kind: always
  label: 裂变产物衰变开启延迟中子发射通道""",
     "裂变产物 Br-87 衰变后伴随发射延迟中子。代码在配置放射性衰变模块时，若未使能延迟中子发射分支或排除了强子反冲核通道，会导致延迟中子产额被直接静默截断为 0。",
     None,
     ["Keepin: Physics of Nuclear Kinetics, Delayed Neutrons from Fission"],
     """## 现象与机理
延迟中子通道在标准衰变表中受半衰期阈值与反冲核动能截断影响，需显式检查衰变物理参数使能。"""),

    ("PHYS-0063", "裂变中子能谱 Watt 谱参数抽样解析公式", 5, "M4", "high", "violation",
     """source: code
present:
- watt
- sinh
- 0.988
- 2.249
checks:
- kind: always
  label: U-235 裂变中子谱标准 Watt 参数抽样""",
     "U-235 热裂变出射中子谱遵循标准 Watt 谱：p(E) ∝ exp(-E/a)sinh(sqrt(bE))，其中推荐参数为 a ≈ 0.988 MeV, b ≈ 2.249 MeV^-1。误用 Maxwell 谱或参数颠倒会导致平均动能严重偏离 ~2.0 MeV。",
     None,
     ["ENDF/B-VIII.0: Prompt Fission Neutron Spectra (Watt Spectrum)"],
     """## 现象与机理
Watt 谱在低能和高能尾部与纯麦克斯韦谱存在显著偏离，中子平均动能相差近 0.5 MeV，直接影响临界性与穿透能谱。"""),

    ("PHYS-0064", "初始高斯能展动能下限非负截断保护", 5, "M4", "high", "violation",
     """source: code
present:
- max
- 0.0
- G4RandGauss
checks:
- kind: always
  label: 高斯能展动能抽样后设置非负保护 Ek > 0""",
     "模拟具有高斯能展的粒子束时，高斯随机数抽样可能产生负动能尾巴。代码未设置非负动能截断保护（if (Ek <= 0) Ek = 1*eV），会导致粒子生成器接收负能量直接抛出致命异常或内核循环崩溃。",
     None,
     ["Geant4 Primary Generator Action: Kinetic Energy Bounds Check"],
     """## 现象与机理
物理粒子的动能严格非负。高斯分布理论上有负无穷大尾巴，在大能展抽样时必须强制截断在物理区间内。"""),

    ("PHYS-0065", "圆形扩展面源径向均匀抽样平方根法则", 5, "M4", "high", "violation",
     """source: code
present:
- sqrt
- G4UniformRand
- radius
checks:
- kind: always
  label: 圆形扩展面源半径抽样采用平方根 r = R * sqrt(xi)""",
     "在圆形面上均匀抽样粒子发射位置时，微元面积与半径关系为 dS = 2*pi*r*dr。半径抽样必须严格采用 r = R * sqrt(xi)。直接写为 r = R * xi 会导致中心粒子密度虚高数百倍，光斑虚假聚集。",
     None,
     ["Monte Carlo Sampling Methods: Uniform Sampling on Disk"],
     """## 现象与机理
直接均匀抽样半径导致单位圆环内的粒子数恒定，而外环面积大，致使中心面积微分处的粒子面密度发散（呈 1/r 奇点分布）。"""),

    ("PHYS-0066", "次级强子产生顶点空间密度单次累加", 7, "M11", "high", "violation",
     """source: code
present:
- GetCurrentStepNumber
- GetStepNumber
- 1
checks:
- kind: always
  label: 次级粒子空间产生密度仅在首步 GetCurrentStepNumber()==1 记录""",
     "统计次级粒子在靶内的空间产生顶点分布时，必须仅在粒子诞生的第一步（GetCurrentStepNumber() == 1）记录其物理坐标。若在所有步进中重复累加，深度密度分布将被成百上千倍严重虚高扭曲。",
     None,
     ["Geant4 Stepping Action: Primary vs Secondary Vertex Tracking"],
     """## 现象与机理
粒子产生后沿径迹可能推进成百上千个物理步，每个步点如果都被当作产生点记录，等于将径迹长度分布与产生顶点分布混淆。""")
]


def emit():
    n = 0
    for (rid, title, stage, mech, sev, out, trig, verdict, question, sources, body) in RULES_DATA:
        meta = {
            "id": rid,
            "title": title,
            "stage": stage,
            "mechanism": mech,
            "severity": sev,
            "output": out,
            "detect": "code" if "source: code" in trig else "snapshot",
            "applies_to": ["all"],
            "status": "implemented",
            "trigger": yaml.safe_load(trig),
            "verdict": verdict.strip() + "\n",
        }
        if question:
            meta["question"] = question.strip() + "\n"
        meta["sources"] = sources
        meta["authors"] = ["待填"]
        meta["reviewed_by"] = []
        
        fm = yaml.safe_dump(meta, allow_unicode=True, sort_keys=False, width=1000)
        target_file = OUT / f"{rid}.md"
        target_file.write_text(f"---\n{fm}---\n\n{body}\n", encoding="utf-8")
        n += 1
    print(f"✓ 成功写出第三批 {n} 条新规则文件到 {OUT}/ (PHYS-0031 ~ PHYS-0066)")


if __name__ == "__main__":
    emit()
