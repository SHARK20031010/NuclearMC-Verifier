# WILD-01 · 次级粒子空间产生分布在每步重复累加导致产额虚高 50 倍

## 案例来源
> CERN Geant4 User Forum [Topic: Tracking secondaries production vertex in UserSteppingAction]

## 现象与求助背景
用户在论坛发帖求助：'Why is my secondary hadron production yield ~50 times higher than cross section data?' 用户在 SteppingAction 中统计次级粒子，但未加 GetCurrentStepNumber() == 1，导致粒子向前输运的几十步全被当成产生点累加。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
