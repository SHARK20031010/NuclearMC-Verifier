# WILD-08 · 半导体探测器载流子统计涨落漏乘 Fano 因子导致本征分辨虚假展宽 3 倍

## 案例来源
> GitHub Issue [Repo: HPGe-PulseProcessor / Issue: Peak width (FWHM) from carrier statistics is 3x larger than experiment]

## 现象与求助背景
开源项目作者提交 Issue：'Our Monte Carlo simulated HPGe peak width at 1332 keV has FWHM ~ 5.8 keV, while manufacturer spec is 1.8 keV. We calculate variance as Var(N) = N = E / 2.96 eV. Where does the extra broadening come from?' 社区指出半导体电荷对不是泊松分布，方差需乘入 Fano 因子 (F ~ 0.08)。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
