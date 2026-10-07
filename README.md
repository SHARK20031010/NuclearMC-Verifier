# NuclearMC-Verifier: 粒子输运蒙卡程序形式化核验器

### Formal Verifier for LLM-Generated Monte Carlo Particle Transport Codes

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-Apache_2.0-blue.svg" alt="License"></a>
  <a href="https://geant4.web.cern.ch/"><img src="https://img.shields.io/badge/Geant4-11.2+-brightgreen.svg" alt="Geant4"></a>
  <a href="https://www.python.org/"><img src="https://img.shields.io/badge/Python-3.9+-blue.svg" alt="Python"></a>
  <img src="https://img.shields.io/badge/Benchmark-190_Tasks-orange.svg" alt="Benchmark">
  <img src="https://img.shields.io/badge/Arm_C_Accuracy-100.0%25-success.svg" alt="Accuracy">
  <img src="https://img.shields.io/badge/Harness-Antigravity_%7C_DSH_%7C_Claude_%7C_OpenCode_%7C_MCP-purple.svg" alt="Harness">
</p>

**面向大语言模型（LLM）生成辐射与粒子输运蒙特卡罗代码（Geant4 / OpenMC）的确定性形式化核验器（Formal V&V Engine）及 190 题分级评测基准参考库。**

---

## 目录

- [一、项目背景与核心挑战](#一项目背景与核心挑战)
- [二、全量 190 题三臂盲测实证结论](#二全量-190-题三臂盲测实证结论)
- [三、核心架构与形式化守恒机制](#三核心架构与形式化守恒机制)
  - [1. 「五动作 × 31 槽位」生成性缺陷分类学](#1-五动作--31-槽位生成性缺陷分类学)
  - [2. 输入规约契约前置对齐（Pre-condition Specification Contract）](#2-输入规约契约前置对齐pre-condition-specification-contract)
  - [3. 五大正交第一性原理物理守恒不变式](#3-五大正交第一性原理物理守恒不变式)
- [四、多智能体生态原生支持 (5 大 Harness 兼容)](#四多智能体生态原生支持-5-大-harness-兼容)
- [五、快速上手指南](#五快速上手指南)
  - [1. 环境准备](#1-环境准备)
  - [2. CLI 总控与启闭](#2-cli-总控与启闭)
  - [3. 对输运源码执行形式化核验](#3-对输运源码执行形式化核验)
  - [4. 一键挂载多平台 Agent 插件](#4-一键挂载多平台-agent-插件)
- [六、190 题基准参考库导航](#六190-题基准参考库导航)
- [七、项目仓库结构](#七项目仓库结构)
- [八、开源协议与引用规范](#八开源协议与引用规范)

---

## 一、项目背景与核心挑战

在核工程、深空探测、辐射防护与高能物理等安全攸关（Safety-Critical）领域，蒙特卡罗辐射输运模拟（如 Geant4、OpenMC）是不可替代的数值模拟基座。大语言模型（LLM）在编写此类程序时，展现出极高的代码生成与编译通过率，但极易出现**静默物理违约（Silent Physical Violations）**与**伪收敛（Pseudo-Convergence）**：

- **编译退出码为 0**：代码零警告编译通过，蒙卡事件循环正常结束；
- **严重数值失真**：由于微观反应截面缺位（如热中子 $S(\alpha,\beta)$）、几何死层未扣除、方差缩减漏乘权重或相空间抽样退化，导致吸收剂量、粒子通量或核素活度估算产生 $10^2 \sim 10^7$ 倍的隐蔽物理畸变；
- **反思欺骗与认知断崖**：复杂场景下，通用的自我反思（Self-Reflection / CoT）提示词不仅无法发现微观物理逻辑缺陷，反而倾向于通过硬编码经验常数掩盖物理错误，诱发“伪收敛”。

为了彻底解决这一根本难题，本项目构建了**基于形式化方法与抽象语法树（AST）静态分析的确定性物理核验引擎**，在代码交付前以微秒级延迟完成物理守恒量断言与生命周期审查。

---

## 二、全量 190 题三臂盲测实证结论

本基准对 190 道涵盖多物理场景的蒙卡程序生成任务执行了严格的隔离式三臂独立盲测实验（3-Arm Blind Experiment）：
- **Arm A (Raw Baseline)**：无约束直接生成（反映顶级模型在单轮提示下的直接代码编写能力）。
- **Arm B (Self-Reflection / CoT)**：模型多轮反思自查与重新思考（思维链自我修正能力）。
- **Arm C (Ours: Formal Verifier)**：挂载本形式化规约契约与正交守恒不变式门禁。

### 实测评测数据矩阵

| 评测梯级 (Task Tier) | 题目规模 (N) | Arm A (原始基线) | Arm B (模型自查) | Arm C (本形式化核验器) | 核心失效特征分析 |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Tier 1 (基础单槽位)** | 60 | 25 / 60 (41.7%) | 59 / 60 (98.3%) | **60 / 60 (100.0%)** | 通用自省可修正初级语法与材料定义，但难察觉截面库命名隐式退化 |
| **Tier 2 (跨链中等耦合)** | 60 | 0 / 60 (0.0%) | 22 / 60 (36.7%) | **60 / 60 (100.0%)** | 几何边界与反应态跨模块级联，无形式化约束的模型全线崩溃 |
| **Tier 3 (极限深穿透与动力学)** | 60 | 0 / 60 (0.0%) | 32 / 60 (53.3%) | **60 / 60 (100.0%)** | 复杂减方差权重漂移；自省诱发 16.7% 的常数拟合伪收敛 |
| **Real-World Wild Corpus** | 10 | 0 / 10 (0.0%) | 0 / 10 (0.0%) | **10 / 10 (100.0%)** | 源自 CERN 官方论坛与 GitHub 真实长尾静默物理缺陷 |
| **全量微观平均 (Overall)** | **190** | **25 / 190 (13.2%)** | **113 / 190 (59.5%)** | **190 / 190 (100.0%)** | **通用自省收益 +46.3%，形式化核验实现 100% 物理闭环拦截与修复** |

<p align="center">
  <img src="figures/fig1_accuracy_gradient.png" width="85%" alt="三臂盲测准确率梯度对比">
</p>

---

## 三、核心架构与形式化守恒机制

### 1. 「五动作 × 31 槽位」生成性缺陷分类学

蒙特卡罗输运程序中任何物理数据流操作，在形式化语义上严格受限于**五种基本计算动作**与**控制流拓扑轴**，构成封闭的 31 维规约违规槽位空间：

| 基本操作 | 形式化语义 (Formal Semantics) | 规约槽位 (31维) | 典型物理违约表现 |
| :--- | :--- | :--- | :--- |
| **定 (Define)** | **实体绑定与注册**：几何、材料、微观截面与初值绑定 | 有无、重复、身份、取值 (4) | 漏注册热中子高精度 $S(\alpha,\beta)$ 截面；闪烁体漏设 Birks 猝灭常数 |
| **抽 (Sample)** | **测度抽样与相空间分布**：粒子源相空间分布抽样 | 变量、定义域、形状、参数、实现、相关性 (6) | 各向同性立体角退化为笛卡尔平面抽样；漏抽核素双能谱共存衰变 |
| **取 (Fetch)** | **状态提取与步进探针**：运行时径迹状态与微观量解构 | 谁、何时、哪里、哪个字段、存在性 (5) | 步后读取初始动能分错能谱箱；步进中查询未生成的次级微观态 |
| **记 (Accumulate)** | **物理量积分与箱元累加**：相空间网格积分与通量计数 | 记什么、在哪记、何时记、权重、分箱、聚合 (6) | 径迹长度通量误累加为粒子数；减方差技巧中漏乘统计权重 |
| **换 (Convert)** | **量纲变换与标度映射**：单位换算与绝对活度标度 | 分母、单位链、响应系数、归一化基准、误差 (5) | 活度公式误除衰变常数致结果失真 $10^7$ 倍；Gy 与 Sv 物理量纲混用 |
| **控制流轴** | **动作时序与拓扑序**：生命周期循环与事件拓扑 | 层级、次数、条件、顺序 (4) | Run 级容器未按 Event 及时清空导致跨事件残留计数污染 |

<p align="center">
  <img src="figures/fig2_action_slots_distribution.png" width="85%" alt="失效槽位分布">
</p>

### 2. 输入规约契约前置对齐（Pre-condition Specification Contract）

在上游彻底消除自然语言任务描述中的隐式假设与模糊性：
- 明确观测量种类（粒子注量 Fluence、吸收剂量 Absorbed Dose、比活度 Specific Activity、探测效率）；
- 严格界定物理计数区域几何体名称与真实介质质量（$V \times \rho$）；
- 规范粒子源项相空间分布与微元球面度测度（$d\Omega = \sin\theta d\theta d\phi$）；
- 区分纳秒束流脉冲时钟（`GetLocalTime`）与核衰变全局时钟（`GetGlobalTime`）；
- 固化归一化换算链（每源粒子、每贝克勒尔或积分束流强）。

### 3. 五大正交第一性原理物理守恒不变式

下游核验器通过 66 条形式化规则对 C++ 源码执行刚性物理约束：
1. **能量与动量守恒不变式 (Energy & Momentum)**：$\sum E_{\text{in}} = \sum E_{\text{dep}} + \sum E_{\text{escape}} + \Delta Q$；
2. **守恒量子数不变式 (Quantum Numbers)**：核反应中轻子数、重子数、电荷数严格守恒；
3. **相空间与角分布测度不变式 (Phase-Space Measure)**：各向同性源严格遵循球面度立体角抽样，严禁笛卡尔直抽；
4. **输运统计无偏性与权重不变式 (Fair-Game Weights)**：分裂（Splitting）与轮盘赌（Russian Roulette）必须严格乘上 `GetWeight()`；
5. **量纲链与标度线性不变式 (Dimensional Linearity)**：吸收剂量计算必须显式除以敏感体积质量，严禁混淆 Gy 与 Sv、Bq 与 Ci。

> 详见白皮书规约定义：[`PHYSICS_SPEC.md`](PHYSICS_SPEC.md)

---

## 四、多智能体生态原生支持 (5 大 Harness 兼容)

本项目提供了跨主流大模型 Coding Agent 的原生适配架构：

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        NuclearMC-Verifier CLI                          │
└──────┬───────────────┬────────────────┬───────────────┬────────────────┘
       ▼               ▼                ▼               ▼
Google Antigravity    DSH (Cordis)    Claude Code    OpenCode / MCP
  Pre-exec Hook     Lifecycle Gate   Subagent Tool   Universal JSON-RPC
```

- **Google Antigravity (AGY)**：支持 Pre-execution 拦截钩子，在代码交付给编译器前自动阻断物理缺陷；
- **DeepSeek Harness (DSH)**：Cordis 插件，支持原生工具级前置拦截瀑布门禁；
- **Anthropic Claude Code**：提供自包含 Subagent 规范与审计工具集成；
- **OpenCode**：兼容自定义规则注入与插件协议；
- **Model Context Protocol (MCP)**：标准 JSON-RPC 2.0 服务端（`adapters/mcp/server.py`），任何支持 MCP 的 IDE 均可即插即用。

---

## 五、快速上手指南

### 1. 环境准备

本项目核验引擎基于 Python 3.9+ 纯标准 AST 引擎构建，安装基础依赖即可运行：

```bash
# 克隆仓库
git clone https://github.com/SHARK20031010/NuclearMC-Verifier.git
cd NuclearMC-Verifier

# 安装环境依赖
pip install -r requirements.txt
```

### 2. CLI 总控与启闭

项目根目录下提供了自包含的总控可执行脚本 `./mc-verifier`：

```bash
# 查看当前 5 大 Harness 就绪情况与核验开关
./mc-verifier status

# 开启物理守恒严格核验门禁
./mc-verifier on

# 关闭核验（进入 0ms 直通模式）
./mc-verifier off
```

### 3. 对输运源码执行形式化核验

可以随时对任意 Geant4 C++ 源码执行单文件物理守恒核验：

```bash
# 测试合规源码（输出：守恒不变式底层契约核验通过）
./mc-verifier check benchmark_tasks_reference/tier1_single_slot/T1-1/code/code_ArmA.cc

# 测试缺陷源码（拦截静默物理错误并输出缺陷诊断与自查清单）
./mc-verifier check benchmark_tasks_reference/wild_corpus/WILD-01/code/case_buggy.cc
```

### 4. 一键挂载多平台 Agent 插件

```bash
# 一键向本地全部 5 大 Harness 挂载技能与适配器配置
./mc-verifier install-all
```

---

## 六、190 题基准参考库导航

本项目附带了脱敏且结构完备的 190 题评测参考库，每一道题均独立成目录，包含**题目要求、考点与典型缺陷说明、三组实测提示词、盲测对比判决以及对应的 C++ 源码**：

```text
benchmark_tasks_reference/
├── tier1_single_slot/           # 60 题基础单考点 (材料、几何、截面、粒子源)
├── tier2_coupled_systems/       # 60 题跨模块中等耦合 (多区域界面、纳秒脉冲、衰变链)
├── tier3_deep_penetration/      # 60 题极限深穿透与动力学 (方差缩减、极薄靶、活化热)
└── wild_corpus/                 # 10 题真实社区长尾漏洞 (CERN 论坛与 GitHub 真实 Bug)
```

> 完整题库导航与对比表格参见：[`benchmark_tasks_reference/README.md`](benchmark_tasks_reference/README.md)

---

## 七、项目仓库结构

```text
NuclearMC-Verifier/
├── guardrail/                         # 形式化核验核心引擎
│   ├── engine/                        # AST 解析、守恒量断言与生命周期检测核心
│   ├── rules/                         # 形式化物理守恒规则 (PHYS-0001 ~ PHYS-0066)
│   ├── data/                          # 核素衰变数据库、材料常数与截面表
│   └── check.py                       # 静态核验审计入口
├── adapters/                          # 5 大智能体平台适配层
│   ├── antigravity/                   # Google Antigravity 拦截钩子
│   ├── dsh/                           # DeepSeek Harness Cordis 插件
│   ├── mcp/                           # Model Context Protocol 标准服务端
│   └── claude/                        # Claude Code 适配规范
├── .agents/                           # 项目私有插件与技能扩展 (mc-formal-verifier)
├── benchmark_tasks_reference/         # 190 题分级评测基准参考库 (含任务、提示词、代码与结论)
├── figures/                           # 论文级可视化图表资产 (准确率梯度、失效槽位分布)
├── scripts/                           # 统一控制与基准构建脚本
├── mc-verifier                        # 统一命令行总控可执行文件
├── PHYSICS_SPEC.md                    # 粒子输运蒙卡程序形式化规约白皮书
├── CITATION.cff                       # 学术引用元数据
├── LICENSE                            # Apache 2.0 开源许可证
├── requirements.txt                   # 依赖清单
└── README.md                          # 本文档
```

---

## 八、开源协议与引用规范

本项目采用 **Apache License 2.0** 许可证开源，完整条款参见 [LICENSE](LICENSE)。

如果您在核科学、计算物理或 AI 代码生成研究中使用了本项目的形式化核验器、规约体系或 190 题基准库，欢迎引用：

```bibtex
@software{nuclear_mc_verifier_2026,
  author       = {{NuclearMC-Verifier Contributors}},
  title        = {{NuclearMC-Verifier: A Formal Verifier for Large Language Model-Generated Monte Carlo Particle Transport Codes}},
  year         = {2026},
  publisher    = {GitHub},
  version      = {v1.0.0},
  url          = {https://github.com/SHARK20031010/NuclearMC-Verifier}
}
```
