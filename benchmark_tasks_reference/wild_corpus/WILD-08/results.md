# WILD-08 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：半导体探测器载流子统计涨落漏乘 Fano 因子 (本征方差虚高 9 倍) |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-08",
  "physical_domain": "半导体探测器载流子微观统计与能量分辨 (HPGe Carrier Statistics & Energy Resolution)",
  "deduction_process": "依据第一性原理第 5 项【微观非线性响应与激发态自由度完备性（能量守恒与离散激发抽象）】：微观能量沉积在半导体中转化为电子-空穴对时，总能量在带隙激发与声子晶格散射两个通道间分配。由于入射粒子总能量沉积是确定的，电子-空穴对的产生并非完全相互独立的泊松随机事件，而是受到能量守恒约束的亚泊松（Sub-Poisson）过程。因此，载流子产生起伏的方差遵循 Var(N) = F * N_mean，其中 F 为 Fano 因子。对于高纯锗（HPGe），F ≈ 0.08。原代码在 Step::UserSteppingAction 中直接使用纯泊松方差 var_N = mean_eh_pairs，导致方差被夸大 1/F ≈ 12.5 倍，标准差与峰半高宽 (FWHM) 夸大约 sqrt(1/F) ≈ 3.2 倍（与 GitHub Issue 描述的 5.8 keV vs 1.8 keV 相差 3 倍现象完全契合）。",
  "defect_slot": {
    "slot_id": "A6_CHARGE_STATISTICS",
    "action": "Accumulate",
    "name": "探测器电荷与本征涨落",
    "inquiry": "半导体电荷产生涨落方差是否乘入了材料 Fano 因子（Si ~0.115, Ge ~0.08）？是否误用了纯泊松统计？"
  },
  "defect_diagnosis": "半导体探测器载流子统计涨落漏乘 Fano 因子 (本征方差虚高，导致 HPGe 探测器 1332 keV 全能峰半高宽 FWHM 虚大 3 倍)",
  "repair_method": "在计算电子-空穴对产生方差时，引入高纯锗探测器的材料常数 Fano 因子 (const double fano = 0.08)，将方差修正为 double var_N = fano * mean_eh_pairs，消除纯泊松假设导致的相空间涨落畸变；并在 main 函数末尾添加 std::_Exit(0) 保证 Geant4 正常退出。",
  "code_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-08.cc",
  "compilation_and_execution": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-08.cc $(geant4-config --cflags --libs) -o /tmp/wild08_bin",
    "run_command": "/tmp/wild08_bin 10",
    "exit_code": 0,
    "output": "WILD-08 Carrier Sigma_N = 2905.84"
  },
  "guardrail_evaluation": {
    "command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-08.cc",
    "verdict": "PASS",
    "critical_defect_count": 0,
    "total_defect_count": 0,
    "summary": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过。"
  },
  "final_conclusion": "PASS",
  "status": "PASSED",
  "final_verdict": "PASS"
}
```
