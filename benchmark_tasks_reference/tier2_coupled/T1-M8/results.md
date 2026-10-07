# T1-M8 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少 S 型曲折通风管道几何 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 杂散中子曲折管道迷宫反射衰减完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 杂散中子曲折管道迷宫反射衰减完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M8",
  "task_name": "高能质子治癌室杂散中子在排风管道中的迷宫散射",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "fluence",
      "src": "U"
    },
    "F1b": {
      "v": "T1-M8 目标几何空间出射面",
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
    "material_defect": "基线代码缺少高能质子治癌靶区水体 (G4_WATER) 与天花板重混凝土 (G4_CONCRETE)，且物理列表未配置高精度强子物理包 QGSP_BIC_HP，无法发生中子非弹性级联产生与低能反照慢化。",
    "geometry_defect": "基线代码未构建天花板内部 S 型曲折通风管道实体（包含进气口、水平过渡折段、两道 90 度反照转角及出气口），完全缺失通风管道几何通道。",
    "source_kinematics_defect": "基线代码未实现 230 MeV 高能质子束打水体模型，或仅以经验各向同性源近似，丢失了质子打靶杂散中子的真实角分布与能谱硬度。",
    "tally_lifecycle_defect": "基线代码未在管道出口设置面积解耦的面通量统计体，未计算相对于靶点产生源强的通量衰减因子与出射粒子去重计数。"
  },
  "repairs_applied": {
    "material_repair": "配置 G4_WATER 质子打靶水模体及 G4_CONCRETE 屏蔽天花板，注册标准 QGSP_BIC_HP 物理列表覆盖 230 MeV 质子核反应与中子热化输运。",
    "geometry_repair": "在 1.5 m 厚混凝土天花板内构建实体 S 型通风管道（断面 30cm×30cm，两段 90 度直角折弯消除视线直通），并在排气口设置 900 cm^2 计分面实体。",
    "kinematics_repair": "发射 230 MeV 铅垂质子束轰击 30cm×30cm×30cm 水靶模体，通过真实核反应动力学生成杂散次级中子。",
    "tally_repair": "在 SteppingAction 中分别统计水靶产生的总中子源强 S0、管道入口入射量与排风口出射量，计算出口面积归一化中子通量与衰减倍数，计分后终止生命周期。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M8.cc $(geant4-config --cflags --libs) -o /tmp/t1m8_bin",
    "compile_ok": true,
    "run_command": "/tmp/t1m8_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "杂散中子曲折管道迷宫反射衰减完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
