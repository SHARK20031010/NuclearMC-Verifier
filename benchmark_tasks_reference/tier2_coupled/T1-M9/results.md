# T1-M9 测试结果与分析

## 一、对比结果

| 测试组 | 结果 | 编译 | 说明 |
| :--- | :---: | :---: | :--- |
| **A组 (直接生成)** | ❌ 未通过 | 编译通过 | 缺少铅玻璃观察窗接缝几何 |
| **B组 (模型自查)** | ✅ 通过 | 编译通过 | 铅玻璃观察窗接缝搭接泄漏模型完整 |
| **C组 (加核验器)** | ✅ 通过 | 编译通过 | 铅玻璃观察窗接缝搭接泄漏模型完整 |

## 二、核验器静态代码审计明细

```json
{
  "task_id": "T1-M9",
  "task_name": "热室铅防护墙嵌 15 cm 铅玻璃直缝 vs 45度阶梯搭接（Z-step）防辐射泄漏验证",
  "status": "PASSED",
  "ring0_intent": {
    "F1a": {
      "v": "fluence",
      "src": "U"
    },
    "F1b": {
      "v": "T1-M9 目标几何空间出射面",
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
    "material_defect": "基线代码未定义铅 (G4_Pb) 与铅玻璃 (G4_GLASS_LEAD)，仅用普通混凝土构建，材料衰减截面与真实热室重屏蔽存在数量级差距。",
    "geometry_defect": "基线代码仅为单一平板，完全缺失直缝平接（Straight Butt Joint）与 45 度阶梯搭接（Z-step Lap Joint）的对比几何结构，无缝隙直通与阶梯阻断实体。",
    "source_kinematics_defect": "基线代码源粒子对准几何中心发射，未聚焦瞄准 1 mm 边缘接缝区域，无法有效激发缝隙泄漏物理效应。",
    "tally_lifecycle_defect": "基线代码未按两种搭接结构进行解耦统计，仅按全局坐标粗略计数，无出射面通量归一化与粒子消逝控制。"
  },
  "repairs_applied": {
    "material_repair": "采用 G4NistManager 构建标准 G4_Pb（10 cm 铅墙）与 G4_GLASS_LEAD（15 cm 铅玻璃观察窗）。",
    "geometry_repair": "构建双构型对比几何：构型1为 1 mm 直通平接缝（直通漏束）；构型2为 45 度斜面 Z-step 阶梯搭接（强制穿透 >= 5 cm Pb 或 9.9 cm 铅玻璃），并在出射面各布置独立计数体。",
    "kinematics_repair": "源项为 Co-60 1.33 MeV 伽马线，分别对准直缝平接缝与 Z-step 搭接缝区域精准发射入射粒子。",
    "tally_repair": "在出射界面计分面按面积 (200 cm^2) 统计透射通量与泄漏粒子数，出射后终止粒子生命周期 (fStopAndKill)。"
  },
  "verification_results": {
    "compile_command": "g++ -O2 /home/shark/胡思乱想/1/测量方案/benchmark_tier2/code_C_isolated/T1-M9.cc $(geant4-config --cflags --libs) -o /tmp/t1m9_bin",
    "compile_ok": true,
    "run_command": "/tmp/t1m9_bin 10",
    "run_exit_code": 0,
    "verify_tier2_passed": true,
    "verify_tier2_reason": "铅玻璃观察窗接缝搭接泄漏模型完整",
    "guardrail_passed": true,
    "guardrail_verdict": "五大守恒总线（因果时钟、相空间测度、权重流、生命周期、核数据）底层契约核验通过 [环 0 回执就绪：用户定 7, AI 定 4]"
  }
}
```
