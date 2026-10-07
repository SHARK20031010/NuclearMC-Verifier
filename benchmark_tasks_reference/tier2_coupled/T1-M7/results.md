# T1-M7 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少 1 mm 镉皮吸收层 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 石蜡慢化与镉皮热中子截止吸收完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 石蜡慢化与镉皮热中子截止吸收完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M7",
  "task_name": "含氢屏蔽慢化热中子在镉皮上的截止吸收效率",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "fluence",
      "src": "U"
    },
    "F1b": {
      "v": "T1-M7 目标几何空间出射面",
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
    "material_defect": "基线代码缺少 1 mm 镉皮吸收层（G4_Cd），或虽定义镉皮但未配置高精度热中子散射物理包与核截面，无法重现 Cd-113 巨大热中子俘获共振效应。",
    "geometry_defect": "基线代码未构建石蜡慢化球（R=15 cm）与外包 1 mm 镉球壳（R_in=15 cm, R_out=15.1 cm）双层嵌套界面几何，无法区分穿透石蜡与穿透镉皮两处界面。",
    "source_kinematics_defect": "基线代码或仅发射热中子，未从 2.0 MeV 快中子源与石蜡含氢弹性碰撞热化过程出发模拟连续减速谱。",
    "tally_lifecycle_defect": "基线代码未对穿过石蜡（Pre-Cd）与穿过镉皮（Post-Cd）的中子能谱进行三能段（热中子 E<0.5 eV、超热中子、快中子）解耦统计，无法量化热中子截断比（Cutoff Ratio）。"
  },
  "repairs_applied": {
    "material_repair": "采用 G4NistManager 构建标准 G4_PARAFFIN 慢化体与 G4_Cd 镉皮，采用 QGSP_BIC_HP 物理列表覆盖低能与热中子精密俘获截面。",
    "geometry_repair": "构建半径 15 cm 的石蜡球，外包 1 mm 镉壳（R=15.0~15.1 cm），并在内外表面设置确定的边界跨越检测。",
    "kinematics_repair": "球心各向同性发射 2.0 MeV 快中子，经历石蜡充分慢化后自然形成热中子能谱分量。",
    "tally_repair": "在 SteppingAction 中解耦记录穿出石蜡与穿出镉皮的中子能谱及通量，定量验证镉皮对热中子（<0.5 eV）的截断比，计分后终结粒子。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M7.cc $(geant4-config --cflags --libs) -o /tmp/t1m7_bin",
    "compile_ok": true,
    "run_command": "/tmp/t1m7_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "石蜡慢化与镉皮热中子截止吸收完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
