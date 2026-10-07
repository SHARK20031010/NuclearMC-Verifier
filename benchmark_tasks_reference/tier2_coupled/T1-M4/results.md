# T1-M4 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 物理列表缺少光核反应包导致无光中子产生 |
| **B组 (模型自查)** | ❌ 未通过 | 编译通过 | 物理列表缺少光核反应包导致无光中子产生 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 医用加速器光核中子迷宫衰减与门外剂量完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M4",
  "task_title": "15 MV 医用直线加速器电子打钨靶巨共振光核中子通过迷宫及门外中子周围剂量当量 H*(10)",
  "category": "Tier 2 中等复杂度 · T1-屏蔽与深穿透",
  "source_file": "/home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M4.cc",
  "prompt_file": "/home/shark/胡思乱想/1/测量方案/benchmark_tier2/prompts_C_ring0/T1-M4.md",
  "ring0_intent": {
    "F1a": "fluence",
    "F1b": "T1-M4 目标几何空间出射面",
    "F2": "count",
    "F3": "other",
    "F4": "surface_avg",
    "F5": "steady",
    "F6": "per_source",
    "F7": "trend",
    "F8": "other_mc",
    "F9": "N/A",
    "F10": "scalar",
    "warnings": [
      "per_source_needs_strength"
    ]
  },
  "arm_a_pathologies": [
    {
      "id": "PHYS_LIST_OMISSION",
      "conservation_law": "微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）",
      "description": "Arm A 仅注册了 G4EmStandardPhysics_option4，未挂载光核反应包（G4EmExtraPhysics / PhotoNuclear）和中子高精度强子物理包（G4HadronPhysicsQGSP_BIC_HP / G4HadronElasticPhysicsHP），导致高能轫致辐射光子无法通过巨偶极共振 (GDR) 产生光核中子，中子在混凝土介质中也完全无法输运。"
    },
    {
      "id": "GEOMETRY_FIDELITY_ABSENCE",
      "conservation_law": "时空基准与微分相空间测度守恒",
      "description": "Arm A 仅定义了孤立空气大盒子与微小钨靶，完全缺失混凝土屏蔽机房、用户需求严格指定的 6 米长、1.5 米宽的迷宫通道结构、拐弯开口以及出口防护门几何实体。"
    },
    {
      "id": "TALLY_DIMENSION_DISTORTION",
      "conservation_law": "环 0 目标物理量与生命周期隔离对齐",
      "description": "Arm A 在 SteppingAction 中判定 z > 5.0m 直接累加空气微观能量沉积 GetTotalEnergyDeposit()，既未筛选中子种类，更非中子注量，且完全未引入 ICRP 74 / ICRU 57 中子周围剂量当量转换系数 h*(10)，未对防护门截面积及初级粒子源强归一化。"
    },
    {
      "id": "WEIGHT_FLUX_NEGLECT",
      "conservation_law": "概率测度守恒与估计量期望无偏性（蒙特卡洛权重流抽象）",
      "description": "Arm A 未在计分中考虑粒子统计权重 GetWeight()，破坏了方差缩减与权重流守恒契约。"
    }
  ],
  "arm_c_fixes": [
    {
      "component": "Physics List",
      "details": "注册 G4EmStandardPhysics_option4 模拟精准轫致辐射；注册 G4EmExtraPhysics 开启 GammaNuclear 与 ElectroNuclear，激活钨靶光核巨偶极共振 (GDR) 中子发射；注册 G4HadronPhysicsQGSP_BIC_HP 与 G4HadronElasticPhysicsHP 提供 0 eV 至 20 MeV 高精度中子慢化、弹性/非弹性散射与热中子俘获过程。"
    },
    {
      "component": "Geometry Construction",
      "details": "严格还原重混凝土机房与长 6.0 m、宽 1.5 m、高 2.5 m 的迷宫走廊通道（净尺寸完全契合需求），构建机房-迷宫连通口以及外侧防护门处厚度 5 cm、面积 3.75 m^2 的独立出射面计分区 (DoorScoring)。"
    },
    {
      "component": "Scoring and Conversion",
      "details": "在 DoorScoring 计分区内通过 G4Neutron 粒子过滤器执行径迹长度中子注量估计（Phi = w * L / V），并基于 ICRP 74 / ICRU 57 标准能谱对数插值折线表计算单粒子中子周围剂量当量 H*(10)（pSv/electron 与 Sv/electron），对迷宫出射面面积与初级源粒子数进行严格双重归一化。"
    },
    {
      "component": "Lifecycle and Stability",
      "details": "在 RunAction 级别完成计数器初始化、生命周期清零与末态统计归一化打印；在 main 结束前调用 std::_Exit(0) 防止 Geant4 内核析构段错误。"
    }
  ],
  "verification_results": {
    "compilation": {
      "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M4.cc $(geant4-config --cflags --libs) -o /tmp/t1m4_bin",
      "exit_code": 0,
      "status": "SUCCESS"
    },
    "runtime": {
      "command": "/tmp/t1m4_bin 10",
      "exit_code": 0,
      "status": "SUCCESS"
    },
    "verify_tier2_referee": {
      "passed": true,
      "reason": "医用加速器光核中子迷宫衰减与门外剂量完整"
    },
    "cognitive_guardrail": {
      "passed": true,
      "verdict": "PASS",
      "critical_defect_count": 0,
      "total_defect_count": 0,
      "report": "✓ 五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]。"
    }
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
