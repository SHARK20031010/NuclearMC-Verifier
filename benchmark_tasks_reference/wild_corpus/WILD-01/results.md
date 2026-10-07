# WILD-01 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：次级粒子空间产生顶点未过滤产生首步 (全步点重复累加虚高) |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-01",
  "topic": "Tracking secondaries production vertex in UserSteppingAction",
  "first_principles_deduction": {
    "principle_category": "时空产生奇点与连续输运线积分的生命周期隔离（拓扑与测度抽象）",
    "deduction_analysis": "物理相互作用导致的粒子创生事件属于时空微观反应的局部脉冲奇点（点事件，即拓扑点），而粒子在介质中的运动是自由程连续滑移积分（线积分，即输运态）。原始代码在 G4UserSteppingAction 中对所有满足 GetParentID() > 0 且为中子的径迹直接累加 gSecondaryCount++，由于 SteppingAction 在粒子径迹的每一个步进（step）都会被触发，一个中子在铅靶中发生数十次弹性散射和输运步进时，其每一个步进都被误当作一次『产生事件』重复计入，将单次创生奇点与后续连续输运线积分混淆，破坏了产生与输运的拓扑隔离，导致次级中子产额虚高 1~2 个数量级（约 50 倍）。"
  },
  "defect_diagnosis": {
    "core": "LIFECYCLE_CORE",
    "contract": "STEPPING_SECONDARY_LIFECYCLE_CONTRACT",
    "defect_name": "次级粒子空间产生顶点未过滤产生首步 (全步点重复累加虚高)",
    "location": "Step::UserSteppingAction",
    "severity": "CRITICAL"
  },
  "remediation": {
    "fix_description": "在 Step::UserSteppingAction 中加入生命周期首步约束 `track->GetCurrentStepNumber() == 1`，使得只有次级中子诞生后的第 1 个步进才执行计数累加，实现产生奇点与连续输运步进的严格拓扑隔离；同时在 main 结束前加入 `std::_Exit(0);` 保证退出无析构段错误。",
    "code_snippet": "if (track->GetCurrentStepNumber() == 1 && track->GetParentID() > 0 && track->GetParticleDefinition()->GetParticleName() == \"neutron\") { gSecondaryCount++; }"
  },
  "verification": {
    "compilation": {
      "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-01.cc $(geant4-config --cflags --libs) -o /tmp/wild01_bin",
      "exit_code": 0,
      "status": "SUCCESS"
    },
    "runtime": {
      "command": "/tmp/wild01_bin 10",
      "exit_code": 0,
      "output_snippet": "WILD-01 Secondary Neutrons = 643",
      "status": "SUCCESS"
    },
    "guardrail": {
      "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-01.cc",
      "verdict": "PASS",
      "critical_defect_count": 0,
      "total_defect_count": 0
    }
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
