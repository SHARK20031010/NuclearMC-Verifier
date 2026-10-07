# 针对大语言模型生成粒子输运蒙卡程序的形式化核验器

### Formal Verifier for LLM-Generated Monte Carlo Particle Transport Codes

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.placeholder.svg)](https://doi.org/10.5281/zenodo.placeholder)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Geant4](https://img.shields.io/badge/Geant4-11.2.2-brightgreen.svg)](https://geant4.web.cern.ch/)
[![Python](https://img.shields.io/badge/Python-3.9+-blue.svg)](https://www.python.org/)

**面向大语言模型（LLM）生成辐射与粒子输运蒙特卡罗代码的形式化验证与确认（Formal V&V）核验器及 190 题分级评测基准**

---

## 目录

- [一、项目背景与核心贡献](#一项目背景与核心贡献)
- [二、全量 190 题三臂盲测实证结论](#二全量-190-题三臂盲测实证结论)
- [三、核心架构与物理守恒机制](#三核心架构与物理守恒机制)
  - [1. 核心操作空间（定、抽、取、记、换）与 31 维规约违规槽位](#1-核心操作空间定抽取记换与-31-维规约违规槽位)
  - [2. 输入规约契约前置对齐机制（Pre-condition Specification Contract）](#2-输入规约契约前置对齐机制pre-condition-specification-contract)
  - [3. 五大正交物理守恒不变式体系（Orthogonal Conservation Invariants）](#3-五大正交物理守恒不变式体系orthogonal-conservation-invariants)
- [四、项目资产与目录结构](#四项目资产与目录结构)
- [五、快速上手与基准复现指南](#五快速上手与基准复现指南)
  - [1. 环境配置](#1-环境配置)
  - [2. 运行单题形式化核验与规约审计](#2-运行单题形式化核验与规约审计)
  - [3. 全量基准评测流水线复现](#3-全量基准评测流水线复现)
- [六、论文手稿与衍生资料](#六论文手稿与衍生资料)
- [七、开源许可与引用规范](#七开源许可与引用规范)

---

## 一、项目背景与核心贡献

在核工程、辐射防护与高能物理仿真领域，蒙特卡罗粒子输运模拟（如 Geant4、OpenMC）是安全攸关（Safety-Critical）的核心计算基座。大语言模型（LLM）在编写科学计算代码时表现出极高的表面语法合规性，但频繁产生**静默物理违约（Silent Physical Violations）**与**伪收敛（Pseudo-Convergence）**——即代码编译通过、运行退出码为 0，但由于微观反应截面缺位、几何死层重叠或深穿透减方差权重漂移，导致物理量估算出现数个数量级的隐蔽失真。

针对这一本质困难，本项目构建了**针对大语言模型生成粒子输运蒙卡程序的形式化核验器（Formal Verifier）**。核心贡献包括：

1. **190 题全生命周期分级评测基准集**：构建覆盖几何实体、介质截面、粒子源项、物理过程、方差缩减与计分控制的递进式基准（Tier 1 基础单槽位 60 题、Tier 2 跨链中等耦合 60 题、Tier 3 极限深穿透与动力学 60 题、Real-World Wild Corpus 真实社区长尾漏洞 10 题）。
2. **生成性物理错误空间的形式化闭环**：确立“五种基本操作（定、抽、取、记、换）× 31 个规约违规槽位”，将不可穷举的工程经验转化为数学完备的缺陷状态机空间。
3. **输入规约契约前置对齐机制**：在上游消除自然语言的 10 类隐式意图与量纲歧义，构建严格的前置规约确认凭证。
4. **五大正交物理守恒不变式体系**：在下游对生成代码施加能量、守恒荷数、相空间测度、输运统计无偏性与量纲标度的刚性第一性原理约束，彻底阻断模型在复杂工况下的经验常数拟合欺骗。

---

## 二、全量 190 题三臂盲测实证结论

本基准对 190 道题目执行了严格的隔离式三臂独立盲测实验（3-Arm Blind Experiment）：
- **Arm A (Raw Baseline)**：无约束直接生成基线（反映前沿通用大模型在纯提示词下的单轮编写能力）。
- **Arm B (Self-Reflection / CoT)**：提示模型执行多轮逻辑自省与反思重试（思维链自我修正能力）。
- **Arm C (Ours: Formal Verifier)**：施加输入规约契约与第一性原理守恒不变式形式化核验。

### 核心评测数据

| 评测梯级 (Task Tier) | 题目规模 (N) | Arm A (原始基线) | Arm B (思维链自省) | Arm C (本形式化核验器) | 核心失效特征 |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Tier 1 (基础单槽位)** | 60 | 25 / 60 (41.7%) | 59 / 60 (98.3%) | **60 / 60 (100.0%)** | 通用自省可修复大部分语法与表面参数定义缺陷 |
| **Tier 2 (跨链中等耦合)** | 60 | 0 / 60 (0.0%) | 22 / 60 (36.7%) | **60 / 60 (100.0%)** | 几何边界与反应态跨链耦合导致未受控模型全部失效 |
| **Tier 3 (极限深穿透/动力学)** | 60 | 0 / 60 (0.0%) | 32 / 60 (53.3%) | **60 / 60 (100.0%)** | 模型陷入认知死锁；自省诱发 16.7% 的常数拟合伪收敛 |
| **Real-World Wild Corpus** | 10 | 0 / 10 (0.0%) | 0 / 10 (0.0%) | **10 / 10 (100.0%)** | 源自 CERN 官方论坛与 GitHub 真实长尾静默缺陷 |
| **全量微观平均 (Overall)** | **190** | **25 / 190 (13.2%)** | **113 / 190 (59.5%)** | **190 / 190 (100.0%)** | **通用自省收益 +46.3%，形式化核验净增益 +40.5%** |

![评测准确率梯度对比](figures/fig1_accuracy_gradient.png)

---

## 三、核心架构与物理守恒机制

### 1. 核心操作空间（定、抽、取、记、换）与 31 维规约违规槽位

蒙特卡罗输运程序中任何物理数据流操作，在形式化语义上严格受限于五种基本计算动作及其固有违规维度：

| 基本操作 | 形式化语义 (Formal Semantics) | 规约违规槽位 (31 个) | 典型物理违规表现 |
| :--- | :--- | :--- | :--- |
| **定 (Define)** | **实体绑定与注册**：几何、材料、微观截面与初值绑定 | 有无、重复、身份、取值 (4) | 漏注册热中子高精度 $S(\alpha,\beta)$ 截面；漏设有机闪烁体 Birks 猝灭 |
| **抽 (Sample)** | **测度抽样与相空间分布**：粒子源相空间分布抽样 | 变量、定义域、形状、参数、实现、相关性 (6) | 各向同性立体角退化为笛卡尔平面抽样；漏抽核素双能谱共存衰变 |
| **取 (Fetch)** | **状态提取与步进探针**：运行时径迹状态与微观量解构 | 谁、何时、哪里、哪个字段、存在性 (5) | 步后读取初始动能分错能谱箱；步进中查询未生成的次级微观态 |
| **记 (Accumulate)** | **物理量积分与箱元累加**：相空间网格积分与通量计数 | 记什么、在哪记、何时记、权重、分箱、聚合 (6) | 径迹长度通量误累加为粒子数；减方差技巧中漏乘统计权重 |
| **换 (Convert)** | **量纲变换与标度映射**：单位换算与绝对活度标度 | 分母、单位链、响应系数、归一化基准、误差 (5) | 活度公式误除衰变常数致结果失真 $10^7$ 倍；Gy 与 Sv 混用 |
| **控制流轴** | **动作时序与拓扑序**：生命周期循环与事件拓扑 | 层级、次数、条件、顺序 (4) | Run 级容器未按 Event 及时清空导致跨事件残留计数污染 (Inter-Event Stale Tally Accumulation) |

![失效槽位分布](figures/fig2_action_slots_distribution.png)

### 2. 输入规约契约前置对齐机制（Pre-condition Specification Contract）

针对粒子输运前向传递中未被自然语言显式声明的隐式假设，核验器在代码生成前施加严格的前置规约契约：
- **F1a/F1b**：观测量物理量种类（通量/剂量/活度/能谱）与物理作用域闭环枚举；
- **F2/F3**：物理量精确定义（吸收剂量 vs 剂量当量）与量纲单位；
- **F4/F5**：统计空间尺度（体/面平均）与时间演化窗口；
- **F6/F7**：归一化基准（每源粒子 vs 束流强度）与统计相对误差限（1σ）；
- **F8/F9/F10**：收敛判据、放射源活度与多通道数据输出规范。

### 3. 五大正交物理守恒不变式体系（Orthogonal Conservation Invariants）

为避免经验性规则的局限性，核验器下游基于第一性原理实施五大正交守恒不变式检验：
1. **能量守恒不变式 (Energy Conservation)**：$\sum E_{\text{in}} = \sum E_{\text{dep}} + \sum E_{\text{escape}} + \Delta Q$。
2. **守恒荷数不变式 (Conserved Quantum Numbers)**：轻子数、重子数、电荷数在微观相互作用中严格守恒。
3. **相空间与角分布测度不变式 (Phase-Space & Angular Symmetry)**：各向同性立体角测度微元 $d\Omega = \sin\theta d\theta d\phi$，禁止退化为一维直抽。
4. **输运统计无偏性与权重不变式 (Fair-Game Weight & Variance)**：遵循“公平游戏”输运测度，禁止无权重补偿的分裂与轮盘赌。
5. **量纲链与标度线性不变式 (Dimensional Linearity & Scaling)**：全链路物理量纲严格跟踪，确保分母一致性与线性响应正确。

---

## 四、项目资产与目录结构

```
.
├── LICENSE                                     # Apache License 2.0 开源许可证
├── CITATION.cff                                # GitHub / Zenodo 标准化引用元数据
├── requirements.txt                            # Python 运行依赖
├── README.md                                   # 项目全景说明与快速指南
├── paper_manuscript.md                         # 论文完整英文学术手稿 (已对齐 190 题最新数据)
├── AI蒙卡程序物理正确率与防错体系_综合总案.md       # 理论全景长篇技术总案
├── figures/                                    # 高清矢量图与位图资产 (fig1, fig2)
├── guardrail/                                  # 形式化核验器规则库与 AST 审计引擎
│   ├── rules/                                  # 30 条形式化物理守恒规则 (PHYS-0001 ~ PHYS-0030)
│   ├── engine/                                 # 意图推理核心、静态代码 AST 扫描与通用分析引擎
│   ├── check.py                                # 在线规则与代码审计执行器
│   └── build.py                                # 规则编译器与倒排索引构建器
├── 测量方案/                                   # 190 题全量评测套件与独立实验工件 (软链接 benchmarks -> 测量方案)
│   ├── tasks/                                  # Tier 1 基础单槽位 60 题定义
│   ├── benchmark_tier2/                        # Tier 2 跨链中等耦合 60 题 (提示词、代码、JSON 审计)
│   ├── benchmark_tier3/                        # Tier 3 极限深穿透与动力学 60 题 (提示词、代码、JSON 审计)
│   ├── real_world_wild_corpus/                 # 真实社区长尾漏洞 10 题 (CERN 论坛与 GitHub 漏洞)
│   └── results/                                # 三臂盲测判决汇总 JSON 与统计脚本
├── benchmark_tasks_reference/                  # 190 题参考库 (每题单独文件夹：题目要求、提示词、对比结果与代码)
├── verify/                                     # Geant4 源码级实测验证探针套件与 6 篇领域核验报告
├── g4dump/                                     # C++ 运行时配置 Hook 拦截与快照导出探针
└── docs/theory/                                # 理论推导稿 (规约契约、链条映射、自查表等)
```

---

## 五、快速上手与基准复现指南

### 1. 环境配置

本项目需要 Python 3.9+ 环境。若需要重新编译运行 C++ Geant4 代码，建议预装 Geant4 11.2+。

```bash
# 安装基础依赖
pip install -r requirements.txt
```

### 2. 运行单题形式化核验与规约审计

对特定的 Geant4 模拟配置快照或 C++ 源码运行形式化核验：

```bash
# 编译规则索引
python3 guardrail/build.py

# 对指定输运代码执行物理规约核验
python3 guardrail/check.py --code 测量方案/benchmark_tier3/code_C_isolated/T3-01.cc
```

### 3. 全量基准评测流水线复现

```bash
# 1. 运行 Tier 1 实验结果聚合与判决
python3 测量方案/analyze.py

# 2. 执行 Tier 3 极限深穿透规约闭环运行流水线
python3 测量方案/benchmark_tier3/execute_tier3_ring0.py

# 3. 统计全量三臂盲测对比报告
python3 测量方案/verdict_report.py
```

---

## 六、论文手稿与衍生资料

- 完整学术论文手稿详见 [`paper_manuscript.md`](paper_manuscript.md)。
- 理论与源码级核验全景报告详见 [`AI蒙卡程序物理正确率与防错体系_综合总案.md`](AI蒙卡程序物理正确率与防错体系_综合总案.md)。
- 6 篇 Geant4 源码核验实测报告归档于 [`verify/`](verify/)。

---

## 七、开源许可与引用规范

本项目采用 **Apache License 2.0** 开源许可证，完整条款参见 [LICENSE](LICENSE)。

如果您在学术研究或工程实践中使用了本仓库的基准集、测试用例或形式化核验方法，请引用我们的 Zenodo 永久归档：

```bibtex
@software{mc_formal_verifier_2026,
  author       = {{MC-AI Safety Working Group}},
  title        = {{A Formal Verifier for Large Language Model-Generated Monte Carlo Particle Transport Codes}},
  year         = {2026},
  publisher    = {Zenodo},
  version      = {v1.0.0},
  doi          = {10.5281/zenodo.placeholder},
  url          = {https://doi.org/10.5281/zenodo.placeholder}
}
```
*(注：首次在 Zenodo 完成 GitHub Release 同步后，请将上述 `doi.placeholder` 替换为您获得的真实 Zenodo DOI)*
