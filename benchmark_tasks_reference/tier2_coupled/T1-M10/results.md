# T1-M10 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少土壤深度剖面分箱 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 宇宙线中子土壤深度热化分布完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 宇宙线中子土壤深度热化分布完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M10",
  "task_name": "宇宙线次级中子能谱垂直入射地表 2 米土壤层深穿透热化通量演化",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "fluence",
      "src": "U"
    },
    "F1b": {
      "v": "T1-M10 目标几何空间出射面",
      "src": "U"
    },
    "F2": {
      "v": "count",
      "src": "U"
    },
    "F3": {
      "v": "other",
      "src": "A"
    },
    "F4": {
      "v": "surface_avg",
      "src": "U"
    },
    "F5": {
      "v": "steady",
      "src": "A"
    },
    "F6": {
      "v": "per_source",
      "src": "U"
    },
    "F7": {
      "v": "trend",
      "src": "A"
    },
    "F8": {
      "v": "other_mc",
      "src": "A"
    },
    "F9": {
      "v": "N/A",
      "src": "U"
    },
    "F10": {
      "v": "scalar",
      "src": "U"
    },
    "warnings": [
      "per_source_needs_strength"
    ]
  },
  "pathology_deduction": {
    "material_defect": "基线代码未构建土壤材料（仅定义 G4_AIR），缺失含水（含 H 元素）的真实土壤组分，导致中子缺乏质子弹性散射慢化中心，无法发生热化物理过程。",
    "geometry_defect": "基线代码缺失 2 米厚土壤层靶区实体以及每 20 cm 一档的 10 层深度剖面几何分箱（CopyNumber 0~9），无法开展深度演化剖面分析。",
    "source_kinematics_defect": "基线代码仅发射单能 10 MeV 中子，严重背离海平面宇宙线次级中子宽能谱物理真实（蒸发峰 ~1-2 MeV、高能散裂级联峰 ~100 MeV 及 1/E 减速谱跨越 9 个数量级）。",
    "tally_lifecycle_defect": "基线代码仅以全局单一空间位置累加步点，混淆了步长与物理通量，缺失径迹长度体积通量（Track-Length Fluence）估计量，未对快中子（>100 keV）、超热中子与热中子（<0.5 eV）进行能区解耦计分。"
  },
  "repairs_applied": {
    "material_repair": "采用 ICRU/PNNL 标准土壤组分（含 H, C, O, Al, Si, K, Ca, Fe，密度 1.52 g/cm3，含水率约 10%），为中子弹性碰撞减速提供完备的轻氢介质。",
    "geometry_repair": "构建 6m 大气世界体，内部放置 4m x 4m x 2m 土壤包络体，并精确切分为 10 个 20 cm 厚度的独立土壤分层切片（SoilLayerPhys，CopyNo 0~9），严格还原地表 2 米分箱需求。",
    "kinematics_repair": "实现多通道宇宙线中子能谱连续抽样函数：加权覆盖蒸发峰（Maxwell-Boltzmann 蒸发型）、高能级联峰（对数正态分布 ~100 MeV）及 1/E 减速谱，动量方向沿 +z 垂直入射。",
    "tally_repair": "引入无偏径迹长度体积通量估计量（Phi = sum(L) / (V * N)），按快中子（E>100 keV）、超热中子（0.5 eV~100 keV）及热中子（E<0.5 eV）独立统计各深度分箱通量演化，并增加层间出射面穿透统计。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M10.cc $(geant4-config --cflags --libs) -o /tmp/t1m10_bin",
    "compile_ok": true,
    "run_command": "/tmp/t1m10_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "宇宙线中子土壤深度热化分布完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过"
  }
}
```
