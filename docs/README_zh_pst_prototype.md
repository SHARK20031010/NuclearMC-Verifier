<div align="center">

# ⚛️ 物理语义树 (PST) 形式化核验引擎

### 面向蒙特卡罗辐射输运代码形式化验证的领域物理中间表示 (Physics IR)

**[English](README.md) | [中文版](README_zh.md)**

<p align="center">
  <img src="https://img.shields.io/badge/Language-Python_3.9+-3776ab.svg" alt="Python">
  <img src="https://img.shields.io/badge/Graph_Backend-NetworkX-orange.svg" alt="NetworkX">
  <img src="https://img.shields.io/badge/Schema-Pydantic_v2-e92063.svg" alt="Pydantic">
  <img src="https://img.shields.io/badge/Latency-%3C_10ms-brightgreen.svg" alt="Latency">
  <img src="https://img.shields.io/badge/Target-Geant4_%7C_OpenMC_%7C_MCNP-blue.svg" alt="Target">
</p>

</div>

---

## 🎯 研究动机：跨越“物理语义鸿沟”

大语言模型（LLM）编写通用代码游刃有余，且生成的 C++ 代码往往能 0 警告编译通过、退出码为 0。但在连续相空间的粒子输运与辐射物理计算中，极易产生导致结果偏差 $10^2 \sim 10^7$ 倍的**“静默物理错误”**。

现有的代码表示模型（如 LLVM IR、Tree-Sitter CST、代码属性图 CPG）仅能捕获**“软件工程语义”**（函数调用、类型继承、内存数据流）。它们无法理解：
- 相空间立体角保测度映射 ($d\Omega = \sin\theta d\theta d\phi$)
- 公平博弈粒子权重流耦合 ($\sum x_i \cdot w_i$)
- 相对论实验室全局时钟与局域衰变寿命的因果隔离

为此，我们提出了**物理语义树（Physics Semantic Tree, PST）**：首个将代码底层 AST 语法 token 映射为领域物理输运算子及守恒传导图的物理级中间表示（Physics IR）。

```text
                  C++ 源码
                     │
        [第一层: AST 解析与 API 提取]
          CallExpr, DeclStmt, AssignExpr
                     │
        [第二层: 局部上下文增强包 (Depth-1)]
          展开周围条件谓词、首步标记、边界守卫
                     │
             ┌───────┴───────┐
             ▼               ▼
     【快轨: 确定性模式匹配】 【慢轨: LLM 结构化推断】
    (100+ 已知 Geant4 API)    (用户自定义封装函数)
             └───────┬───────┘
                     │
        [第三层: 结构化物理语义描述]
          PhysicalOperation JSON 记录
                     │
        [第四层: 物理语义树 (PST) 构建]
          G_PST = (V_phys, E_flow) 数据流图叠加
                     │
        [第五层: 守恒规则图分析核验]
          图可达性分析与流守恒断言
                     │
          验证通过 / 物理缺陷拦截诊断报告
```

---

## 🏗️ 五层技术架构拆解

### 第一层：AST 解析与 API 提取
- 遍历 C++ 抽象语法树，提取函数声明体、调用表达式（`CallExpr`）、声明（`DeclStmt`）与赋值（`AssignExpr`）。
- 筛选蒙卡核心候选操作（如 `GetTotalEnergyDeposit`, `GetWeight`, `G4RandomDirection` 等）。

### 第二层：局部上下文增强（Depth-1 展开）
- **规避陷阱一（上下文感知缺失）**：单个 API 在不同控制流下含义截然不同。
- 打包包含调用点的直接作用域：周围 `if` 条件谓词（几何边界状态、产生首步过滤）、局部变量声明、直接被调用者函数签名与函数体。

### 第三层：双轨制物理语义映射
- **规避陷阱二（LLM 输出随机性与延迟代价）**：
  - **确定性快轨（<10ms）**：基于离线提纯并固化的 100+ 核心 API 知识库，零 token 成本、100% 确定性极速映射。
  - **LLM Schema 慢轨（兜底）**：仅在遇到未知自定义类或封装函数时触发，通过严格 Pydantic Schema 约束输出。

### 第四层：物理语义树 (PST) 图组装
- 基于 NetworkX 构建有向类型化属性图 $\mathcal{G}_{PST} = (\mathcal{V}_{phys}, \mathcal{E}_{flow})$：
  - **物理节点 $\mathcal{V}_{phys}$**：五动作（`Define`, `Sample`, `Fetch`, `Accumulate`, `Convert`）。
  - **守恒传导边 $\mathcal{E}_{flow}$**：`DATA_DEPENDENCY`（数据依赖）、`WEIGHT_COUPLING`（权重流耦合）、`LIFECYCLE_CONTAINER`（生命周期从属）、`CAUSALITY_FLOW`（时钟因果）。

### 第五层：图分析与守恒总线核验
- 将第一性原理守恒定律表述为 **PST 图上的可达性分析与流守恒断言**：
  - `PHYS-INV-WEIGHT-01`：深穿透减方差计分器未引入动态权重流耦合（漏乘 `GetWeight()`）。
  - `PHYS-INV-MEASURE-01`：点源发射方向球面度测度塌缩为固定单向笔形束。
  - `PHYS-INV-LIFECYCLE-01`：次级粒子顶点统计未限制产生首步，导致全径迹重复累加虚高。
  - `PHYS-INV-CAUSAL-01`：绝对时钟误用粒子局域寿命 `GetLocalTime()`。

---

## 🧪 黄金标准集验证（规避陷阱三：PST 正确性）

为量化证明 PST 图构建的准确率与鲁棒性，建立了 `Golden-PST-30` 专家级黄金基准集：
- 覆盖 6 大物理领域的 30 个代表性仿真场景。
- 通过 `golden_benchmark/evaluate_pst.py` 自动计算 **节点级 F1** 与 **边级 F1** 分数：

```bash
# 对标预测 PST 与真实 Golden PST
python3 cli.py eval /path/to/case_buggy.cc golden_benchmark/sample_annotations/wild_01_secondary_vertex.json
```

---

## 🚀 快速上手与 CLI 工具

### 1. 安装依赖
```bash
pip install -r requirements.txt
```

### 2. 常用命令
```bash
# 1. 运行形式化核验报告
python3 cli.py check path/to/simulation.cc

# 2. 生成并输出 PST 的 Mermaid 关系图代码
python3 cli.py mermaid path/to/simulation.cc

# 3. 导出完整 PST 图结构的 JSON 数据
python3 cli.py dump-pst path/to/simulation.cc

# 4. 运行全量单元测试
python3 -m pytest tests/
```

---

## 📄 学术定位（CPC / Energy and AI 论文贡献）

> **新型物理级中间表示 (Novel Physics-Level IR)**：提出了连接微观代码语法树（AST）与宏观物理守恒律的**物理语义树（Physics Semantic Tree, PST）**。区别于现有只捕获计算机控制流的软件工程图（CPG/CallGraph），PST 在理论上首次形式化了连续相空间粒子输运的本构过程，在毫秒级算力消耗下实现了无数值模拟的 100% 物理闭环核验。
