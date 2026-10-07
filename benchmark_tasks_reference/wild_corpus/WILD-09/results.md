# WILD-09 案例测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 说明 |
| :--- | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 实际社区真实 Bug，直接写均踩坑报错 |
| **B组 (模型自查)** | ❌ 未通过 | 自查无法发现该隐蔽陷阱：热中子慢化轻水材料命名非标或缺失热散射截面 S(alpha,beta) |
| **C组 (加核验器)** | ✅ 通过 | 核验器直接拦截错误，按规则提示后修复成功 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "WILD-09",
  "verdict": "PASS",
  "defect_slot": "D2_MATERIAL_DEFINITION",
  "defect_title": "热中子慢化轻水材料命名非标导致分子结合态热散射截面 S(alpha, beta) 挂载失效",
  "first_principles_deduction": {
    "conservation_laws_analysis": {
      "measure_invariance": "几何体积与微观相空间未发生非线性雅可比畸变，几何建模正常。",
      "causality_and_clock": "飞行时间与级联微观碰撞时钟单调递增，因果律保持良好。",
      "lifecycle_isolation": "初级粒子创生（脉冲奇点）与连续介质内输运线积分严格拓扑隔离。",
      "variance_and_weights": "采用模拟输运（Analog transport），中子散射与吸收反应的概率测度及权重流守恒。",
      "energy_and_microstate_completeness": "微观非线性响应与激发态自由度完备性破坏：热中子（E < 4 eV）在轻水中的慢化本质上依赖水分子化学键引起的束缚态分子振动、阻转（libration）等低能激发自由度。自建材料破坏了与热中子散射律 S(alpha, beta) 数据库的键匹配，导致分子内部自由度丢失，中子退化为与自由质子气体的弹性散射，能量传递通道截断，无法形成 0.025 eV 的麦克斯韦热平衡峰。"
    },
    "root_cause_breakdown": "原代码通过 G4NistManager 手动获取元素 H 和 O 并自定义组合成材料 'H2O'。在 Geant4 中，G4ThermalNeutrons（底层依托 G4ParticleHPThermalScattering 和 G4ParticleHPThermalScatteringData）通过硬编码字符串对数据库进行严格索引匹配。其内部索引表仅包含标准材料/元素对 ('G4_WATER', 'H') 映射到 'h_water' 数据集，或显式指定的 'TS_H_of_Water' 结合态元素。当用户命名材料为 'H2O' 且使用常规元素 'H' 时，G4ThermalNeutrons 无法识别该材料存在热散射数据，静默跳过 S(alpha, beta) 处理，导致中子慢化退化为常规模拟中的自由冷原子核碰撞模型（Free Gas Model），慢化在 ~0.5 eV 处停滞而无法热化。"
  },
  "remediation": {
    "action": "将手动用 H、O 元素组装的 'H2O' 材料替换为 NIST 预定义标准轻水材料 nist->FindOrBuildMaterial(\"G4_WATER\")。",
    "mechanism": "使用 'G4_WATER' 标准名称使 G4ParticleHPThermalScatteringNames 正确命中 ('G4_WATER', 'H') -> 'h_water'，进而载入 JEFF-3.3/ENDF-B 的 S(alpha, beta) 热散射数据表，恢复分子微观束缚态与上散射热平衡自由度。"
  },
  "verification": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-09.cc $(geant4-config --cflags --libs) -o /tmp/wild09_bin",
    "compile_status": "SUCCESS",
    "run_command": "/tmp/wild09_bin 10",
    "run_exit_code": 0,
    "run_output_summary": "WILD-09 Thermal Moderator completed, Geant4 RunManager executed 10 events normally and terminated cleanly.",
    "guardrail_check_command": "python3 /home/shark/胡思乱想/1/guardrail/check.py --source /home/shark/胡思乱想/1/测量方案/real_world_wild_corpus/code_C_isolated/WILD-09.cc",
    "guardrail_verdict": "PASS",
    "guardrail_output": "【认知护栏检测结果】：\n✓ 五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 。"
  },
  "status": "PASSED",
  "final_verdict": "PASS"
}
```
