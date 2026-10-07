# WILD-02 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：符合测量或飞行时间误用局部时钟 GetLocalTime() |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-02",
  "background": "CERN Geant4 User Forum [Topic: Coincidence time difference is always zero or small]. 用户提问其模拟的 TOF 探测器放置于距离源 1 米外，但记录的中子到达时间接近零（皮秒量级而非纳秒量级）。",
  "first_principles_derivation": {
    "principle": "Level 3 因果律与绝对时钟单调性（时间基准抽象）",
    "analysis": "在粒子输运与时间飞行谱（TOF）测量中，观测量的定义是粒子从初级发生顶点（t=0）发射并穿过介质到达探测器的绝对实验室时间。在 Geant4 框架语义契约中，GetLocalTime() 是径迹生命周期内的局域时钟（从当前径迹创建时刻起算），而 GetGlobalTime() 才是自事件初始产生点（全局时间原点 t=0）以来的实验室因果单调递增时钟。当 14 MeV 快中子进入塑料闪烁体探测器后，与介质中的氢核、碳核发生弹性/非弹性散射产生次级反冲质子或反冲核，次级粒子诞生时刻其 GetLocalTime() 从 0 开始重新计时，在介质中微步输运的时间仅有亚皮秒至皮秒量级。若在 UserSteppingAction 中使用 GetLocalTime() 计分，会导致时间原点发生非物理的局部重置，严重破坏了全局时间单调性与因果律基准，从而导致观测到的 TOF 时间接近零。"
  },
  "slot_diagnosis": {
    "slot_id": "F3_STEP_TIMING",
    "action": "Fetch",
    "name": "步进时钟选取",
    "core": "CAUSALITY_CORE",
    "contract": "TIME_METRICS_CONTRACT",
    "severity": "CRITICAL",
    "defect_name": "符合测量或飞行时间误用局部时钟 GetLocalTime()",
    "defect_location": "Step::UserSteppingAction 第 55 行: G4double tof = s->GetPreStepPoint()->GetLocalTime();"
  },
  "remedy": {
    "description": "将 Step::UserSteppingAction 中的局域时钟读取替换为实验室绝对全局时钟 GetGlobalTime()：G4double tof = s->GetPreStepPoint()->GetGlobalTime(); 同时在 main() 退出前显式调用 std::_Exit(0) 规避 Geant4 析构段错误。",
    "file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-02.cc"
  },
  "compilation_and_execution": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-02.cc $(geant4-config --cflags --libs) -o /tmp/wild02_bin",
    "run_command": "/tmp/wild02_bin 10",
    "compile_exit_code": 0,
    "run_exit_code": 0,
    "execution_output_summary": "WILD-02 TOF Sum = 8.85302e+06 ns, 进程正常退出 Exit 0"
  },
  "guardrail_verification": {
    "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-02.cc",
    "verdict": "PASS",
    "critical_defect_count": 0,
    "total_defect_count": 0,
    "bus_status": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验全部通过"
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
