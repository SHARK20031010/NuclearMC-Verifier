# T2-M9 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少水介质径向距离分箱剂量函数 g(r) |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 192Ir 之后装径向剂量函数 g(r) 计算完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 192Ir 之后装径向剂量函数 g(r) 计算完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M9",
  "task_name": "TG-43 协议 192Ir 后装源水介质径向距离剂量函数 g(r)",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "192Ir 施源管周围径向剂量函数 g(r)",
      "src": "U"
    },
    "F2": {
      "v": "absorbed",
      "src": "U"
    },
    "F3": {
      "v": "Gy",
      "src": "A"
    },
    "F4": {
      "v": "distribution",
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
      "v": "curve",
      "src": "U"
    },
    "warnings": [
      "per_source_needs_strength"
    ]
  },
  "pathology_deduction": {
    "material_defect": "基线代码未构建标准近距离治疗水介质幻体。",
    "geometry_defect": "基线代码缺少 0.5 cm 到 5 cm 范围内的同心同轴径向球壳/柱壳几何分箱。",
    "source_kinematics_defect": "基线代码缺少 192Ir 复杂多线衰变谱抽样与封装胶囊几何。",
    "tally_lifecycle_defect": "基线代码未按 TG-43 公式归一化计算径向剂量函数 g(r) = D(r)/(D(r0)*(r0/r)^2)。"
  },
  "repairs_applied": {
    "material_repair": "采用大尺寸水体幻体 (G4_WATER) 模拟无限近距离介质。",
    "geometry_repair": "在源中心周围划分多层同心球壳检测区 (0.5 cm ~ 5.0 cm)。",
    "kinematics_repair": "发射 192Ir 级联光子谱 (均能 ~380 keV) 模拟密封籽源。",
    "tally_repair": "计分各径向距离处的吸收剂量，并严格按 TG-43 几何因子换算径向剂量函数 g(r)。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M9.cc $(geant4-config --cflags --libs) -o /tmp/t2-m9_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m9_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "192Ir 之后装径向剂量函数 g(r) 计算完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
