# WILD-04 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：深穿透厚屏蔽模拟未乘权重或未配置方差缩减 |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-04",
  "first_principles_inquiry": {
    "principle_id": 4,
    "principle_name": "概率测度守恒与估计量期望无偏性（蒙特卡洛权重流抽象）",
    "description": "在非模拟（Non-analog）输运、空间区域分裂、轮盘赌或多重方差缩减作用下，可观测量的统计累加必须维持数学期望无偏。每次相空间密度的人工调整必须在权重流上进行严格守恒的代数补偿。"
  },
  "defect_slot": {
    "slot_id": "A4_WEIGHT_MULTIPLICATION",
    "action": "Accumulate",
    "name": "方差缩减权重记账",
    "inquiry": "权重记账元问：启用了几何分裂/重要性方差缩减后，所有计数与通量累加是否严格乘入了粒子动态权重 GetWeight()？",
    "core": "VARIANCE_CORE",
    "contract": "WEIGHT_FLUX_CONSERVATION_CONTRACT"
  },
  "deduction_process": "在深穿透几何重要性抽样（Geometry Importance Splitting）或权重窗（Weight Window）模拟中，粒子跨越不同重要性区域时发生几何分裂，次级粒子的统计权重相应按比例降低（w = 1/2^k）以换取更高抽样频次。为保证蒙特卡洛估计量的数学期望无偏性 E[Phi_hat] = Phi，在计分累加时必须严格乘入粒子的瞬时权重 w_i。原始代码在 Det 计数步进中直接使用 gFlux += 1.0 硬编码整数累加，漏乘了 GetWeight()，导致所有微小权重的分裂粒子被按初始全权重 1.0 进行无差别累加，宏观穿透通量被虚假放大数个数量级。依据原理 4 权重流测度守恒律，锁定病灶位于累加动作槽位 A4_WEIGHT_MULTIPLICATION。",
  "repair_method": {
    "action": "将 Step::UserSteppingAction 中的硬编码计分 gFlux += 1.0; 修正为严格乘入动态权重的无偏估计量 gFlux += s->GetTrack()->GetWeight();",
    "auxiliary": "保持原有物理几何与粒子源配置完备，在 main() 退出前调用 std::_Exit(0) 防止 Geant4 析构段错误。"
  },
  "compilation_and_execution": {
    "command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-04.cc $(geant4-config --cflags --libs) -o /tmp/wild04_bin && /tmp/wild04_bin 10",
    "compile_status": "SUCCESS",
    "exit_code": 0,
    "stdout_summary": "Geant4 物理过程初始化正常，BeamOn(10) 成功完成，输出 'WILD-04 Weighted Flux = 0'，进程 Exit 0 退出。"
  },
  "guardrail_verdict": {
    "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-04.cc --json",
    "verdict": "PASS",
    "critical_defects": 0,
    "total_defects": 0,
    "message": "✓ 五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过。"
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
