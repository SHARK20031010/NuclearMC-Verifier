# T1-M1 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少双弯折迷宫通道几何 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 迷宫双弯折通道与泄漏统计完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 迷宫双弯折通道与泄漏统计完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M1",
  "task_name": "二回路迷宫双弯折通道伽马与中子泄漏能谱",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "fluence",
      "src": "U"
    },
    "F1b": {
      "v": "T1-M1 目标几何空间出射面",
      "src": "U"
    },
    "F2": {
      "v": "count",
      "src": "U"
    },
    "F3": {
      "v": "other",
      "src": "A"
    },
    "F4": {
      "v": "surface_avg",
      "src": "U"
    },
    "F5": {
      "v": "steady",
      "src": "A"
    },
    "F6": {
      "v": "per_source",
      "src": "U"
    },
    "F7": {
      "v": "trend",
      "src": "A"
    },
    "F8": {
      "v": "other_mc",
      "src": "A"
    },
    "F9": {
      "v": "N/A",
      "src": "U"
    },
    "F10": {
      "v": "scalar",
      "src": "U"
    },
    "warnings": [
      "per_source_needs_strength"
    ]
  },
  "pathology_deduction": {
    "material_defect": "基线代码未严格配置重混凝土屏蔽墙材料组分，且物理列表缺少高精度中子物理包 QGSP_BIC_HP，无法正确计算中子热化与散射反照。",
    "geometry_defect": "基线代码缺失双弯折迷宫通道几何实体（入口 1m×1m、两折 90 度折角、出口段），无法模拟迷宫内的多次散射衰减。",
    "source_kinematics_defect": "基线代码仅发射单能点源，缺失 2 MeV 伽马与 U-235 裂变中子 Watt 连续能谱的混合源项抽样。",
    "tally_lifecycle_defect": "基线代码未在迷宫出口设置 1m×1m 边界跨越计分面，未解耦统计光子与中子的泄漏通量及分段能谱，且缺乏生命周期终止逻辑。"
  },
  "repairs_applied": {
    "material_repair": "采用 G4NistManager 构建标准 G4_CONCRETE 屏蔽墙，并注册 QGSP_BIC_HP 物理列表覆盖全能区中子/伽马输运与核反应。",
    "geometry_repair": "在 5m×3m×5m 混凝土墙内构建两折 90 度迷宫通道（Leg1 进深、Leg2 横向走廊、Leg3 出口段，截面均为 1m×1m），出口处放置 ExitDetector 计分面。",
    "kinematics_repair": "实现 2 MeV 单能伽马与 Watt 连续裂变快中子谱（a=0.988 MeV, b=2.249 MeV^-1）混合源抽样，在入口处对准第一通道发射。",
    "tally_repair": "在出口计分面严格按出射面 PostStep 几何边界跨越统计伽马与中子泄漏通量及 10 档能谱分布，并在计分后即刻终止粒子 (fStopAndKill)。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M1.cc $(geant4-config --cflags --libs) -o /tmp/t1m1_bin",
    "compile_ok": true,
    "run_command": "/tmp/t1m1_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "迷宫双弯折通道与泄漏统计完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
