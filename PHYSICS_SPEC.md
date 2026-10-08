# 粒子输运蒙卡程序形式化规约与生成准则 (Specification for Monte Carlo Transport)

> 本文档为**针对大语言模型生成粒子输运蒙卡程序的形式化核验器**（Formal Verifier for LLM-Generated Monte Carlo Particle Transport Codes）的领域物理规约白皮书。  
> 适用于全量智能体（Antigravity, OpenCode, DSH, Claude Code, Codex）与人类开发者。

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

## 三、Geant4 框架语义与生命周期契约

1. **步进生命周期**：
   - 跨界面通量统计必须使用后步点（`PostStepPoint`）状态，或基于前步点（`PreStepPoint`）与边界状态联合判定；
   - 探测器灵敏体积前若存在死层（Dead Layer），死层内的能量沉积严禁计入光电峰积分。
2. **过程与核素识别**：
   - 次级粒子物理过程查询优先使用 `GetCreatorProcess()`；
   - 激活产物与衰变子体必须确保加载相应衰变物理过程（`G4RadioactiveDecay`）及核素数据。
3. **容器重置拓扑序**：
   - Run 级统计容器与 Event 级累加变量生命周期必须显式隔离，禁止跨事件残留计数污染 (Inter-Event Stale Tally Accumulation)。

---

## 四、形式化核验反思协议 (Formal Remediation Protocol)

若核验器（Formal Verifier）在提交前抛出违规异常：
- **禁止行为**：严禁通过硬编码经验常数、人为虚构加权因子或经验数值拟合掩盖物理缺陷！
- **正确行为**：阅读核验器指明的违规槽位（Define/Sample/Fetch/Accumulate/Convert），从输运物理机理与框架 API 用法层面进行正规修复。

---

## 五、核验器启停与命令行接口 (CLI & Master Switch)

可在终端直接使用总控开关与检查工具：
```bash
./mc-verifier status    # 查看当前全局/局部核验状态与拦截策略
./mc-verifier on        # 启用形式化核验器（严格门禁）
./mc-verifier off       # 禁用形式化核验器（零开销放行）
./mc-verifier check <file.cc>  # 针对指定 C++ 源码执行离线核验
```
