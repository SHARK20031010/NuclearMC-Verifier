# T2-M7 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少依据中子动能的 ICRP 103 连续加权 |
| **B组 (模型自查)** | ❌ 未通过 | 编译通过 | 缺少依据中子动能的 ICRP 103 连续加权 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 人体参考模型多器官等效剂量计算完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M7",
  "task_name": "软组织/骨/肺三层人体躯干 ICRP 103 动能非线性加权器官等效剂量",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "人体模型软组织/骨/肺各器官等效剂量",
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
    "material_defect": "基线代码未配置骨骼与肺低密度组织材料组分。",
    "geometry_defect": "基线代码未构建软组织、骨与肺组织三层解耦躯干切片几何。",
    "source_kinematics_defect": "基线代码未抽样宽束快中子连续动能。",
    "tally_lifecycle_defect": "基线代码未依据中子瞬态动能应用 ICRP 103 连续权重函数 w_R(E) 积分剂量当量 (Sv)。"
  },
  "repairs_applied": {
    "material_repair": "构建 ICRU 骨骼、肺低密度组织与软组织材料。",
    "geometry_repair": "构建三层人体躯干平板切片几何（各层独立放置）。",
    "kinematics_repair": "宽束中子从前方垂向入射轰击躯干模型。",
    "tally_repair": "结合 ICRP 103 动能连续函数 w_R(E) = 2.5 + 18.2*exp(-(ln(E))^2/6) 逐步积分各器官剂量当量 (Sv)。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M7.cc $(geant4-config --cflags --libs) -o /tmp/t2-m7_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m7_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "人体参考模型多器官等效剂量计算完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
