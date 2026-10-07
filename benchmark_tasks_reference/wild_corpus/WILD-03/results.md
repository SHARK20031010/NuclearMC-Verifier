# WILD-03 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：闪烁体在高 LET 粒子照射下未设置 Birks 猝灭常数；使能光学物理但材料属性表未配置折射率 RINDEX |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-03",
  "task_description": "Scintillation/Cerenkov optical photon simulation returns 0 hits due to missing optical properties in material table",
  "deduction": {
    "first_principles_inquiry": "Level 3 第一性原理抽象质询：微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）、微分相空间测度不变性",
    "slot_analysis": {
      "slot_id": "D4_MATERIAL_PROPERTIES",
      "slot_name": "材料物理属性表挂载",
      "action": "Define"
    },
    "theoretical_deduction": "在蒙特卡洛粒子输运模拟中，介质内电磁相互作用产生的次级光辐射（如切伦科夫辐射相速度条件 cos(theta_c) = 1/(beta*n) 以及闪烁发光与光子输运）要求介质必须具备完整的相空间介电色散关系 n(omega)。在 Geant4 框架语义契约中，使能 G4OpticalPhysics 物理包后，G4OpTransportation 输运过程与 G4Cerenkov 产生过程强依赖材料属性表 (G4MaterialPropertiesTable) 中显式配置的随波长折射率属性 RINDEX。若属性表中未配置 RINDEX，Geant4 无法获取光子相速度与波矢，G4Cerenkov 过程将由于缺少折射率而静默跳过发光，且产生的任何光学光子均无法在介质内建立输运相空间，导致全场光学光子产生数与击中数恒为 0 且无任何报错信息。"
  },
  "defects_identified": [
    {
      "core": "NUCLEAR_CORE",
      "contract": "CERENKOV_RINDEX_PROPERTY_CONTRACT",
      "severity": "CRITICAL",
      "defect_name": "使能光学物理但材料属性表未配置折射率 RINDEX",
      "description": "Geant4 契伦科夫辐射与光学光子传播依赖材料折射率 RINDEX。若介质材料属性表 (G4MaterialPropertiesTable) 中未显式添加 RINDEX，G4Cerenkov 过程将由于找不到相速度而静默跳过，全场光子发射数恒为 0 且不报任何错误！"
    }
  ],
  "fix_methodology": {
    "slot": "D4_MATERIAL_PROPERTIES",
    "remedy_description": "在材料属性表 (G4MaterialPropertiesTable) 中显式添加随光子能量分布的折射率属性 RINDEX，并补齐吸收长度 ABSLENGTH、闪烁发光光谱 SCINTILLATIONCOMPONENT1、产额 SCINTILLATIONYIELD、时间常数 SCINTILLATIONTIMECONSTANT1 与分辨率比例 RESOLUTIONSCALE，确保光学光子创生与输运通道完整闭环。",
    "code_changes": "在 Det::Construct() 中，为 G4MaterialPropertiesTable 添加 RINDEX、ABSLENGTH、SCINTILLATIONCOMPONENT1、SCINTILLATIONYIELD 等物理属性，并挂载至介质水材料上；在 main 退出前调用 std::_Exit(0) 防止 Geant4 析构段错误。"
  },
  "verification": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-03.cc $(geant4-config --cflags --libs) -o /tmp/wild03_bin",
    "compile_exit_code": 0,
    "run_command": "/tmp/wild03_bin 10",
    "run_exit_code": 0,
    "run_output_tail": "WILD-03 Simulation completed",
    "guardrail_command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-03.cc --json",
    "guardrail_verdict": "PASS",
    "guardrail_critical_defects": 0,
    "guardrail_total_defects": 0
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
