# T2-M6 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少眼球巩膜与敷贴器解耦计分 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 眼底黑色素瘤敷贴器巩膜剂量模型完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 眼底黑色素瘤敷贴器巩膜剂量模型完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M6",
  "task_name": "眼眶黑色素瘤 125I 放射性粒子敷贴器巩膜与视神经剂量",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "眼球巩膜靶区与视神经敏感区吸收剂量",
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
    "material_defect": "基线代码未设置眼球组织与黄铜敷贴器材料。",
    "geometry_defect": "基线代码未构建眼球球体、巩膜外壳与后极部视神经解耦几何。",
    "source_kinematics_defect": "基线代码未布置 5 颗 125I 密封籽源空间阵列与特征低能伽马谱。",
    "tally_lifecycle_defect": "基线代码未解耦统计巩膜靶区与后极部视神经敏感区的吸收剂量比。"
  },
  "repairs_applied": {
    "material_repair": "构建眼球水组织、巩膜与黄铜敷贴器 (Eye Plaque)。",
    "geometry_repair": "构建眼球球体几何，外附敷贴器，并解耦巩膜靶区与视神经体积。",
    "kinematics_repair": "在敷贴器内均匀排布 5 颗 125I 籽源，发射 27-35 keV 特征光子。",
    "tally_repair": "在 SteppingAction 中分别计分巩膜与视神经的吸收剂量并计算剂量比值。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M6.cc $(geant4-config --cflags --libs) -o /tmp/t2-m6_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m6_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "眼底黑色素瘤敷贴器巩膜剂量模型完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
