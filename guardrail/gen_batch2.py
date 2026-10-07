#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成第二批规则（PHYS-0018 ~ PHYS-0030）。

这批规则不是照着「参数配置总表」写的，是照着**实测出来的 14 个错**写的。
靶子来自 /home/shark/胡思乱想/1/测量方案/results/审读/verdicts.tsv。

用法：python3 gen_batch2.py   （直接写进 rules/）
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "rules"

AUTH = 'authors: ["待填"]\nreviewed_by: []\n'

# 每条：(id, 标题, stage, mechanism, severity, output, trigger_yaml, verdict, question, sources, body)
R = []

R.append(("PHYS-0018", "装了非弹性强子物理，却没装弹性", 4, "M9", "high", "violation", """  source: snapshot
  subject: physics_list
  name: "-"
  key: hadron_missing_elastic
  value_is: true""", """这个程序的物理列表里装了「非弹性」强子物理，但一个「弹性」都没有。
弹性散射是低能中子在材料里改变方向的主要机制。漏掉它，中子的走向和穿透深度都会算错。
程序不会报错，也不会有任何提示。""", None,
    ['Geant4 11.2.2 物理构造器命名约定：弹性类含 `hElastic`/`Elastic`，非弹性类含 `hInelastic`/`Inelastic`',
     '本机实测：任务 T1-2 自己拼列表只装了 `G4EmStandardPhysics_option4` + `G4HadronPhysicsQGSP_BIC_HP`，中子过程只有 Transportation/neutronInelastic/nCapture/nFission，没有 hadElastic',
     '本机实测：T1-2 跑 5 万事件透射率 44.666%，换官方 QGSP_BIC_HP 为 22.512%，差 1.98 倍'],
    """## 现象

自己拼物理列表时，很容易写成这样：

```cpp
RegisterPhysics(new G4EmStandardPhysics_option4());
RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
```

两行都合法、都编译通过、跑起来也不报错。但第二行只装了**非弹性**强子物理，
弹性散射那一项没有。

## 为什么是问题

弹性散射是低能中子改变方向的主要机制。对"14 MeV 中子穿过 30 cm 混凝土"
这类题，弹性散射贡献了相当一部分的慢化和散射返回。
漏掉它，透射率算成 0.445，正确值 0.225 —— **差将近 2 倍**，而程序不报任何错。

## 判据

快照里的构造器名字里：
- 含 `elastic` 的 → 装了弹性
- 含 `inelastic` 的 → 装了非弹性

**装了非弹性、却一个弹性都没有** → 报。

纯电磁的任务（只有一个 `G4EmStandard*` 构造器）不会触发这条，因为它们
压根没有强子物理，谈不上"漏了弹性"。

## 已知局限

- 判据靠构造器名字里的子串。如果某个列表用别的命名（非标准写法）装弹性，
  会漏报。
- 本条只回答"弹性装没装"，不回答"该不该装"。所以判成 `violation` 而不是
  `question` —— 装了非弹性却漏弹性，几乎不存在有意为之的正当理由。"""))

R.append(("PHYS-0019", "用步点判断「粒子穿过几何边界」", 1, "M13", "high", "question", """  source: code
  present: ['fGeomBoundary']
  checks:
    - kind: same_line
      patterns: ['GetStepStatus', 'fGeomBoundary']
      label: '在步点上判断几何边界'""", """代码里在步点上判断「几何边界」（fGeomBoundary）。
需要确认用的是前步点还是后步点。""", """Geant4 把"这一步穿过几何边界"这个标志设在**后步点**上；
前步点的状态是从上一步的后步点拷贝过来的。

你这一行用的是前步点还是后步点？

用错了的后果：会把"上一步刚进到某个体积"误当成"这一步正在穿边界"。

实测：任务 T1-4 用了前步点，结果只捞到"一步贯穿铅板、零碰撞"的弹道粒子；
<1 MeV 的伽马计数从 65 掉到 1，能谱形状整块丢失。""",
    ['Geant4 11.2.2 source/processes/transportation/src/G4Transportation.cc:508 —— `stepData.GetPostStepPoint()->SetStepStatus(fGeomBoundary);`',
     'Geant4 11.2.2 source/track/src/G4ParticleChangeForTransport.cc:96-104（后步点的体积是进入的那个）',
     'Geant4 11.2.2 source/track/include/G4Step.icc:142-149（前步点状态是上一步后步点的拷贝）',
     '本机实测：任务 T1-4 探针双判据对比 1000 事件，报 223 中子/75 伽马，正确值 439/200'],
    """## 现象

```cpp
auto pre = st->GetPreStepPoint();
if (pre->GetStepStatus() != fGeomBoundary) return;   // ← 用前步点
```

## 为什么是问题

Geant4 的 `fGeomBoundary` 状态由 `G4Transportation` 在**这一步结束时**
写到**后步点**上（`G4Transportation.cc:508`）。
前步点的状态是上一步后步点的副本。

所以用前步点判"这一步穿边界"，实际问的是"上一步结束时是否在边界上"，
语义完全不同。

## 实测后果

任务 T1-4（1 MeV 伽马打 5 cm 铅板，统计穿出的能谱）：
用前步点的版本只捞到"一步就贯穿铅板、一次碰撞都没有"的弹道粒子。
能谱形状整块丢失 —— 低能段（<1 MeV）计数从 65 掉到 1。

## 为什么不用 violation

也存在正当用法：想知道"上一步是不是刚从某个体积进来"时，看前步点是对的。
程序分不清意图，所以只提问、不下结论。"""))

R.append(("PHYS-0020", "把 track 的能量当成「刚产生时」的能量", 6, "M13", "high", "question", """  source: code
  present: ['GetKineticEnergy']
  checks:
    - kind: present
      pattern: '(?s)GetTrack\\(\\)\\s*;?.{0,400}?GetKineticEnergy\\(\\)'
      label: '对 track 读动能'""", """代码里从 track 上读动能。需要确认读的时机对不对。""",
    """track 上存的是"现在"的能量 —— 一步走完之后，这一步该扣的已经扣掉了。

你要的如果是"刚产生时"的能量，应该读 step 的**前步点**（`GetPreStepPoint()->GetKineticEnergy()`）。

实测：任务 T6-1 用 track 能量记录"产生时刻"的韧致辐射光子能量，
576 个光子（占 28%）被记成 0 落进最低道 —— 韧致辐射谱形状看着还是对的
（有 1/E 形状、有 511 keV 湮灭峰），肉眼看不出来。

反过来说：如果你做的是"径迹长度 kerma"这类估计量，用当前能量反而是对的。
所以这条是问题，不是结论。""",
    ['Geant4 11.2.2 source/track/include/G4Track.hh（动能是 track 的当前状态）',
     'Geant4 11.2.2 source/track/include/G4Step.icc（前步点 = 这一步开始时的状态）',
     '本机实测：T6-1 只把 `tr->GetKineticEnergy()` 改成 `step->GetPreStepPoint()->GetKineticEnergy()`，同 1000 事件同随机数种子，0–0.2 MeV 道占比 0.35871→0.21719，全谱 L1 距离 0.2830'],
    """## 现象

```cpp
G4Track* tr = step->GetTrack();
if (tr->GetCurrentStepNumber() != 1) return;   // 只想看"产生步"
G4double e = tr->GetKineticEnergy();           // ← 这时已经被这一步扣过了
```

## 为什么是问题

`G4Track::GetKineticEnergy()` 是 track 的**当前**状态。
当 `UserSteppingAction` 被调用时，这一步的离散作用已经算完，
track 的能量已经是这一步之后的能量。

想拿"刚产生时"的能量，要读这一步的**前步点**。

## 实测后果

任务 T6-1（20 MeV 电子打钨靶，出韧致辐射谱）：
- 用 track 能量：576 个"产生时 >0.2 MeV"的光子被记成 0，落进最低道
- 改前步点：0–0.2 MeV 道占比从 0.35871 降到 0.21719

关键在于**结果看起来是对的**：谱有正确的 1/E 形状、有 511 keV 湮灭峰、
最高能量到 18.8 MeV。不逐道比对，看不出 28% 的光子分错了箱。

## 已知局限

判据是"文件里 `GetTrack()` 之后 400 字符内出现 `GetKineticEnergy()`"。
行距更远或中间绕了变量的写法会漏。"""))

R.append(("PHYS-0021", "用「被定义步的过程名」判断次级粒子的来源", 6, "M13", "medium", "question", """  source: code
  present: ['GetProcessDefinedStep']
  checks:
    - kind: always
      label: '用到了 GetProcessDefinedStep'""", """代码里用 `GetProcessDefinedStep()` 取过程名。需要确认这是不是你要的那个"来源"。""",
    """`GetProcessDefinedStep()` 给的是"**这一步**被哪个过程定义"。

对次级粒子来说，它产生之后走的第一步往往由 `ionIoni` 或 `NoProcess` 定义，
而不是产生它的那个过程。

你要问的如果是"这个粒子是**哪个过程产生出来**的"，那要用
`GetCreatorProcess()`。

实测：任务 T3-2 用 `GetProcessDefinedStep()->GetProcessName()` 去筛
"中子作用产生的残余核"，白名单里七个过程名一个都没匹配上 ——
表格输出全空（只有表头），程序退出码 0、没有任何报错。""",
    ['Geant4 11.2.2 source/track/include/G4Track.hh —— `GetCreatorProcess()` 返回产生该 track 的过程',
     'Geant4 11.2.2 source/track/include/G4Step.hh —— `GetProcessDefinedStep()` 返回定义这一步的过程',
     '本机实测（T3-2 调试版，300 事件）：`GetProcessDefinedStep` 的过程名只出现 ionIoni / NoProcess / Radioactivation / hIoni / conv；`GetCreatorProcess` 实测 neutronInelastic 489 次、hadElastic 86 次、Radioactivation 96 次'],
    """## 现象

```cpp
auto* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
G4String pn = proc->GetProcessName();
if (pn == "neutronInelastic" || pn == "nCapture" || ...) gAtoms[...]++;
```

## 为什么是问题

两个 API 名字很像，含义不同：

| 你要的 | 该用的 |
|---|---|
| 这个粒子是**谁产生**的 | `track->GetCreatorProcess()` |
| **这一步**是被谁处理的 | `step->GetPostStepPoint()->GetProcessDefinedStep()` |

## 实测后果（最刺眼的一例）

任务 T3-2（铁样品被 14 MeV 中子照射后生成哪些放射性核素）：

- 白名单里写了 `neutronInelastic` / `nCapture` / `hadElastic` / `nFission` /
  `PhotoEvaporation` / `RadioactiveDecay` / `Decay`
- 实际 `GetProcessDefinedStep` 只会给出 `ionIoni` / `NoProcess` /
  `Radioactivation` / `hIoni` / `conv`
- **一个都不匹配** → 累计表恒空 → 输出只有表头，**没有一行数据**
- 程序退出码 0，没有任何报错

同一批任务里，T3-1 用同一个 API 但查的是**中子自己**的俘获步
（`p->GetProcessName()=="nCapture"`），那是对的 —— 因为那一步确实由
`nCapture` 定义。所以这条规则只提问，不判错。

## 顺带记下的事实

Geant4 里放射性衰变的过程名是 **`Radioactivation`**，不是
`RadioactiveDecay`（见 `source/processes/hadronic/models/radioactive_decay/include/G4Radioactivation.hh:58`）。
按名字猜很容易猜错。"""))

R.append(("PHYS-0022", "代码里硬编码了两张以上的数值表（谱线表）", 2, "M14", "high", "question", """  source: code
  present: ['(?s)\\[\\s*\\w*\\s*\\]\\s*=\\s*\\{[^{}]*,[^{}]*,[^{}]*,[^{}]*\\}.*\\[\\s*\\w*\\s*\\]\\s*=\\s*\\{[^{}]*,[^{}]*,[^{}]*,[^{}]*\\}']
  checks:
    - kind: always
      label: '两张以上的数值表'""", """代码里有两张以上的数值表（通常是「能量 + 强度」）。这些数需要逐条核对出处。""",
    """程序里有两张以上的数值表，多半是"发射线能量 + 强度"这种形式。

请逐条核对：**每个数是从哪个文件、哪一版来的？**

实测（这一批任务里最典型的两例）：
- T6-4 把 Ir-192 的 604.4 keV 那条线强度写成 **82.2**，正确值 **8.2**
  —— 抄写时换了位。该峰计数从 15 掉到 1。
- T3-4 六条强度里错了三条：Co-60 的两条写成 0.60（应约 1.00，偏低 1.67 倍），
  Na-24 的 2.754 MeV 写成 0.50（应约 1.00，偏低 2 倍）。

两处都是"数字写错"，不是"算错" —— 靠看代码逻辑发现不了，只能核对出处。

可核对的地方：
- Ir-192：本机 Geant4 自带
  `/home/shark/geant4/11.2.2-install/share/Geant4/examples/advanced/brachytherapy/iridium_source_primary.mac`
- Co-60 / Na-24 等：本机 Geant4 数据目录里的放射性衰变数据（`G4LEVELGAMMADATA` / `z*.a*` 文件）""",
    ['本机实测：T6-4 `code/T6-4/code.cc:53` 的强度表 `{28.7,29.7,82.7,47.8,4.5,82.2,0.3}`；按同表其余 6 条线标定比值恒为 0.43，反推该条应为 8.22%',
     '本机实测：T3-4 `code/T3-4/code.cc:24` 的 `gI[NL] = {1.00, 0.60, 0.60, 1.00, 0.27, 0.50}`；本机 z27.a60 显示 Co-60 两条 γ 强度均约 1.00',
     '本机文件：/home/shark/geant4/11.2.2-install/share/Geant4/examples/advanced/brachytherapy/iridium_source_primary.mac'],
    """## 现象

```cpp
const double E[7] = {295.9,308.5,316.5,468.1,588.6,604.4,884.5};
const double I[7] = { 28.7, 29.7, 82.7, 47.8,  4.5, 82.2,  0.3};
```

## 为什么是问题

这类表是**从别处抄来的**，不是算出来的。抄写错误不会触发任何检查：

- 语法合法
- 归一化照做（代码里会 `sum += I[i]` 自动归一）
- 输出正常
- 退出码 0

唯一的破绽是**数值本身不对**，而这只能靠核对原始出处发现。

## 判据

"文件里出现两张以上、每张含 ≥4 个数值字面量的数组初始化"。
不试图判断哪个数错了 —— 那是不可能自动做到的。
只负责把它们**点出来**，要求给出处。

## 为什么用 question

程序没有能力核对核数据。它能做的是"提醒你这里有需要核对的东西"。
所以输出是问题，不是结论。"""))

R.append(("PHYS-0023", "算了「效率」，但代码里没有任何立体角 / 几何接受度", 6, "M6", "high", "question", """  source: code
  present: ['效率|全能峰|efficiency|Efficiency']
  checks:
    - kind: missing
      pattern: '立体角|solid|atan|acos|Omega|接受度|acceptance'
      label: '几何接受度计算'""", """代码里在算"效率"，但没有任何立体角或几何接受度的计算。""",
    """你算的这个"效率"，**分母是什么**？

如果是点源，探测器对源张的立体角就是上限，必须算进去；
如果算的是平行束，那分母就是束流截面。

两者结果差一个量级，但打印出来都是"一个百分比"，看不出区别。

实测：任务 T4-1 用固定方向的点枪（相当于零发散平行束），
报出全能峰效率 39.5%。而点源在 5 cm 处对 r=3 cm 晶体的几何接受度只有 7.13%
—— **超几何上限 11.8 倍**，应该约 2.8%。

任务 T4-2 同类问题更夸张：注释写"点源在轴线上，距晶体中心 16 cm"，
代码却是固定方向，超上限 44 倍。""",
    ['本机实测：任务 T4-1 报全能峰效率 39.545%（峰窗 ±1 keV），点源 5 cm 几何接受度 ½(1−cos(atan(3/5)))=7.13%',
     '本机实测：任务 T4-2 注释（`code/T4-2/code.cc:35`）写"点源在轴线上，距晶体中心 16 cm"，代码（`code/T4-2/code.cc:67-68`）是 `(0,0,-8cm)` + `(0,0,1)` 固定方向，几何接受度 2.28%'],
    """## 现象

```cpp
G4double eff = (G4double)fPeak / nEvents;   // "全能峰效率"
```

## 为什么是问题

"效率"至少有两种含义，数值差一个量级：

| 含义 | 分母 |
|---|---|
| **本征效率** | 打到探测器的粒子数 |
| **绝对效率**（源-探测器） | 源发射的粒子总数 —— 这里必须算立体角 |

算第二种却省掉立体角，就会把"打到探测器的比例"当成"源发射的比例"，
效率被系统性高估。

## 实测后果

任务 T4-1：报 39.5%，几何上限 7.13%，正确值约 2.8%。
程序输出完全正常（"全能峰计数 39517，全能峰效率 39.517%"），
没有任何地方提示这个数超了几何上限。

## 判据的边界

判据是"出现效率字样、且全文找不到立体角/三角函数/接受度"。
写得很绕、或者用别的词表达立体角的代码会漏（漏报，不误报）。

## 为什么用 question

程序分不清"本征效率"和"绝对效率"哪个是用户要的。
只能问。"""))

R.append(("PHYS-0024", "注释说「点源」，代码里发射方向却是写死的常量", 3, "M6", "high", "question", """  source: code
  present: ['点源']
  checks:
    - kind: present
      pattern: 'SetParticleMomentumDirection\\(\\s*G4ThreeVector\\(\\s*[-\\d.]'
      label: '写死的发射方向'""", """注释里写了"点源"，但发射方向是一个写死的常量。""",
    """"点源 + 写死的发射方向"= 一束**平行束**，不是点源。

两者的差别：
- 点源：粒子从一点向各方向发散，到远处按 1/r² 摊开
- 平行束：粒子都朝同一个方向，不摊开

对探测效率、对远处的注量率，两者结果完全不同。

实测：任务 T4-2 注释写"点源在轴线上，距晶体中心 16 cm"，
代码却是 `(0,0,-8cm)` + `(0,0,1)` 固定方向 ——
几何接受度只有 2.28%，报出的效率超上限 44 倍。

请确认你要的是哪一种。""",
    ['本机实测：任务 T4-2 `code/T4-2/code.cc:35` 注释 vs `code/T4-2/code.cc:67-68` 代码',
     '本机实测：任务 T4-1（无"点源"注释但同类写法）效率超几何上限 11.8 倍'],
    """## 现象

```cpp
// 几何：... 点源在轴线上，距晶体中心 16 cm
fGun->SetParticlePosition(G4ThreeVector(0., 0., -8.*cm));
fGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));   // ← 常量方向
```

## 为什么是问题

`G4ParticleGun` 是**单粒子枪**：每次只从一个位置、朝一个方向发。
方向写成常量，就是零发散的平行束。

注释写"点源"说明作者心里想的是点源 —— 但代码不是。
这种"注释与实现不一致"是静默错误的典型来源：读代码的人相信注释。

## 判据

- 前提：**原文**（不去注释）里出现"点源"
- 条件：**去掉注释后**的代码里出现 `SetParticleMomentumDirection(G4ThreeVector(<数字>...`

两个判断故意用不同的文本版本 —— 因为要检出的正是"注释说的"和
"代码做的"不一致。这也是本批规则里唯一一条**故意扫注释**的。

## 已知局限

- 只用中文"点源"作前提；英文注释 `point source` 会漏。
- 用 `G4GeneralParticleSource`（GPS）的写法不适用（那里方向由
  `/gps/ang/type` 控制）。
- 只覆盖"注释标了点源"的情况。T4-1 没写"点源"两个字，
  靠 PHYS-0023（效率无立体角）覆盖。"""))

R.append(("PHYS-0025", "用了闪烁体，但没设「猝灭系数」（Birks）", 4, "M1", "high", "violation", """  source: code
  present: ['SCINTILLATIONYIELD']
  checks:
    - kind: missing
      pattern: 'Birks'
      label: '猝灭系数（Birks）'""", """程序里设了闪烁体的光产额，但没有任何地方设猝灭系数（Birks）。
Geant4 默认**不淬灭**（系数为 0），重带电粒子的光产额会被系统性高估。
程序不会报错，不会提示，输出照常。""", None,
    ['Geant4 11.2.2 source/processes/electromagnetic/utils/src/G4EmSaturation.cc:93-96 —— 仅当 bfactor>0 才做淬灭',
     'Geant4 11.2.2 source/materials/src/G4IonisParamMat.cc:61 —— `fBirks = 0.` 默认值',
     '本机实测：T4-3 补上聚苯乙烯的 Birks 系数（0.07943 mm/MeV，取自 G4EmSaturation.cc:274-277 内建表 Hirschberg 1992）后，中子光产额 10000/MeV → 1780/MeV，伽马/中子光比 3.32 → 0.74（反号）'],
    """## 现象

```cpp
mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV);
mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0 * ns);
// ← 没有 Birks
```

## 为什么是问题

闪烁体的光产额对**电离密度**敏感：重带电粒子（质子、α）沿径迹的
电离密度高，会有一部分激发态被淬灭掉，实际光产额远低于 γ 产生的电子。

这个效应由 **Birks 系数**描述。Geant4 默认是 0（不淬灭）。

所以不设 Birks 的后果不是"略有偏差"，而是**不同粒子的相对响应全错**。

## 实测后果（甄别结论直接反号）

任务 T4-3（塑料闪烁体里区分中子和伽马）：

| | 不设 Birks | 设 Birks |
|---|---|---|
| 中子光产额 | 10000 /MeV | 1780 /MeV |
| γ / 中子 光比 | **3.32** | **0.74** |

比例从"伽马光多 3 倍"变成"中子光多 35%" —— 甄别判据完全反过来。

## 两个容易踩的坑

1. **Birks 不是材料属性表（MPT）的键**。写成
   `mpt->AddConstProperty("BIRKS", ...)` 会抛 `G4Exception : mat207`。
   正确写法是 `mat->GetIonisation()->SetBirksConstant(...)`。
2. Geant4 的内建 Birks 表只认 4 个材料名（`G4_POLYSTYRENE` / `G4_BGO` /
   `G4_lAr` / `G4_PbWO4`，见 `G4EmSaturation.cc:269-303`）。
   自定义名字（比如这里的 `"PlasticScint"`）不在表内，即使设了总开关也不淬灭。"""))

R.append(("PHYS-0026", "程序里有「死层」体积", 6, "M6", "medium", "question", """  source: code
  present: ['dead|Dead']
  checks:
    - kind: always
      label: '死层体积'""", """程序里有"死层"体积。需要确认"探测到"的判据怎么写的。""",
    """程序里有"死层"（dead layer）体积。

请确认"探测到"的判据 —— **死层里的能量沉积不该算作探测到**。
死层的作用恰恰是"粒子在这里损失能量但不产生信号"。

实测：任务 T4-4 的判据写成
`if (g_eCry + g_eDead > 0.) g_any[g_cur]++;`
—— 死层沉积也算"探测到"。后果是探测效率随死层厚度几乎不变
（0/0.1/1/3/10 mm 死层 → 59.95 / 58.60 / 59.80 / 61.45 / 59.60 %）。

只算灵敏区时，同一组数据是 59.95 / 58.55 / 58.50 / 56.15 / **43.00** ——
10 mm 死层下低报 1.39 倍。

顺带：该程序 `code/T4-4/code.cc:24` 的 `kR=2cm`、`kGap=2cm` 让相邻体积
重叠 2 cm，而运行日志**没有任何 overlap 警告** —— Geant4 默认不做这项检查。""",
    ['本机实测：T4-4 `code/T4-4/code.cc:48` 的 `if (g_eCry + g_eDead > 0.) g_any[g_cur]++;`',
     '本机实测：T4-4 死层 0/0.1/1/3/10 mm → 效率 59.95/58.60/59.80/61.45/59.60%（含死层）；只算灵敏区 → 59.95/58.55/58.50/56.15/43.00%',
     '本机实测：T4-4 相邻体积重叠 2 cm，运行日志无 overlap 告警（`G4PVPlacement` 末参 `pSurfChk=false` 为默认值）'],
    """## 现象

```cpp
if (n == "crystal")   g_eCry  += ed;   // 灵敏区
else if (n == "dead") g_eDead += ed;   // 死层
...
if (g_eCry + g_eDead > 0.) g_any[g_cur]++;   // ← 死层也算"探测到"
```

## 为什么是问题

死层（dead layer）在真实探测器里是晶体表面的不灵敏区：
粒子在那里沉积能量，但**不产生信号**。这正是研究死层厚度的意义所在。

把死层沉积也计入"探测到"，等于把死层从模型里抹掉了 ——
于是"死层厚度对效率的影响"这个要测的东西，测不出来。

## 实测后果

| 死层厚度 | 含死层判据 | 只算灵敏区 |
|---|---|---|
| 0 mm | 59.95% | 59.95% |
| 0.1 mm | 58.60% | 58.55% |
| 1 mm | 59.80% | 58.50% |
| 3 mm | 61.45% | 56.15% |
| 10 mm | 59.60% | **43.00%** |

含死层那一列**基本不随厚度变化**（还略有上升），
但打印出来是一张整齐的表，看不出问题。

## 已知局限

判据是"文件里出现 dead/Dead"。对体积名不含 dead 的写法会漏。"""))

R.append(("PHYS-0027", "用球体做灵敏体积（微剂量学）", 4, "M7", "high", "question", """  source: code
  present: ['G4Orb']
  checks:
    - kind: always
      label: '球体灵敏体积'""", """你用了球体做灵敏体积。需要确认线能的分母。""",
    """球体里的"平均弦长"是 **4V/S = 4r/3**（用半径算），**不是直径**。

线能 y = 沉积能 ÷ 平均弦长。分母选错，整个谱按同一个比例偏。

实测：任务 T5-1 用 `new G4Orb("Cavity", 0.5*um)`（第二参数是**半径**），
分母却写死 1 µm（= 直径）。球平均弦长应为 0.6667 µm，全谱偏小 **1.5 倍**。
只把分母改成 0.6667 µm（物理一个字不动），平均线能从 25.88 变成 38.81 keV/µm，
比值恰好 1.5000。

同一批的 T5-3 用了正确的 `4.*kR/3.`，是对照组。""",
    ['球几何：V = 4πr³/3，S = 4πr²，平均弦长 l̄ = 4V/S = 4r/3',
     '本机实测：T5-1 `code/T5-1/code.cc:51` 用 `G4Orb("Cavity", 0.5*um)`，`code/T5-1/code.cc:31` 注释与 `:34` 分母写"1 um"',
     '本机实测（A/B 对照，只改分母不动物理）：平均线能 25.8765 → 38.8148 keV/µm，比值 1.5000',
     '对照：T5-3 `code/T5-3/code.cc:26,70` 用 `4.*kR/3./um`（= 6.667 µm），判定正确'],
    """## 现象

```cpp
auto* lCav = new G4LogicalVolume(new G4Orb("Cavity", 0.5*um), te, "Cavity");
...
double y = fEdep / keV;   // 注释写 "y = E_dep / mean chord = E_dep / 1 um"
```

## 为什么是问题

两个概念要分清：

| 量 | 值（半径 r） |
|---|---|
| 直径 | 2r |
| **平均弦长** ē | **4r/3** |

微剂量学里的线能 y 定义用**平均弦长**：y = ε / l̄。
粒子随机穿过球体，弦长平均值就是 4V/S。

`G4Orb(name, Rmax)` 的第二个参数是**半径**。写 0.5 µm 得到的是直径 1 µm 的球，
平均弦长 0.6667 µm。分母用 1 µm，y 就整体偏小 1.5 倍。

## 实测后果

平均线能 25.88 → 38.81 keV/µm（改分母后）。比值恰好 1.5000
（= 1 / 0.6667），确认就是分母的问题，不是别的。

谱整体平移 1.5 倍意味着：导出的 y_F、y_D 全部错，
而"有个完整的线能谱"这个外观完全正常。

## 为什么用 question

除以直径在某些"路径长度"定义下未必是错的（虽然对标准线能定义是错的）。
程序不替人下这个结论。"""))

R.append(("PHYS-0028", "用了跨事件的去重集合，但没在每事件清空", 7, "M5", "high", "question", """  source: code
  present: ['\\.insert\\(']
  checks:
    - kind: missing
      pattern: 'BeginOfEventAction'
      label: '每事件清空的钩子'""", """代码里用了 `insert(...).second` 这种"只记一次"的写法，
但没有任何 `BeginOfEventAction`。""",
    """Geant4 里 **trackID 是每个事件从头编号的** —— 每个新事件都从 1 开始。

如果这个"已见过"的集合只在 `BeginOfRunAction` 清空，
那么第二个事件开始，trackID 就会和第一个事件撞号，
新粒子会被当成"已经见过"而**静默丢弃**。

正确做法：在 `BeginOfEventAction` 里清空。

实测：任务 T6-2 正是这个写法。只加一个每事件清空、同样 100 个质子，
中子产额从 1163 变成 1785（11.63 → 17.85 个/质子，**低 53%**）。

而且它的答案随事件数乱跳：
N=1/2/3/12/30/100 → 19.0 / 25.5 / 17.0 / 19.33 / 17.1 / 11.63
—— 这是个**物理量不该有的行为**，是这件事的第二个破绽。""",
    ['Geant4 11.2.2 source/run/src/G4RunManager.cc:445 —— `auto anEvent = new G4Event(i_event);`（trackID 每事件重排）',
     '本机实测：T6-2 `code/T6-2/code.cc:32` 用 `gSeen.insert(t->GetTrackID()).second`，`code/T6-2/code.cc:64-66` 只在 `BeginOfRunAction` 清空；全文件无 `BeginOfEventAction`',
     '本机实测（A/B，同样 100 质子）：产额 1163 → 1785（+53%）',
     '本机实测：报告值随 BeamOn(N) 漂移 N=1/2/3/12/30/100 → 19.0/25.5/17.0/19.33/17.1/11.63'],
    """## 现象

```cpp
if (!gSeen.insert(t->GetTrackID()).second) return;   // 同一中子只记一次
...
class RunAct : public G4UserRunAction {
  void BeginOfRunAction(const G4Run*) override {
    gSeen.clear();          // ← 只在整轮开始时清一次
  }
};
```

## 为什么是问题

`BeginOfRunAction` 在**整轮模拟**开始时调用一次。
`G4RunManager` 给每个事件新建 `G4Event`，trackID 从 1 重新编号。

所以第二次以后的事件里，只要 trackID 撞上前面用过的号，
这个粒子就被 `return` 掉了 —— 统计少了一大截，而且**少多少取决于事件数**
（前面事件越多，撞号概率越大）。

## 两个可观测的破绽

1. **产额偏低**：1163 vs 1785（正确）。
2. **结果随事件数变化**：19.0 / 25.5 / 17.0 / 19.33 / 17.1 / 11.63。
   物理量不该依赖你跑了多少事件。

第 2 条不需要读代码就能发现 —— 同一个程序用不同事件数各跑一遍，
看标量结果在不在统计误差内一致，就够了。这是"黑盒检查"的一条素材。

## 已知局限

判据是"出现 `.insert(`"+"全文没有 `BeginOfEventAction`"。
用 `std::set::count` 或自己写数组标记的写法会漏。"""))

R.append(("PHYS-0029", "事件数写死在代码里", 7, "M6", "medium", "question", """  source: code
  present: ['(?i)const\\s+(G4int|int|long)\\s+\\w*[Nn]\\w*\\s*=\\s*\\d+']
  checks:
    - kind: always
      label: '写死的事件数'""", """事件数写死在代码里，而且不接受命令行覆盖。""",
    """请确认这个数是够的，并且把**不确定度**一起报出来。

实测两例：
- T2-3 写死 100 个事件。用 20 个随机种子各跑 100 事件，
  结果极差 **3 倍**（0.671e-14 ~ 1.987e-14 Gy/中子），相对统计误差 27%。
  但程序打印 6 位有效数字、不给误差、退出码 0。
- T6-3 写死 100 事件喂 300 个能量箱，交付的深度剂量曲线是统计噪声主导。

这类问题的共同点：**数字看起来很精确，其实是噪声**。
程序给的有效数字远超它实际的分辨能力，这本身就是误导。""",
    ['本机实测：T2-3 `code/T2-3/code.cc:79` 硬编码 nEvents=100 且不接受命令行覆盖',
     '本机实测：T2-3 20 个随机种子 × 100 事件 → 0.671e-14 ~ 1.987e-14 Gy/中子，20 种子均值 1.2651e-14，相对统计误差 27%',
     '本机实测：T2-3 同一程序跑 10000 事件收敛值 1.25249e-14（与 100 事件的交付值 1.27686e-14 只差 1.9% —— 属随机误差，不是系统偏差）',
     '本机实测：T6-3 写死 beamOn 100，交付曲线锯齿由统计噪声主导'],
    """## 现象

```cpp
const G4int nEvents = 100;   // 写死，且 main() 不读 argv
rm->BeamOn(nEvents);
```

## 为什么是问题

不是"100 个事件一定不够"，而是**两个后果**：

1. **基数不够**：100 个事件对绝大多数观测量都不够，结果是噪声主导。
2. **不给不确定度**：程序打印 6 位有效数字，却不告诉你这个数的不确定度。
   读者会以为它有 6 位精度。

第 2 条更隐蔽 —— 就算事件数够，不给误差也让人没法判断结果可不可信。

## 这一条和"物理错误"是两类

前后两批审读里反复出现，**单独列出来**：

| 题 | 缺陷 | 性质 |
|---|---|---|
| T3-1 | 活度公式错，绝对值差 1.2e7 倍 | 系统偏差（物理错） |
| T2-3 | 100 事件、误差 27%、打印 6 位 | **随机误差 + 假精确** |
| T6-3 | 100 事件喂 300 个箱 | **随机误差 + 假精确** |

T2-3 的交付值恰好落在 20 种子均值附近（+0.9%），
所以按"结果对不对"不能判它错 —— 但它仍然是个该报的缺陷。

## 判据的边界

只抓"事件数写成常量"，抓不到用宏文件 `/run/beamOn 100` 的写法。
命令行参数默认值（`argc>1 ? atoi(argv[1]) : 100`）不会触发 —— 那是对的写法。"""))

R.append(("PHYS-0030", "程序输出了「活度」这类绝对量", 8, "M8", "high", "question", """  source: code
  present: ['Bq']
  checks:
    - kind: always
      label: '输出绝对量'""", """程序输出了绝对值（Bq 之类）。""",
    """请把从源强到最终结果的**完整公式和单位链**写出来，逐步标出每个量的单位。

另外，如果这是活化产物，请列出该核素**全部**主要发射线及强度，
并说明你只用了其中哪几条、为什么。

实测两例（都是"看着对、其实错"）：
- T3-1：公式多除了一个衰变常数、少乘了束流面积，净放大 **1.2e7 倍**。
  但它的时间演化曲线形状完全正确（365 天 → 0.8769）—— 只看曲线发现不了。
- T3-3：只发 Na-24 的 1.369 MeV 一条 γ，漏掉 2.754 MeV 主线。
  按 NIST 的质能吸收系数，2.754 MeV 那条占空气比释动能的 62.1% ——
  结果低 **2.59 倍**。

一个日常核对法：把中间量按量纲一路乘下来，看看最后是不是 Bq。
单位链上错一步，数值就错一个量级，但程序不会报错。""",
    ['本机实测：T3-1 `code/T3-1/code.cc:87` 的 `Rp = phi * pCap / V`（少乘束流面积 πR²=19.635 cm²），`:90` 的 `A0 = Rp*(1-exp(-lam*Tirr))/lam`（多除 lam）',
     '本机实测：T3-1 打印 A0=1.2342e13 Bq/cm³，正确值 1.0098e6 Bq/cm³（净放大 1.222e7 倍）；交叉验证：打印值 × 样品体积 = 2.4234e14 = 正确的 Co-60 原子数密度',
     '本机实测：T3-3 只发 1.369 MeV；按 NIST 空气 µ_en/ρ（1.369→0.0261、2.754→0.0213），2.754 线占空气比释动能 62.1%；正确 D_dot(1 GBq,10cm)=4.32e4 µGy/h vs 实测 1.67e4 µGy/h'],
    """## 现象

程序直接打印一个绝对量：

```
A0 = 1.2342e+13 Bq/cm3
```

## 为什么是问题

绝对量 = 蒙特卡洛计数 × 一堆解析换算。
蒙特卡洛那一半程序自己算，**换算那一半是写代码时手推的**。

于是有两类错会全部落到这个数上：

1. **公式错**（T3-1）：多一个除、少一个乘，放大 1.2e7 倍。
   时间演化曲线形状还对，因为 λ 的部分没错。
2. **输入源项不全**（T3-3）：只发一条 γ，漏掉最主的那条，
   整体低 2.59 倍。

两类都不会触发任何 Geant4 检查。程序退出码 0。

## 为什么这类错特别难发现

- **形状对**：T3-1 的曲线完全正常（365 天 → 0.8769，符合 Co-60 半衰期）。
- **量级不荒谬**：1.2e13 Bq/cm³ 对一个"被中子照了 30 天的钴样品"来说，
  不是明显离谱的数。
- **没有对照**：程序不告诉你正确答案是多少。

## 判据

只抓"输出里出现 Bq"。不试图验算 —— 程序做不到。
它的作用是**把注意力引到单位链上**，让人（或模型自己）去逐步核对。

## 已知局限

- 只抓 `Bq`。输出 Gy/h、Sv/h 这类绝对量不会触发
  （T2-1 的 µGy/h 就没被抓到）。
- 程序不做任何数值验证，所以假警报（单位链正确时也问）是常态 ——
  按新目标这是可接受的。"""))


def emit():
    import yaml
    n = 0
    for (rid, title, stage, mech, sev, out, trig, verdict, question, sources, body) in R:
        meta = {
            "id": rid, "title": title, "stage": stage, "mechanism": mech,
            "severity": sev, "output": out,
            "detect": "code" if "source: code" in trig else "snapshot",
            "applies_to": ["all"], "status": "implemented",
            "trigger": yaml.safe_load(trig),
            "verdict": verdict.strip() + "\n",
        }
        if question:
            meta["question"] = question.strip() + "\n"
        meta["sources"] = sources
        meta["authors"] = ["待填"]
        meta["reviewed_by"] = []
        fm = yaml.safe_dump(meta, allow_unicode=True, sort_keys=False, width=1000)
        (OUT / f"{rid}.md").write_text(f"---\n{fm}---\n\n{body}\n", encoding="utf-8")
        n += 1
    print(f"写出 {n} 条规则")


if __name__ == "__main__":
    emit()
