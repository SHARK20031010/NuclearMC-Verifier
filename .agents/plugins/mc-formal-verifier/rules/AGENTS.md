# 粒子输运蒙卡程序形式化规约与生成准则 (Specification for Monte Carlo Transport)

> 本规则由插件 `mc-formal-verifier` 动态提供。当插件启用时生效。  
> 详见项目领域白皮书：[PHYSICS_SPEC.md](../../../PHYSICS_SPEC.md)

---

## 一、前置规约契约机制 (Pre-condition Specification Contract)

在编写或重构任何蒙特卡罗粒子输运仿真程序（当前落地 Geant4 C++，设计规约涵盖 OpenMC / MCNP）之前，**必须在代码注释或设计说明中首先对齐以下核心规约参数**：

1. **观测量种类 (Observable)**：明确为粒子注量（Fluence）、吸收剂量（Absorbed Dose）、比活度（Specific Activity）、能量沉积谱还是探测效率；
2. **物理作用域 (Spatial Scope)**：明确计数区域的几何实体名称与体积/表面积测度；
3. **辐射源项 (Source Specification)**：粒子种类、初级动能/能谱形状、空间分布、角分布测度（必须严格保持立体角 $d\Omega = \sin\theta d\theta d\phi$ 测度）；
4. **时间演化窗口 (Temporal Metric)**：明确区别粒子局域寿命（`GetLocalTime`）与全局实验时钟（`GetGlobalTime`），界定纳秒束流脉冲与核衰变时间窗；
5. **归一化基准 (Normalization)**：结果分母是“每源粒子（Per Primaries）”、“每居里/贝克勒尔（Per Becquerel）”还是“束流积分剂量”，必须保留完整换算链条。

---

## 二、五大正交物理守恒不变式约束 (Orthogonal Conservation Invariants)

代码必须严格遵守以下物理不变式，任何违约将被核验器直接阻断：

1. **能量与动量守恒不变式**：
   - 沉积能量与逃逸能量之和受初级动能与核反应 Q 值严格约束；严禁在步进计数中虚增或遗漏能量。
2. **守恒荷数不变式**：
   - 粒子生成与核反应中，电荷数、轻子数、重子数必须守恒。
3. **相空间与角分布测度不变式**：
   - 各向同性点源发射必须符合球面度测度均匀分布，严禁以笛卡尔线性抽样替代极角抽样；
   - 能量抽样如果为高斯分布或多能峰谱，必须使用标准反变换法或拒绝抽样，并正确截断负能区。
4. **输运统计无偏性与减方差权重不变式**：
   - 使用重要抽样、几何分裂（Splitting）或俄罗斯轮盘赌（Russian Roulette）时，粒子计分必须严格乘以统计权重（`GetWeight()`）；
   - 严禁任何无权重补偿的偏置操作。
5. **量纲链与响应标度线性不变式**：
   - 吸收剂量（Gy = J/kg）计算必须显式除以灵敏靶体积的真实介质质量（$V \times \rho$）；
   - 严禁混淆物理单位（如 Gy 与 Sv、Bq 与 Ci、MeV 与 J）。

---

## 三、控制流测度代数完备性与生命周期时序契约 (Measure-Theoretic Four-Basis & Lifecycle LTL)

蒙卡模拟本质是对相空间乘积测度空间的离散估计 $I = \int_{\Omega} f \, d\mu$。控制流的统计正确性必须严格满足以下**四维测度代数完备基与时序公理**：

1. **测度支撑集不变式 (Support Invariance · 条件槽位)**：
   - 跨界面通量与界面粒子积分，其测度支撑域必须严格限制在几何界面 $\partial V$；
   - 必须使用后步点（`PostStepPoint`）状态或联合判定；严禁将体内部步进或前步点误判为边界支撑集。
2. **示性测度重度不变式 (Multiplicity Invariance · 次数槽位)**：
   - 单粒子轨迹上示性测度严禁虚假倍乘膨胀；
   - 次级粒子产生位置或生成产额计分，必须显式满足单步单射条件（如 `GetCurrentStepNumber() == 1`）或集合去重（`insert(trackID)`），严禁沿径迹在后续数百个步进中重复触发。
3. **乘积测度纤维化重置不变式 (Fibration Invariance · 层级槽位)**：
   - 乘积测度空间 $\Omega_{\text{Event}} \times \Omega_{\text{Run}}$ 必须严格满足有限生命周期自动机分层隔离；
   - 事件级统计容器在进入新事件时，必须满足安全性时序命题 $\square (\text{Accumulate}(x) \implies \Diamond_{\text{past}} \text{Reset}(x, \text{Tier}_{\text{Event}}))$，严禁跨事件残留污染 (Inter-Event Stale Tally Accumulation)。
4. **鞅信息流因果时序不变式 (Filtration Inversion · 顺序槽位)**：
   - 物理观测量关于随机自然时序流 $\mathcal{F}_t$ 的取值必须先于状态更新与相互作用（$\text{Fetch}(E_{\text{kin}}) \ \mathcal{W} \ \text{ProcessInvocation}$）；
   - 严禁核反应后抓初态能，或能量沉积累加逆向于粒子反冲。
5. **探测器死层与灵敏体积物理隔离**：
   - 探测器灵敏体积前若存在死层（Dead Layer），死层内的能量沉积严禁计入光电峰积分。
6. **过程与核素识别**：
   - 次级粒子物理过程查询优先使用 `GetCreatorProcess()`；
   - 激活产物与衰变子体必须确保加载相应衰变物理过程（`G4RadioactiveDecay`）及核素数据。


---

## 四、形式化核验反思协议 (Formal Remediation Protocol)

若核验器（Formal Verifier）在提交前抛出违规异常：
- **禁止行为**：严禁通过硬编码经验常数、人为虚构加权因子或经验数值拟合掩盖物理缺陷！
- **正确行为**：阅读核验器指明的违规槽位（Define/Sample/Fetch/Accumulate/Convert），从输运物理机理与框架 API 用法层面进行正规修复。
