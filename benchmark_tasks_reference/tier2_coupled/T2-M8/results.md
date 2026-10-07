# T2-M8 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少钼滤过板与铍窗过滤层 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 低能 X 射线管微米焦点钼靶与铍窗出射谱完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 低能 X 射线管微米焦点钼靶与铍窗出射谱完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M8",
  "task_name": "40 kV 电子打钼靶经 0.8 mm 铍窗和 0.03 mm 钼滤过板出射能谱与表面吸收剂量",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "微米焦点钼靶出射特征谱与表面吸收剂量",
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
    "material_defect": "基线代码缺少高纯金属铍 (Be) 与钼滤过层材料设置。",
    "geometry_defect": "基线代码缺少微米焦点钼阳极斜面靶、0.8 mm 铍窗与 0.03 mm 钼滤过板层叠结构。",
    "source_kinematics_defect": "基线代码电子束能量与打靶几何未成 45 度特征出射角。",
    "tally_lifecycle_defect": "基线代码未统计滤波后特征 K 射线能谱及空气参考点表面吸收剂量。"
  },
  "repairs_applied": {
    "material_repair": "配置高纯 G4_Be (0.8 mm) 与 G4_Mo (0.03 mm) 过滤层及 G4_AIR 参考体。",
    "geometry_repair": "构建电子管阳极钼靶、出线铍窗与钼滤过层及表面计分探测器。",
    "kinematics_repair": "40 keV 电子轰击钼靶激发前向轫致辐射与 Mo 特征 X 射线。",
    "tally_repair": "计分滤波出射 X 射线微细能谱及紧邻空气处的表面吸收剂量 (Gy)。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M8.cc $(geant4-config --cflags --libs) -o /tmp/t2-m8_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m8_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "低能 X 射线管微米焦点钼靶与铍窗出射谱完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
