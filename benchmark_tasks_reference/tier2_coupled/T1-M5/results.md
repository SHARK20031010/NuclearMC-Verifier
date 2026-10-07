# T1-M5 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少沿罐壁轴向高度方向的剂量率剖面分箱 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 储罐双壁与轴向剂量率剖面完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 储罐双壁与轴向剂量率剖面完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M5",
  "task_category": "T1 屏蔽与深穿透",
  "description": "圆柱形放射性废液储罐（内胆 1 cm 不锈钢，外包 20 cm 轻水，最外层 2 cm 铅屏蔽）。源为均匀溶于水中的 137Cs，计算罐外侧表面沿轴向高度分布的剂量率剖面。",
  "target_observable": "储罐同心双壁铁水复合屏蔽透射曲线",
  "source_code_path": "/home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M5.cc",
  "compilation": {
    "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M5.cc $(geant4-config --cflags --libs) -o /tmp/t1m5_bin",
    "status": "SUCCESS",
    "exit_code": 0
  },
  "execution": {
    "command": "/tmp/t1m5_bin 10",
    "status": "SUCCESS",
    "exit_code": 0
  },
  "verify_tier2": {
    "status": true,
    "verdict": "储罐双壁与轴向剂量率剖面完整"
  },
  "guardrail": {
    "verdict": "PASS",
    "critical_defect_count": 0,
    "total_defect_count": 0,
    "intent_status": {
      "has_intent": true,
      "stats": {
        "user_decisions": 7,
        "ai_decisions": 4,
        "ai_decision_ratio": "4/11",
        "declared_warnings": [
          "per_source_needs_strength"
        ]
      },
      "defect_count": 0
    },
    "cores_status": {
      "intent_core": "PASS (0 defects)",
      "causality_core": "PASS (0 defects)",
      "measure_core": "PASS (0 defects)",
      "variance_core": "PASS (0 defects)",
      "lifecycle_core": "PASS (0 defects)",
      "nuclear_core": "PASS (0 defects)"
    }
  },
  "physics_diagnosis_and_fixes": {
    "defect_1_geometry": {
      "diagnosis": "Arm A 仅定义了单个 50 cm 水柱，完全缺失 1 cm 不锈钢内胆、20 cm 轻水屏蔽外包、2 cm 铅外屏蔽及罐外侧监测探测层实体。",
      "fix": "采用高精度同心同轴多层圆柱壳结构依次构建：水核 (R=0~50cm) -> 不锈钢内胆 (R=50~51cm) -> 轻水屏蔽 (R=51~71cm) -> 铅屏蔽 (R=71~73cm) -> 罐外表面探测层 (R=73~74cm)。"
    },
    "defect_2_source_sampling": {
      "diagnosis": "Arm A 使用原点 (0,0,0) 的固定单向 (1,0,0) 笔形伽马点源，严重破坏体源相空间均匀性与 4pi 立体角测度。",
      "fix": "构建 137Cs 均匀溶于水体的柱坐标体源：采用雅可比反变换 r = R * sqrt(xi) 确保面积/体积测度守恒，z 轴向均匀分布，并采用 G4RandomDirection() 实现 4pi 全立体角各向同性发射。"
    },
    "defect_3_z_profile_tally": {
      "diagnosis": "Arm A 仅累加全空间能量沉积标量，完全缺乏罐外侧表面沿轴向高度方向的分布解耦。",
      "fix": "在 SteppingAction 中通过 PostStepPoint 的 fGeomBoundary 判定自铅层出射到外表面探测层的粒子，在 z 轴划设 20 个区间进行高度剖面分箱统计 (NUM_Z_BINS=20)，并在 RunAction 中按侧表面积归一化输出透射通量剖面。"
    },
    "defect_4_variance_reduction_weight": {
      "diagnosis": "Arm A 未考虑深穿透屏蔽下的粒子权重统计，在深穿透模拟或重要性抽样下无法维持期望无偏。",
      "fix": "在步进计分中严格调用 step->GetTrack()->GetWeight() 进行权重累加，完全满足深穿透权重流守恒契约。"
    },
    "defect_5_intent_alignment": {
      "diagnosis": "缺少环 0 需求确认回执块，无法约束出射面通量与源粒子归一化基准。",
      "fix": "头文件前置规范的 guardrail-intent 结构化回执块，并在输出中严格保持每源粒子出射表面通量量纲一致。"
    }
  },
  "overall_conclusion": "T1-M5 第一性原理重构与自主修复全部完成，原生编译运行无段错误退出 (Exit 0)，客观真值裁判与认知护栏六大总线全部满分通过。",
  "status": "PASSED"
}
```
