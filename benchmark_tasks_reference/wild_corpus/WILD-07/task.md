# WILD-07 · 50 MeV 电子打厚钨靶模拟中子产额缺失光核物理包产额恒为零

## 案例来源
> CERN Geant4 User Forum [Topic: Photonuclear reaction produces 0 neutrons with 50 MeV electrons]

## 现象与求助背景
用户提问：'I set up 50 MeV electrons hitting a 2 mm tungsten converter target to produce bremsstrahlung and photoneutrons. I used G4EmStandardPhysics. But my neutron detector records exactly 0 neutrons. Is (gamma, n) cross section missing?' 专家指出标准电磁包不含光核反应，必须注册 G4PhotonuclearPhysics。

## 涉及的物理规则与检查点
- **操作检查点**：粒子产生生命周期与首步过滤
- **守恒规则**：输运生命周期 (LIFECYCLE-CORE)
