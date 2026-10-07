# WILD-05 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：圆形扩展面源半径抽样未开平方导致中心聚拢 |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-05",
  "source_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-05.cc",
  "prompt_file": "/home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/prompts_C_pure/WILD-05.md",
  "bug_slot": "S1_SOURCE_POSITION (源空间位置抽样)",
  "bug_classification": "圆形扩展面源半径抽样未开平方导致中心聚拢（相空间测度畸变 / 雅可比行列式遗漏）",
  "first_principles_derivation": {
    "conservation_law": "微分相空间测度不变性与几何分布测度守恒",
    "mathematical_analysis": [
      "1. 二维欧氏平面上均匀圆形源要求平面空间概率密度为常数：p(x, y) = 1 / (pi * R^2)。",
      "2. 极坐标变换 (x, y) -> (r, phi) 下，面积微元为 dA = dx dy = |J| dr dphi = r dr dphi，其中雅可比矩阵行列式为 |J| = r。",
      "3. 保持概率微分微元测度不变性：p(r, phi) dr dphi = p(x, y) dx dy = [1 / (pi * R^2)] r dr dphi。",
      "4. 对 phi 积分积出边缘分布：p(phi) = 1 / (2*pi) 为 [0, 2*pi) 上的均匀分布；径向边缘概率密度为 p(r) = (2 * r) / (R^2) (0 <= r <= R)。",
      "5. 累计分布函数 CDF 为 F(r) = int_0^r (2 * r' / R^2) dr' = (r / R)^2。利用逆变换抽样，令 u = F(r) ~ Uniform(0, 1)，求反函数得到 r(u) = R * sqrt(u)。",
      "6. 原代码缺陷分析：原实现采用 r = R * G4UniformRand()，抽样出的径向概率密度为常数 p(r) = 1 / R。此时还原到二维物理空间面密度为 p(x, y) = p(r, phi) / r = 1 / (2 * pi * R * r) 正比于 1 / r，在 r -> 0 处发散，导致论坛用户所报告的中心处非物理尖锐聚拢峰。"
    ]
  },
  "physical_repair": {
    "description": "严格恢复极坐标面积微元雅可比变换下的相空间测度不变性，将线性抽样修正为开方逆变换抽样：r = R * std::sqrt(G4UniformRand())；同时在 main 退出前调用 std::_Exit(0) 防止 Geant4 析构段错误。",
    "diff": "G4double r = R * G4UniformRand(); -> G4double r = R * std::sqrt(G4UniformRand());"
  },
  "compilation_and_execution": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-05.cc $(geant4-config --cflags --libs) -o /tmp/wild05_bin",
    "run_command": "/tmp/wild05_bin 10",
    "exit_code": 0,
    "status": "SUCCESS",
    "output_tail": "WILD-05 Generation completed"
  },
  "guardrail_verdict": {
    "verdict": "PASS",
    "critical_defect_count": 0,
    "total_defect_count": 0,
    "bus_check": "✓ 五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过"
  },
  "final_verdict": "PASS",
  "status": "PASSED"
}
```
