# WILD-06 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：闪烁体在高 LET 粒子照射下未设置 Birks 猝灭常数 |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-06",
  "status": "PASSED",
  "topic": "Fast neutron scintillation light yield vs gamma light yield ratio inverted",
  "source_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/prompts_C_pure/WILD-06.md",
  "code_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-06.cc",
  "audit_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/results_isolated/WILD-06_audit.json",
  "first_principles_derivation": {
    "principle_mapping": [
      {
        "principle_id": 5,
        "principle_name": "微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）",
        "interrogation_focus": "微观能量沉积转化为宏观观测信号时，是否隐含假设了完全理想的线性比例关系？高电离激发密度下的非辐射耗散通道是否完备闭环？",
        "derivation": "在有机塑料闪烁体（聚乙烯基甲苯基质）中，快中子探测主要依赖快中子与氢核的弹性散射产生反冲质子（Recoil Protons）。质子和重带电粒子具有极高的阻止本领 (dE/dx)。在高激发态分子局域密度下，激子-激子碰撞湮灭显著增加，非辐射耗散（Non-radiative Quenching）占主导，发光响应遵循 Birks 定律 dL/dx = (S*dE/dx) / (1 + kB*dE/dx)。原始代码未对材料调用 SetBirksConstant，导致 Geant4 内部 G4EmSaturation 处于非激活线性状态，默认发光量与微观能量沉积严格成正比（L = Y * E），忽略了高 LET 反冲质子的荧光猝灭，导致模拟的反冲质子发光响应异常偏高 2~4 倍，进而造成快中子发光与伽马发光比值严重倒置。"
      },
      {
        "principle_id": 3,
        "principle_name": "微观相互作用物理通道完备性与粒子动力学生命周期",
        "interrogation_focus": "强子物理列表是否成对注册强子弹性 (Elastic) 与非弹性 (Inelastic) 物理过程？",
        "derivation": "快中子探测在有机闪烁体中的核心能量沉积来自于强子弹性散射道 (n + p -> n + p)。原始代码仅注册了 G4HadronPhysicsQGSP_BIC_HP（属于 Inelastic 过程构造器），缺失 G4HadronElasticPhysicsHP。成对挂载高精度强子弹性与非弹性过程，确保快中子反冲质子动力学产生通道完备。"
      }
    ]
  },
  "lesion_slots": [
    {
      "slot_id": "Material_Properties_Ionisation_BirksConstant",
      "slot_name": "闪烁介质电离参数与 Birks 猝灭常数槽位",
      "location": "Det::Construct() 中 scMat 材料属性定义处",
      "defect": "未配置塑料闪烁体的 Birks 猝灭常数，导致微观非辐射耗散通道缺失，高 LET 粒子线性发光失真",
      "fix": "显式调用 scMat->GetIonisation()->SetBirksConstant(0.126 * mm / MeV)"
    },
    {
      "slot_id": "Physics_List_Hadron_Elastic_Constructor",
      "slot_name": "强子弹性散射物理构造器槽位",
      "location": "Phys::Phys() 物理模块列表注册处",
      "defect": "缺少 G4HadronElasticPhysicsHP，导致 14 MeV 快中子弹性散射产生反冲质子的关键物理通道缺失",
      "fix": "成对注册 RegisterPhysics(new G4HadronElasticPhysicsHP())"
    }
  ],
  "verification": {
    "compilation": {
      "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-06.cc $(geant4-config --cflags --libs) -o /tmp/wild06_bin && /tmp/wild06_bin 10",
      "exit_code": 0,
      "status": "SUCCESS",
      "stdout_summary": "Simulation of 10 events completed successfully with clean exit 0"
    },
    "guardrail_check": {
      "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-06.cc",
      "exit_code": 0,
      "status": "PASS",
      "output": "【认知护栏检测结果】：\n✓ 五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 。"
    }
  },
  "final_verdict": "PASS"
}
```
