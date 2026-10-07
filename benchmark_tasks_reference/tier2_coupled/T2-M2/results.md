# T2-M2 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少多能量加权调制形成展宽 Bragg 峰 (SOBP) |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 质子束 SOBP 展宽峰与深度曲线完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 质子束 SOBP 展宽峰与深度曲线完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M2",
  "task_name": "100 MeV 质子束在人体组织等效水中的扩束 Bragg 峰 (SOBP)",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "水箱质子展宽 Bragg 峰 (SOBP) 深度剂量分布",
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
    "material_defect": "基线代码未指定高精度水介质质子停止本领模型。",
    "geometry_defect": "基线代码水箱缺少沿入射轴向的微细深度网格剖分。",
    "source_kinematics_defect": "基线代码仅发射单一单能质子，无法展宽 Bragg 峰，形成尖锐远端过冲。",
    "tally_lifecycle_defect": "基线代码未实现多能量分量加权合成及 SOBP 平台区平坦度校验。"
  },
  "repairs_applied": {
    "material_repair": "采用标准 G4_WATER，配置精确电离能损与多重库仑散射。",
    "geometry_repair": "构建水箱并细分深度分箱层。",
    "kinematics_repair": "通过三组能量分量 (100 MeV, 85 MeV, 70 MeV) 加权抽样合成展宽 Bragg 峰。",
    "tally_repair": "沿深度轴向计分展宽平坦区 (~2 cm) 吸收剂量分布曲线。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M2.cc $(geant4-config --cflags --libs) -o /tmp/t2-m2_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m2_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "质子束 SOBP 展宽峰与深度曲线完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
