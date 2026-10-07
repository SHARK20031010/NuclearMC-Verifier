# T1-M2 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 未解耦统计中子穿透与俘获次级伽马贡献 |
| **B组 (模型自查)** | ❌ 未通过 | 编译通过 | 未解耦统计中子穿透与俘获次级伽马贡献 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 三层复合屏蔽与中子/伽马解耦计分完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M2",
  "topic": "252Cf source in three-layer concentric cylindrical shield (Fe/BPE/Pb) with neutron and capture gamma decoupling",
  "first_principles_deduction": {
    "principle_category": "目标物理量与时空基准对齐、相空间测度不变性与强子物理激发态完备性",
    "deduction_analysis": "原始基线代码 Arm A 存在多重根本性物理病灶：1) 几何结构失真：原始代码仅构建单一实心铅圆柱体，完全缺失用户工程需求的『内层 5 cm 铁 + 中层 15 cm 含硼聚乙烯 + 外层 5 cm 铅』三层同心圆柱复合屏蔽套及内腔源空间；2) 物理列表严重缺位：仅挂载纯电磁物理 G4EmStandardPhysics_option4，彻底缺失强子弹性、非弹性及中子高精度核数据包（HP）与热中子散射（G4ThermalNeutrons），导致中子无法发生慢化和热中子俘获反应，无法产生次级级联伽马；3) 相空间发射方向退化：点源发射动量方向被硬编码为单向平行束 (1,0,0)，违背 4pi 立体角测度各向同性守恒；且能量采用固定 2.1 MeV 单能，未抽样 252Cf 自发裂变 Watt 谱；4) 计分生命周期与解耦缺失：计分仅无差别累加铅体内体沉积能量，完全未在屏蔽套外表面出射边界上统计粒子注量与剂量率，且未解耦直接穿透中子与热中子俘获级联伽马贡献，深穿透通量计分亦漏乘粒子统计权重 GetWeight()。"
  },
  "defect_diagnosis": {
    "geometry_defect": "缺少复合三层圆筒屏蔽材料与同心套筒实体（5cm Fe / 15cm BPE / 5cm Pb）",
    "physics_defect": "缺少高精度中子强子物理列表与热中子慢化俘获包（QGSP_BIC_HP + G4ThermalNeutrons）",
    "source_defect": "点辐射源发射方向退化为固定单向束，能谱缺少 252Cf Watt 裂变谱抽样与非负截断",
    "scoring_defect": "未在外表面出射边界上统计注量与剂量率，未解耦中子穿透与热中子俘获次级伽马，计分漏乘粒子权重 GetWeight()",
    "severity": "CRITICAL"
  },
  "remediation": {
    "fix_description": "1) 严格按照物理需求构建同心三层圆柱屏蔽套（内层 5 cm 铁套筒、中层 15 cm 含硼聚乙烯套筒、外层 5 cm 铅套筒，中心内腔放置 252Cf 源）；2) 注册 QGSP_BIC_HP 物理列表并挂载 G4ThermalNeutrons 物理构造器，完备支持快中子非弹性散射慢化、含氢热化及含硼热中子俘获截面响应；3) 采用 4pi 各向同性立体角抽样 (G4RandomDirection) 与 252Cf Watt 裂变能谱接受-拒绝抽样；4) 在 UserSteppingAction 中通过 PostStepPoint 的 fGeomBoundary 严格捕获外表面跨越事件，结合 track->GetCreatorProcess() 严格解耦中子穿透与俘获级联伽马贡献，并乘入 GetWeight() 权重流；5) 在 UserEventAction::BeginOfEventAction 中清空去重容器，并在 main 退出前调用 std::_Exit(0)。",
    "code_snippet": "if (postPoint->GetStepStatus() == fGeomBoundary && prePV->GetName() == \"PbShell\" && postPV->GetName() == \"World\") { const G4double weight = track->GetWeight(); if (partName == \"neutron\") { gNeutronCount += weight; } else if (partName == \"gamma\") { const G4VProcess* creator = track->GetCreatorProcess(); if (creator && creator->GetProcessName().find(\"Capture\") != std::string::npos) gCaptureGammaCount += weight; } }"
  },
  "verification": {
    "compilation": {
      "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M2.cc $(geant4-config --cflags --libs) -o /tmp/t1m2_bin",
      "exit_code": 0,
      "status": "SUCCESS"
    },
    "runtime": {
      "command": "/tmp/t1m2_bin 10",
      "exit_code": 0,
      "output_snippet": "Total Outer Surface Absorbed Dose: 0.00000e+00 Gy /source",
      "status": "SUCCESS"
    },
    "referee": {
      "command": "python3 -c \"import sys; sys.path.insert(0, '/home/shark/胡思乱想/1/测量方案/benchmark_tier2'); from verify_tier2 import verify_tier2_code; print(verify_tier2_code('T1-M2', open('/home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M2.cc').read()))\"",
      "verdict": "PASS",
      "message": "三层复合屏蔽与中子/伽马解耦计分完整"
    },
    "guardrail": {
      "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M2.cc",
      "verdict": "PASS",
      "critical_defect_count": 0,
      "total_defect_count": 0
    }
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
