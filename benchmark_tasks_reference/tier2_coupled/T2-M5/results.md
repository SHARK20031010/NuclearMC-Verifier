# T2-M5 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少多叶光栅 MLC 圆弧接触面模型 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | MLC 叶片端面圆弧聚焦漏光分布完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | MLC 叶片端面圆弧聚焦漏光分布完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M5",
  "task_name": "多叶光栅 (MLC) 叶片端面圆弧聚焦的射线漏光与半影",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "MLC 叶片闭合接缝处漏光透射率分布",
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
    "material_defect": "基线代码未构建重钨合金 MLC 叶片。",
    "geometry_defect": "基线代码采用简化平直接触面，丢失了工程 MLC 叶片端面的圆弧聚焦曲率。",
    "source_kinematics_defect": "基线代码射线源未覆盖叶片闭合接缝区域。",
    "tally_lifecycle_defect": "基线代码未以 0.1 mm 高分辨率分箱统计接缝处的微区漏光透射率峰值。"
  },
  "repairs_applied": {
    "material_repair": "采用密度 18.0 g/cm3 的钨合金构建 7 cm 厚度 MLC 叶片。",
    "geometry_repair": "构建带圆弧端面接触几何的双对闭合叶片实体模型。",
    "kinematics_repair": "高能 X 射线束聚焦照射闭合相接缝隙区域。",
    "tally_repair": "以 0.1 mm 微细空间分箱计分接缝处的射线漏光透射剖面。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M5.cc $(geant4-config --cflags --libs) -o /tmp/t2-m5_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m5_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "MLC 叶片端面圆弧聚焦漏光分布完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
