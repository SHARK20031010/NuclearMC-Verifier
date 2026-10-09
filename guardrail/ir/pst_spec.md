# 物理语义树形式化规范白皮书 (Physics Semantic Tree Specification)

> **版本**：v1.0.0-IR  
> **领域**：蒙特卡罗粒子输运科学计算 (Geant4 / OpenMC)  
> **定位**：连接 C++ 语法结构与微观物理守恒不变式的类型化中间表示 (Domain-Specific IR)

---

## 1. 为什么需要物理语义树 (PST)？

在通用编译器与软件工程分析工具中，代码被表示为**抽象语法树 (AST)**、**控制流图 (CFG)** 或 **数据流图 (DFG)**。然而，这些经典表示存在根本性缺陷：
- **经典表示仅捕捉“软件工程语义”**：例如“函数 $A$ 调用了函数 $B$”、“变量 $x$ 赋值给 $y$”；
- **完全无法表达“第一性原理物理语义”**：它无法知晓函数 $A$ 是在 $4\pi$ 球面度测度内采样动量方向，还是退化为了单向平行笔束；也无法知晓变量 $y$ 的累加是否丢失了统计权重 $\omega$ 或介质质量 $\rho V$。

**物理语义树 (Physics Semantic Tree, PST)** 是一个有向无环图 $\mathcal{G}_{PST} = (\mathcal{V}, \mathcal{E})$，其节点 $\mathcal{V}$ 表示具有独立物理意义的原子计算步骤，边 $\mathcal{E}$ 表示物理量、守恒流或量纲因果流转。

---

## 2. 节点系统：五动作 × 六总线域 (Actions × Domains)

每个 PST 节点 $v \in \mathcal{V}$ 满足：
$$v = \langle \text{id}, \mathcal{A}, \mathcal{D}, \mathcal{Q}, \mathcal{M}, \mathcal{I}_{exp}, \text{evidence}, \text{loc} \rangle$$

### 2.1 五大原子物理动作 ($\mathcal{A} \in \text{PhysicalAction}$)
1. **`Define` (定义与装配)**：材料微观截面、探测器几何体积、死层厚度、活度源强、物理列表装载。
2. **`Sample` (相空间抽样)**：动量角分布立体角抽样、连续能谱反变换抽样、空间位置抽样、衰变时刻抽样。
3. **`Fetch` (步进物理提取)**：在微观步进中抓取能量沉积 $E_{dep}$、全局实验时钟 $t_{global}$、统计权重 $\omega$、跨界面状态 $s_{status}$。
4. **`Accumulate` (物理计数累加)**：能谱多道计数道址累加、探测效率历史事件累计、跨界面粒子流积分。
5. **`Convert` (量纲转换与归一)**：吸收剂量除以介质质量 ($D = E_{dep} / m$)、每源粒子除以发射总数、物理单位换算。

### 2.2 六大正交守恒总线域 ($\mathcal{D} \in \text{ConservationDomain}$)
1. **`PhaseSpace` (相空间测度域)**：立体角测度微元 $d\Omega = \sin\theta d\theta d\phi$ 保持、极坐标圆盘开方抽样、高斯展宽 $\text{FWHM} = 2.355\sigma$。
2. **`Causality` (因果与实验时钟域)**：绝对全局实验室时钟 (`GetGlobalTime()`)、飞行时间 (TOF)、双探头符合测量时间窗 $|\Delta t| \le \tau$。
3. **`Variance` (权重流与统计无偏域)**：重要抽样、几何分裂与俄罗斯轮盘赌中的统计权重 $\omega$ 守恒传递。
4. **`Lifecycle` (步进生命周期与几何边界域)**：后步点 (`PostStepPoint`) 跨界面通量单调性、死层能量与有效灵敏区物理隔离、Run/Event 容器隔离。
5. **`Nuclear` (核素标识与微观反应域)**：热中子 $S(\alpha, \beta)$ 散射库完整性、衰变子体分支比、光核反应物理过程加载。
6. **`Ring0Intent` (意图与需求契约域)**：目标物理量定义 (F1)、空间作用域 (F4)、归一化分母基准 (F6) 与源绝对强度 (F9)。

---

## 3. 依赖边系统：六类物理因果流 ($\mathcal{E} \in \text{PSTEdgeType}$)

PST 的有向边 $e = (u, v) \in \mathcal{E}$ 编码了跨语句、跨方法的微观物理量传递关系：

1. **`WeightFlow` (统计权重流)**：从 `Fetch(Weight)` 节点引向 `Accumulate(Tally)` 节点。若经历减方差操作的径迹缺失此边，直接判死为权重偏置失效。
2. **`CausalTime` (时钟因果流)**：从粒子源发射时钟引向探测器到达时钟，强制保证时钟基准属于绝对全局实验坐标系。
3. **`NormalizationDenominator` (归一化分母边)**：从 `Define(Mass)` 或 `Define(Primaries)` 节点引向 `Convert(Dose/Activity)` 节点，确保分母量纲闭环。
4. **`BoundaryGate` (几何边界门控边)**：表明该抓取/累加动作必须处于 `stepStatus == fGeomBoundary` 的真值分支内。
5. **`VolumeContainment` (几何拓扑从属边)**：标注物理体积是否包含死层，若死层节点向有效计分节点输送能量边，则判定为死层混淆违约。
6. **`DataDependency` (通用数据依赖边)**：常规的数值与对象计算链路。

---

## 4. 守恒验证：物理断言在 PST 上的图论判定公式

将复杂的蒙卡输运审查，形式化转化为 PST 上的图遍历判定：

### 判定定理 1：公平博弈统计无偏判定 (Fair-Game Tally Invariance)
$$\forall v \in \mathcal{V}_{\text{Accumulate}}, \quad \text{若存在减方差分裂/轮盘赌，则 } \exists u \in \mathcal{V}_{\text{Fetch(Weight)}}, \quad \text{满足 } (u, v) \in \mathcal{E}_{\text{WeightFlow}}$$
**违约诊断**：入度缺少权重流边，粒子通量或沉积谱严重人为高估/失真。

### 判定定理 2：相空间各向同性测度判定 (Phase-Space Measure Invariance)
$$\forall v \in \mathcal{V}_{\text{Sample(Direction)}}, \quad \text{若 } v.\text{metric} = \text{Isotropic}, \quad \text{则 } v.\text{AST} \text{ 必须包含 } \mathcal{J}(\theta) = \sin\theta \text{ 或调用 } \text{G4RandomDirection}()$$
**违约诊断**：极角线性抽样，导致球面两极发生测度人工聚焦，角分布非物理畸变。

### 判定定理 3：吸收剂量质量归一判定 (Absorbed Dose Mass Invariance)
$$\forall v \in \mathcal{V}_{\text{Convert(Dose)}}, \quad \exists u \in \mathcal{V}_{\text{Define(Mass)}}, \quad \text{满足 } (u, v) \in \mathcal{E}_{\text{NormalizationDenominator}}$$
**违约诊断**：吸收剂量（Gy = J/kg）未除以介质质量 $m = V \times \rho$，数值偏差数个数量级。
