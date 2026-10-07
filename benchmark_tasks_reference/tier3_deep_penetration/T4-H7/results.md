# T4-H7 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少像素化半导体 ToT 电荷与 ToA 时间戳模型 |
| **B组 (模型自查)** | ❌ 未通过 | 编译通过 | 缺少像素化半导体 ToT 电荷与 ToA 时间戳模型 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 像素半导体 Timepix3 ToT 与 ToA 模型完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T4-H7",
  "task_name": "任务 T4-H7 (Tier 3 高等复杂度 · T4 探测器与符合)",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "efficiency",
      "src": "U"
    },
    "F1b": {
      "v": "T4-H7 目标几何空间",
      "src": "U"
    },
    "F2": {
      "v": "absolute",
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
    "defect_diagnosis": "基线代码在 T4-H7 场景下存在深穿透/生命周期/权重流或非线性响应契约缺陷"
  },
  "repairs_applied": {
    "remediation": "基于 Level 3 第一性原理五大守恒总线重构 T4-H7 几何、物理包、计分生命周期与权重流"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier3/code_C_isolated/T4-H7.cc $(geant4-config --cflags --libs) -o /tmp/t4-h7_iso_bin",
    "compile_ok": true,
    "run_command": "/tmp/t4-h7_iso_bin 10",
    "run_exit_code": 0,
    "run_output_snippet": "[T4-H7] Timepix3 ToT & ToA Hits: 10",
    "verify_tier3_passed": true,
    "verify_tier3_reason": "像素半导体 Timepix3 ToT 与 ToA 模型完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
