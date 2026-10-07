# WILD-10 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：强偏转磁场未配置 G4ChordFinder 步进弦长限制 |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-10",
  "prompt_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/prompts_C_pure/WILD-10.md",
  "code_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-10.cc",
  "audit_log_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/results_isolated/WILD-10_audit.json",
  "first_principles_analysis": {
    "physics_background": "100 MeV 质子在 1.5 T 偶极弯转磁场中的输运。100 MeV 质子总能量 E ≈ 1038.27 MeV，相对论动量 p ≈ 444.58 MeV/c，在 1.5 T 垂直磁场下的弯转半径 R = p / (qB) ≈ 0.988 m。质子在进入磁场后应发生顺滑圆弧弯转并偏向 -x 方向。",
    "first_principles_inquiry": "【微分相空间测度不变性与几何边界数值逼近】连续空间相空间轨道在离散步进积分映射时，弯曲轨迹被直线弦（Chord）逼近以寻找几何交点。若弦与弧的最大偏差（Sagitta，矢高 delta chord）容差过大，离散化微元将在边界处发生非线性跨越畸变，导致粒子跨越曲面/边界交点直接跳入外部世界，破坏边界相空间流守恒与轨道拓扑连续性。",
    "defect_slot": "D1/D5-MEASURE_CORE 磁场轨道数值积分步长契约 (MAGNETIC_CHORD_STEP_INTEGRATION_CONTRACT)",
    "defect_name": "强偏转磁场未配置 G4ChordFinder 步进弦长限制",
    "defect_description": "强磁铁偏转区域仅创建了 G4FieldManager，但未对 G4ChordFinder 配置最大弦长限制 (SetDeltaChord) 及交点容差 (SetDeltaIntersection)。Geant4 默认步进弦长容差较大，质子在强弯曲圆弧轨道推进时单步弦长过大，跳跃穿过了磁铁/真空室边界，导致粒子失真丢失。",
    "repair_strategy": "在 Det::Construct() 中，为 fieldMgr 显式创建并绑定 G4ChordFinder，设置弦长最大容差 fieldMgr->GetChordFinder()->SetDeltaChord(0.1 * mm)，并同步精细化交点精度 SetDeltaIntersection(0.001 * mm) 和单步精度 SetDeltaOneStep(0.01 * mm)，使磁场轨道数值积分严格保持与几何边界的微米级精确求交。"
  },
  "compilation_and_execution": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-10.cc $(geant4-config --cflags --libs) -o /tmp/wild10_bin",
    "compile_exit_code": 0,
    "run_command": "/tmp/wild10_bin 10",
    "run_exit_code": 0,
    "run_stdout_tail": "WILD-10 Dipole transport completed"
  },
  "guardrail_check": {
    "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-10.cc --json",
    "verdict": "PASS",
    "critical_defect_count": 0,
    "total_defect_count": 0,
    "defects_by_core": {
      "intent_core": [],
      "causality_core": [],
      "measure_core": [],
      "variance_core": [],
      "lifecycle_core": [],
      "nuclear_core": []
    }
  },
  "final_conclusion": "PASS",
  "status": "PASSED",
  "final_verdict": "PASS"
}
```
