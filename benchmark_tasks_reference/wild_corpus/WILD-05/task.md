# WILD-05 · 圆形扩展面源半径抽样未开方导致虚假中心尖峰

## 案例来源
> CERN Geant4 User Forum [Topic: GPS circular disk source concentration in the center]

## 现象与求助背景
论坛求助帖：'When I sample particles uniformly on a disk using r = R * G4UniformRand(), the 2D histogram shows a high peak at r=0. Why is it not flat?' 专家指出极坐标微元为 r dr dtheta，必须开平方 r = R * sqrt(xi)。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
