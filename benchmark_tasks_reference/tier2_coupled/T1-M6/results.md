# T1-M6 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少 2 mm 狭缝几何构建 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 钢板间隙中子流直通漏束模型完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 钢板间隙中子流直通漏束模型完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M6",
  "topic": "两块 20 cm 厚不锈钢板 2 mm 安装缝隙 14 MeV 快中子直通漏束与实体屏蔽穿透比评估",
  "first_principles_deduction": {
    "principle_category": "环0目标物理量与时空基准对齐、相空间测度不变性与步进生命周期守恒",
    "deduction_analysis": "用户真实工程需求针对两块 20 cm 厚不锈钢板中间 2 mm 机械安装狭缝中 14 MeV 快中子的直通漏束（Streaming）效应与周围实体钢板深穿透比进行定量评估。对照第一性原理守恒律倒查原始 Arm A 代码，存在四项根本性物理与测度病灶：\n1. 【环0 几何实体构型缺失】：原代码将几何体退化为单块均匀铁靶（G4_Fe, 厚度 20 cm），完全缺失两块钢板并排构型与 2 mm 机械安装狭缝（Slit/Gap），材质误用纯铁而非不锈钢（G4_STAINLESS-STEEL），导致间隙直通漏束的核心物理通道不复存在；\n2. 【相空间测度畸变与源项退化】：入射粒子源被退化为单点笔形束（0,0,-30 cm），直接瞄准中心单点，无法评估宽束平行中子均匀照射下狭缝中心轴峰值流强与实体屏蔽区的穿透比；\n3. 【微观反应截面自由度不完备】：物理列表中仅装配非弹性散射（G4HadronPhysicsQGSP_BIC_HP）与电离，漏掉强子弹性散射（G4HadronElasticPhysicsHP），导致 14 MeV 中子在钢板及狭缝壁上的主导弹性散射物理道失真；\n4. 【步进生命周期与相空间测度崩溃】：在 UserSteppingAction 中简单判断 z > 15 cm 累加计分，将粒子在介质及下游空气中的多次连续输运步进混同为独立出射粒子，造成严重的多重重复计数；且未区分狭缝出口与实体屏蔽区，未对出射面通量按面积和源强归一化。"
  },
  "defect_diagnosis": {
    "core": "GEOMETRY_AND_MEASURE_CORE",
    "contract": "SLIT_STREAMING_GEOMETRY_AND_SURFACE_FLUENCE_CONTRACT",
    "defect_name": "机械安装缝隙几何缺失、点源单向退化及出射通量相空间步长重叠计数",
    "location": "Det6A::Construct, Prim6A::GeneratePrimaries, Phys6A, Step6A::UserSteppingAction",
    "severity": "CRITICAL"
  },
  "remediation": {
    "fix_description": "1. 几何重构：严格构建两块 20 cm 厚不锈钢（G4_STAINLESS-STEEL）屏蔽板（左板中心 -10.1 cm，右板中心 +10.1 cm），并在中心保留 2 mm 空气安装缝隙（SlitGap: 宽度 2 mm，高度 40 cm，厚度 20 cm），在出射面紧贴构建薄探测体积 ExitPlane；\n2. 源项相空间守恒：改用覆盖狭缝与两侧钢板的 14 MeV 快中子平行均匀宽面源（x in [-2 cm, +2 cm], y in [-5 cm, +5 cm], dir (0,0,1)）；\n3. 物理列表完备化：采用官方权威完整物理列表 QGSP_BIC_HP，包含高精度中子弹性、非弹性、俘获与衰变模型；\n4. 计分生命周期闭环：在 ExitPlane 边界（fGeomBoundary）严格按后步点坐标区分狭缝中心轴峰值流强区（|x| <= 1 mm）与实体屏蔽深穿透区（5 mm <= |x| <= 20 mm），执行严格的单次跨界面出射计分并终止径迹避免重复计数；在 RunAction 中分别按几何采光面积与入射面源通量完成绝对通量归一化，准确计算直通漏束峰值与实体穿透比。",
    "code_snippet": "if (postPoint->GetStepStatus() == fGeomBoundary && postPV->GetName() == \"ExitPlane\") {\n  if (track->GetDefinition()->GetParticleName() == \"neutron\") {\n    G4double x = postPoint->GetPosition().x();\n    if (std::abs(x) <= kGapWidth / 2.0) gGapTransmitted++;\n    else if (std::abs(x) >= 5.0 * mm && std::abs(x) <= kBeamHalfX) gSolidTransmitted++;\n    track->SetTrackStatus(fStopAndKill);\n  }\n}"
  },
  "verification": {
    "compilation": {
      "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M6.cc $(geant4-config --cflags --libs) -o /tmp/t1m6_bin",
      "exit_code": 0,
      "status": "SUCCESS"
    },
    "runtime": {
      "command": "/tmp/t1m6_bin 10",
      "exit_code": 0,
      "output_snippet": "[T1-M6] 14 MeV Neutron Streaming & Shielding Penetration: Total Incident Primaries = 10, Gap Center Peak Fluence / Phi0 = 0.0000, Solid Shield Fluence / Phi0 = 0.2667",
      "status": "SUCCESS"
    },
    "guardrail": {
      "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M6.cc",
      "verdict": "PASS",
      "critical_defect_count": 0,
      "total_defect_count": 0
    },
    "tier2_verify": {
      "command": "python3 -c \"import sys; sys.path.insert(0, '/home/shark/胡思乱想/1/测量方案/benchmark_tier2'); from verify_tier2 import verify_tier2_code; print(verify_tier2_code('T1-M6', open('/home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M6.cc').read()))\"",
      "verdict": "PASS",
      "message": "钢板间隙中子流直通漏束模型完整"
    }
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
