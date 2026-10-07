# T2-M3 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 计算剂量率未除以脉冲持续时间 tau = 2 us |
| **B组 (模型自查)** | ❌ 未通过 | 编译通过 | 计算剂量率未除以脉冲持续时间 tau = 2 us |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | FLASH 超高剂量率单脉冲峰值剂量率完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M3",
  "task_name": "FLASH 超高剂量率单脉冲电子束瞬间吸收剂量率分布",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "FLASH 脉冲电子打入水箱瞬时峰值吸收剂量率",
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
      "v": "instant",
      "src": "U"
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
    "material_defect": "基线代码未配置大容量水介质吸收体。",
    "geometry_defect": "基线代码仅为大块单一实体，无空间网格剂量剖面。",
    "source_kinematics_defect": "基线代码缺少微秒级单脉冲电荷量与脉冲时间关联。",
    "tally_lifecycle_defect": "基线代码计算剂量率未除以脉冲持续时间 tau = 2 us，导致瞬时峰值剂量率失真。"
  },
  "repairs_applied": {
    "material_repair": "配置 G4_WATER 吸收体，标准 G4EmStandardPhysics_option4 物理列表。",
    "geometry_repair": "构建 10x10x10 cm 水靶区吸收体。",
    "kinematics_repair": "发射 10 MeV 电子单脉冲，关联微秒脉冲电荷与电子总数。",
    "tally_repair": "在 UserSteppingAction 中累加总沉积能量，除以脉冲宽度 tau=2.0*us 换算瞬时峰值吸收剂量率 (Gy/s)。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M3.cc $(geant4-config --cflags --libs) -o /tmp/t2-m3_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m3_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "FLASH 超高剂量率单脉冲峰值剂量率完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
