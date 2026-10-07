# WILD-10 · 强偏转弯铁磁场未配置步进弦长容差导致束流单步飞跃真空室边界丢弃

## 案例来源
> CERN Geant4 User Forum [Topic: Particle tracks jump across dipole magnet boundary and disappear]

## 现象与求助背景
束流物理用户求助：'In my 1.5 T dipole bending magnet, 100 MeV protons are lost. When looking at trajectories, the particle takes a single huge step of 20 cm, jumping completely over the curved vacuum chamber boundary and escaping into world.' 专家指出默认 fieldMgr 弦长容差过大，必须调用 fieldMgr->GetChordFinder()->SetDeltaChord(0.1 * mm)。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
