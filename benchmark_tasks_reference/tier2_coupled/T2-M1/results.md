# T2-M1 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少沿轴百分深度剂量 (PDD) 分箱 |
| **B组 (模型自查)** | ❌ 未通过 | 编译通过 | 缺少横向离轴 Profile 分箱 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 均整板与 PDD/Profile 双剖面完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T2-M1",
  "task_name": "6 MV 医用加速器倒圆锥均整板水箱 PDD 与离轴 Profile",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "dose",
      "src": "U"
    },
    "F1b": {
      "v": "水箱沿轴百分深度剂量与横向 Profile",
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
    "material_defect": "基线代码缺失钨均整器与水箱组织等效材料模型。",
    "geometry_defect": "基线代码未构建倒圆锥形均整板（G4Cons）以及 30x30x30 cm 水箱实体。",
    "source_kinematics_defect": "基线代码电子源未对准靶体，且无韧致辐射能量调节。",
    "tally_lifecycle_defect": "基线代码缺失沿轴百分深度剂量（PDD，0.5 cm 分箱）和横向离轴 Profile 分箱计分。"
  },
  "repairs_applied": {
    "material_repair": "采用 G4_W 构建均整器，G4_WATER 构建吸收水箱。",
    "geometry_repair": "构建倒圆锥体均整板 G4Cons(R1=0, R2=5cm, dz=2cm) 与 30cm 水箱。",
    "kinematics_repair": "发射 6 MeV 电子束轰击靶区产生前向韧致辐射并经均整板整形。",
    "tally_repair": "在水箱内建立沿轴深度 (PDD) 与离轴横向 (Profile) 双剖面能量沉积网格计分。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T2-M1.cc $(geant4-config --cflags --libs) -o /tmp/t2-m1_bin",
    "compile_ok": true,
    "run_command": "/tmp/t2-m1_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "均整板与 PDD/Profile 双剖面完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
