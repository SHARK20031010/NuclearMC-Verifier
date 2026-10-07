# WILD-09 · 热中子在含氢慢化剂中自建水材料命名为 H2O 导致 S(alpha, beta) 静默失效能谱严重偏硬

## 案例来源
> CERN Geant4 User Forum [Topic: Thermal neutron spectrum in water moderator does not Maxwellianize]

## 现象与求助背景
用户提问：'I created a custom water material using AddElement(H, 2); AddElement(O, 1); and registered G4ThermalNeutrons. But the neutron energy spectrum exiting the water tank peaks around 0.5 eV instead of thermal 0.025 eV Maxwellian peak. Why?' 专家指出 G4ThermalNeutrons 依赖标准材料名称 'G4_WATER' 进行截面库字典键匹配，自建命名为 'H2O' 导致热散射静默失效。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
