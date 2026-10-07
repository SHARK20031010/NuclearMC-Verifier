# WILD-04 · 深穿透几何重要性分裂计分漏乘粒子权重 GetWeight()

## 案例来源
> GitHub Commit [Repo: ShieldingMC / Commit: Fix missing GetWeight() in flux tally]

## 现象与求助背景
开源项目作者提交 Commit 记录：'Fix severe bug where flux scored was multiplied by 1.0 instead of track weight during splitting, leading to 10^4 times overestimation.'

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
