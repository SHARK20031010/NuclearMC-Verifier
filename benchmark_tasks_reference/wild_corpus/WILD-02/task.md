# WILD-02 · 符合测量或飞行时间误用局部时钟 GetLocalTime()

## 案例来源
> CERN Geant4 User Forum [Topic: Coincidence time difference is always zero or small]

## 现象与求助背景
用户在论坛提问：'I am simulating a TOF detector. The time of arrival of neutrons is completely wrong (a few picoseconds instead of nanoseconds).' 用户读取了 GetLocalTime()，进入探测器敏感体积时局域时钟清零。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
