# WILD-06 · 快中子照射塑料闪烁体未配置 Birks 猝灭常数导致光输出高估 5.6 倍

## 案例来源
> CERN Geant4 User Forum [Topic: Fast neutron scintillation light yield vs gamma light yield ratio inverted]

## 现象与求助背景
用户在论坛求助：'I am simulating neutron detection with EJ-200 / BC-408 plastic scintillator using G4OpticalPhysics. The simulation predicts neutron light output is 3.5x higher than 1 MeV gamma, but in my lab measurement, gamma gives much more light! What is wrong with Geant4?' 专家指出有机塑料闪烁体必须调用 SetBirksConstant(0.126 * mm / MeV)，否则高 LET 反冲质子完全不猝灭。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
