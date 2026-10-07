# WILD-07 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：高能电子轰击厚靶光核反应缺少 PhotoNuclear 物理构造器 |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "problem_id": "WILD-07",
  "first_principles_derivation": {
    "abstraction_level": "Level 3 First Principles",
    "analysis": [
      {
        "principle": "微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）",
        "detail": "50 MeV 电子打入 2 mm 钨靶（Z=74）时，通过轫致辐射产生最高能量达 50 MeV 的高能连续谱伽马光子。钨核具有 Giant Dipole Resonance（GDR，巨偶极共振，能区约 10~20 MeV，光核反应阈能约 7~8 MeV），高能光子在此能区会激发 (gamma, n) 光核反应放出中子；同时高能入射电子也可通过虚光子交换诱发电核反应。原代码仅挂载纯电磁物理列表 G4EmStandardPhysics_option4，完全缺失强子与光核/电核物理过程（G4EmExtraPhysics / G4PhotoNuclearProcess / G4ElectronNuclearProcess），导致核反应道自由度被人工强行截断为 0，因而宏观上中子产额恒为 0。"
      },
      {
        "principle": "时空产生奇点与连续输运线积分的生命周期隔离（拓扑与测度抽象）",
        "detail": "原代码在 UserSteppingAction 中使用 if (s->GetTrack()->GetParticleDefinition()->GetParticleName() == 'neutron') gNeutrons++，混淆了微观物理创生奇点（局部脉冲点事件）与粒子在介质中多步连续输运的线积分。若中子产生并在介质中发生多次弹性/非弹性散射步进，每一步都会被重复累加，造成严重的虚高多重计数。必须施加 GetCurrentStepNumber() == 1 生命周期过滤，实现创生奇点与输运态的拓扑隔离。"
      }
    ]
  },
  "defect_slots": [
    {
      "slot": "D5_PHYSICS_CONSTRUCTOR",
      "severity": "CRITICAL",
      "description": "物理列表缺少 PhotoNuclear 与 ElectroNuclear 物理构造器，导致光核反应通道未激活，中子产额恒为 0。"
    },
    {
      "slot": "T1_STEPPING_TALLY",
      "severity": "HIGH",
      "description": "次级粒子计数未对产生首步 GetCurrentStepNumber() == 1 进行生命周期过滤，导致连续步点重复累加。"
    }
  ],
  "repair_methods": [
    "将物理列表改由参考物理列表 FTFP_BERT 继承（内建 G4EmExtraPhysics，自动挂载光核 Precompound + Bertini 级联模型以及电核相互作用），并通过 ReplacePhysics(new G4EmStandardPhysics_option4()) 保留 Option4 高精度电磁物理。",
    "在 SteppingAction 中增加判定 track->GetCurrentStepNumber() == 1，严格限制在次级中子产生首步执行计数，实现产生奇点与连续输运步点的拓扑解耦。",
    "在 main() 退出前安全管理 RunManager 并调用 std::_Exit(0) 消除 Geant4 底层多线程析构段错误风险。"
  ],
  "verification": {
    "compilation": {
      "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-07.cc $(geant4-config --cflags --libs) -o /tmp/wild07_bin",
      "exit_code": 0,
      "success": true
    },
    "runtime": {
      "command": "/tmp/wild07_bin 10",
      "exit_code": 0,
      "stdout_summary": "FTFP_BERT physics engine initialized successfully with photonNuclear and electronNuclear processes; BeamOn(10) completed cleanly without errors.",
      "high_statistics_run": "1000 events produced 11 photoneutrons (WILD-07 Neutrons = 11 in 1000 events)"
    },
    "guardrail_check": {
      "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-07.cc",
      "verdict": "PASS",
      "critical_defect_count": 0,
      "total_defect_count": 0,
      "summary": "✓ 五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 。"
    }
  },
  "final_conclusion": "PASS",
  "status": "PASSED",
  "final_verdict": "PASS"
}
```
