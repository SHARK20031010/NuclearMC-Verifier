# T2-M10 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少侧向 45/90 度角探测器杂散中子计分 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 碳离子侧向散射杂散中子剂量模型完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 碳离子侧向散射杂散中子剂量模型完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M10",
  "task_name": "290 MeV/u 碳离子照射水模体在侧向 45°/90° 角 50 cm 处杂散中子非靶剂量",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "侧向 45/90 度角探测器杂散中子剂量",
      "src": "U"
    },
    "F2": {
      "v": "equivalent",
      "src": "U"
    },
    "F3": {
      "v": "Sv",
      "src": "A"
    },
    "F4": {
      "v": "volume_avg",
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
      "no_sievert",
      "per_source_needs_strength"
    ]
  },
  "pathology_deduction": {
    "material_defect": "基线代码未挂载高能重离子非弹性核反应与次级中子产生模型。",
    "geometry_defect": "基线代码未在侧向 45 度与 90 度角放置偏轴离体杂散辐射探测器。",
    "source_kinematics_defect": "基线代码离子束未配置 Z=6, A=12 及 290 MeV/u 真实动能。",
    "tally_lifecycle_defect": "基线代码未对侧向探测器杂散中子进行解耦统计，且未引入辐射权重因数换算当量剂量。"
  },
  "repairs_applied": {
    "material_repair": "采用 G4_WATER 幻体，注册 QGSP_BIC_HP + G4EmExtraPhysics 全面覆盖碳核碎裂反应。",
    "geometry_repair": "在靶点中心侧向 45 度与 90 度、距离 50 cm 处精准布置两组敏感探测器 (det_45, det_90)。",
    "kinematics_repair": "发射 290 MeV/u C-12 离子束照射水靶中心。",
    "tally_repair": "独立计分两组侧向探测器内的次级杂散中子计数，并乘入中子加权因子 w_R 换算等效剂量 (Sv)。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M10.cc $(geant4-config --cflags --libs) -o /tmp/t2-m10_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m10_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "碳离子侧向散射杂散中子剂量模型完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
