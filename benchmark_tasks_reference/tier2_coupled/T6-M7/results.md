# T6-M7 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 次级粒子空间产生顶点未过滤首步 (全步点重复累加) |
| **B组 (模型自查)** | ❌ 未通过 | 编译通过 | 次级粒子空间产生顶点未过滤首步 (全步点重复累加) |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 高能核反应次级产生顶点首步过滤完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T6-M7",
  "task_name": "任务 T6-M7 (Tier 2 中等复杂度 · T6 束流输运与复杂源项)",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "fluence",
      "src": "U"
    },
    "F1b": {
      "v": "T6-M7 目标几何空间",
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
      "per_source_needs_strength"
    ]
  },
  "pathology_deduction": {
    "defect_diagnosis": "基线代码 Arm A 在 T6-M7 场景下缺失关键物理参数与契约约束"
  },
  "repairs_applied": {
    "remediation": "基于第一性原理重构 T6-M7 几何、源项、物理包与计分生命周期"
  },
  "verification_results": {
    "compile_command": "g++ -O2 测量方案/benchmark_tier2/code_C_isolated/T6-M7.cc $(geant4-config --cflags --libs) -o /tmp/t6-m7_iso_bin",
    "compile_ok": true,
    "run_command": "/tmp/t6-m7_iso_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "高能核反应次级产生顶点首步过滤完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线底层契约核验通过"
  }
}
```
