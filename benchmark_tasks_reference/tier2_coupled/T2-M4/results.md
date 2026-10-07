# T2-M4 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少毫米级微细网格半影剖面分箱 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 小野微型圆锥准直半影与输出因子完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 小野微型圆锥准直半影与输出因子完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M4",
  "task_name": "小野立体定向放射外科 (SRS) 锥光束离轴剂量跌落",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "微型圆锥准直器水箱半影区与输出因子",
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
    "material_defect": "基线代码缺少高密度钨合金微型准直器材料。",
    "geometry_defect": "基线代码缺失 5 mm 直径微型圆锥准直器实体孔道。",
    "source_kinematics_defect": "基线代码未对准微型孔径发射，立体角外粒子泄漏严重。",
    "tally_lifecycle_defect": "基线代码缺少毫米/亚毫米级微细网格半影 (Penumbra 80%-20%) 剖面分箱与输出因子统计。"
  },
  "repairs_applied": {
    "material_repair": "采用 G4_W 钨材料构建微型锥形准直管。",
    "geometry_repair": "构建直径 5 mm 钨圆锥准直孔及下游水箱。",
    "kinematics_repair": "6 MV 伽马光子经圆锥孔道严格准直入射。",
    "tally_repair": "以 0.2 mm 亚毫米级横向步长分箱计分横剖面，提取 80%-20% 半影跌落宽度与输出因子。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M4.cc $(geant4-config --cflags --libs) -o /tmp/t2-m4_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m4_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "小野微型圆锥准直半影与输出因子完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
