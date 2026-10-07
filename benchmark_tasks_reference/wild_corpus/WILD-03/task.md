# WILD-03 · 使能光学物理但材料未绑定折射率 RINDEX 导致光子发射数为零

## 案例来源
> GitHub Issue [Project: Scintillation detector optical simulation returns 0 hits]

## 现象与求助背景
开源项目作者提交 Issue：'I added G4OpticalPhysics and set scintillation yield, but no optical photons are tracked at all.' 原因：材料属性表中漏设 RINDEX，Geant4 光学物理直接静默跳过。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
