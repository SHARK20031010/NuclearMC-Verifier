# T1-M3 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少入射倾角 (30/45/60度) 动量矢量抽样 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 厚混凝土墙斜向入射角相关透射模型完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 厚混凝土墙斜向入射角相关透射模型完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M3",
  "task_name": "厚混凝土墙斜向入射角相关透射模型",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": "fluence",
    "F1b": "T1-M3 目标几何空间出射面",
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
  "pathology_deduction": {
    "material_defect": "基线代码使用了普通建筑混凝土 (G4_CONCRETE, 密度 2.3 g/cm3)，缺少重晶石混凝土关键重元素钡 (Ba, Z=56)，导致 6 MeV 伽马的光电效应与电子对产生截面被显著低估，未能满足用户要求的重晶石混凝土规范。",
    "geometry_defect": "基线代码墙体横向尺寸仅 1m×1m，在 60° 大倾角入射时侧向偏移达 69.3 cm，粒子容易从侧表面泄漏而非出射面透射，破坏了无限平板屏蔽边界条件。",
    "source_kinematics_defect": "基线代码仅在垂直法向 (0,0,1) 固定单向发射，完全缺失 0°、30°、45°、60° 倾角动量矢量抽样与参数化多工况支持。",
    "tally_lifecycle_defect": "基线代码仅凭单步坐标跃变 (z > 25 cm) 无差别累计所有粒子，混淆次级带电粒子与伽马透射；缺失全能未碰撞峰与多次散射能量解耦；缺失出射极角透射角分布分箱统计；未在出射后终止粒子，存在边界重复计分风险。"
  },
  "repairs_applied": {
    "material_repair": "采用 G4NistManager 混合 70% G4_BARIUM_SULFATE 与 30% G4_CONCRETE 构建密度 3.35 g/cm3 的真实重晶石防辐射混凝土 (BariteConcrete)。",
    "geometry_repair": "构建厚度 40 cm (half-z = 20 cm)、横向 3m×3m (half-x=1.5m, half-y=1.5m) 的厚屏蔽墙，充分容纳大倾角斜向散射路径。",
    "kinematics_repair": "实现参数化初级粒子源，支持 0°、30°、45°、60° 倾角矢量 (sin(theta), 0, cos(theta)) 抽样，束流精准入射于前表面中心 (z = -20.001 cm)。",
    "tally_repair": "在 SteppingAction 中严格判定从 BariteWall 到 World 出射后表面 (z >= 20.0 cm) 的伽马粒子；解耦统计 6 MeV 全能未碰撞与多次散射伽马；建立 0°~90° 范围 9 个出射极角分箱；出射后即刻调用 fStopAndKill 隔离生命周期，杜绝空气反散射重复统计。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M3.cc $(geant4-config --cflags --libs) -o /tmp/t1m3_bin",
    "compile_ok": true,
    "run_command": "/tmp/t1m3_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "厚混凝土墙斜向入射角相关透射模型完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
