# 按致错机制倒排

共 66 条规则。本文件由 `build.py` 自动生成，**不要手改**。

## M1（8）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0009](rules/PHYS-0009.md) | Birks 抑制没开（有机闪烁体场景下剂量会被高估） | snap/em_parameter | question | high | implemented |
| [PHYS-0010](rules/PHYS-0010.md) | 极化效应没开 | snap/em_parameter | question | medium | implemented |
| [PHYS-0011](rules/PHYS-0011.md) | 荧光 / 俄歇电子没开 | snap/em_parameter | question | medium | implemented |
| [PHYS-0015](rules/PHYS-0015.md) | LPM 效应是开着的（高能电子/正电子） | snap/em_parameter | notice | low | implemented |
| [PHYS-0025](rules/PHYS-0025.md) | 用了闪烁体，但没设「猝灭系数」（Birks） | code | violation | high | implemented |
| [PHYS-0031](rules/PHYS-0031.md) | 热中子散射截面库 (TSL) 与材料名规范 | code | violation | high | implemented |
| [PHYS-0044](rules/PHYS-0044.md) | 快中子门槛反应 (n,2n) 截面与阈能衔接 | code | violation | high | implemented |
| [PHYS-0062](rules/PHYS-0062.md) | 放射性衰变延迟中子通道与前驱核寿命 | code | violation | medium | implemented |

## M10（4）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0005](rules/PHYS-0005.md) | 过程上多个模型的能量段之间有缝隙，某段能量没人管 | snap/process | violation | high | implemented |
| [PHYS-0006](rules/PHYS-0006.md) | 模型的能量范围是空的（不负责任何能量） | snap/model | violation | high | implemented |
| [PHYS-0012](rules/PHYS-0012.md) | 产生截断没有真正杀死低能次级（ApplyCuts） | snap/em_parameter | notice | medium | implemented |
| [PHYS-0014](rules/PHYS-0014.md) | 中子记录/衰变的时间阈值被改动 | snap/hadronic_parameter | violation | medium | implemented |

## M11（2）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0055](rules/PHYS-0055.md) | 随机细胞群多敏感体积单事件击中解耦 | code | violation | high | implemented |
| [PHYS-0066](rules/PHYS-0066.md) | 次级强子产生顶点空间密度单次累加 | code | violation | high | implemented |

## M12（14）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0036](rules/PHYS-0036.md) | 穿墙圆柱管道直通流与反散射解耦 | code | question | medium | implemented |
| [PHYS-0038](rules/PHYS-0038.md) | 宽谱中子辐射权重因子 w_R(E) 积分 | code | question | medium | implemented |
| [PHYS-0039](rules/PHYS-0039.md) | 纳米尺度电离簇尺寸与宏观能损辨识 | code | question | medium | implemented |
| [PHYS-0042](rules/PHYS-0042.md) | 高 LET 辐射微剂量学饱和修正因子 | code | question | medium | implemented |
| [PHYS-0043](rules/PHYS-0043.md) | 放射性衰变母子链 Bateman 方程与分支比 | code | question | high | implemented |
| [PHYS-0045](rules/PHYS-0045.md) | 高能电子/伽马光核反应中子发射物理 | code | violation | high | implemented |
| [PHYS-0046](rules/PHYS-0046.md) | 循环流动介质活化停留时间与有效体积 | code | question | medium | implemented |
| [PHYS-0047](rules/PHYS-0047.md) | 裂变产物衰变热发热功率能量记账 | code | question | high | implemented |
| [PHYS-0048](rules/PHYS-0048.md) | 放射性体内滞留生物清除有效半衰期 | code | question | medium | implemented |
| [PHYS-0051](rules/PHYS-0051.md) | 闪烁晶体外表面光学光学边界属性 | code | violation | high | implemented |
| [PHYS-0052](rules/PHYS-0052.md) | 半导体探测器电荷涨落 Fano 因子加权 | code | question | medium | implemented |
| [PHYS-0054](rules/PHYS-0054.md) | 介质材料折射率属性与契伦科夫辐射使能 | code | violation | high | implemented |
| [PHYS-0058](rules/PHYS-0058.md) | 辐射生物学线性二次模型 (LQ) 单击/双击分量 | code | question | medium | implemented |
| [PHYS-0060](rules/PHYS-0060.md) | 辐射化学自由基扩散反应阶段注册 | code | violation | high | implemented |

## M13（4）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0019](rules/PHYS-0019.md) | 用步点判断「粒子穿过几何边界」 | code | question | high | implemented |
| [PHYS-0020](rules/PHYS-0020.md) | 把 track 的能量当成「刚产生时」的能量 | code | question | high | implemented |
| [PHYS-0021](rules/PHYS-0021.md) | 用「被定义步的过程名」判断次级粒子的来源 | code | question | medium | implemented |
| [PHYS-0050](rules/PHYS-0050.md) | 探测器粒子飞行时间 TOF 步点时钟选取 | code | violation | high | implemented |

## M14（1）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0022](rules/PHYS-0022.md) | 代码里硬编码了两张以上的数值表（谱线表） | code | question | high | implemented |

## M2（3）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0001](rules/PHYS-0001.md) | 高精度（_HP）物理列表会砍掉原模型的适用能量下限 | snap/physics_list | notice | high | implemented |
| [PHYS-0007](rules/PHYS-0007.md) | EM 物理用的是哪一套（option0-4 / Livermore / Penelope） | snap/constructor | notice | medium | implemented |
| [PHYS-0034](rules/PHYS-0034.md) | 多层同心几何共面重叠与卡步校验 | code | violation | high | implemented |

## M3（5）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0008](rules/PHYS-0008.md) | 过程上挂载/替换了截面数据集 | drift/dataset_added | notice | medium | implemented |
| [PHYS-0032](rules/PHYS-0032.md) | 方差缩减几何重要性分裂权重记账 | code | violation | high | implemented |
| [PHYS-0037](rules/PHYS-0037.md) | 超短高剂量率脉冲时间归一化链条 | code | violation | high | implemented |
| [PHYS-0049](rules/PHYS-0049.md) | 探测器高斯能量展宽 FWHM 与标准差转换 | code | violation | high | implemented |
| [PHYS-0053](rules/PHYS-0053.md) | 高计数率脉冲堆积与可麻痹死时间判据 | code | violation | high | implemented |

## M4（10）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0003](rules/PHYS-0003.md) | 模型的适用能量范围被静默改写 | drift/energy_shift | violation | high | implemented |
| [PHYS-0004](rules/PHYS-0004.md) | 隐藏漂移 —— 只在收尾扫描才发现的改动 | drift/hidden_shift | violation | high | implemented |
| [PHYS-0013](rules/PHYS-0013.md) | 强子截面被整体缩放了 | snap/hadronic_parameter | violation | high | implemented |
| [PHYS-0035](rules/PHYS-0035.md) | 大气宇宙线天顶角与动量角抽样 | code | violation | high | implemented |
| [PHYS-0041](rules/PHYS-0041.md) | 旋转照射动量方向与源坐标同步变换 | code | violation | high | implemented |
| [PHYS-0056](rules/PHYS-0056.md) | BNCT 伴随粒子产生顶点对齐 | code | violation | high | implemented |
| [PHYS-0059](rules/PHYS-0059.md) | 放射性微球近距离放疗连续 Beta 能谱抽样 | code | violation | high | implemented |
| [PHYS-0063](rules/PHYS-0063.md) | 裂变中子能谱 Watt 谱参数抽样解析公式 | code | violation | high | implemented |
| [PHYS-0064](rules/PHYS-0064.md) | 初始高斯能展动能下限非负截断保护 | code | violation | high | implemented |
| [PHYS-0065](rules/PHYS-0065.md) | 圆形扩展面源径向均匀抽样平方根法则 | code | violation | high | implemented |

## M5（2）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0028](rules/PHYS-0028.md) | 用了跨事件的去重集合，但没在每事件清空 | code | question | high | implemented |
| [PHYS-0033](rules/PHYS-0033.md) | 超薄靶低能光子次级电子产生截断 | code | violation | high | implemented |

## M6（7）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0023](rules/PHYS-0023.md) | 算了「效率」，但代码里没有任何立体角 / 几何接受度 | code | question | high | implemented |
| [PHYS-0024](rules/PHYS-0024.md) | 注释说「点源」，代码里发射方向却是写死的常量 | code | question | high | implemented |
| [PHYS-0026](rules/PHYS-0026.md) | 程序里有「死层」体积 | code | question | medium | implemented |
| [PHYS-0029](rules/PHYS-0029.md) | 事件数写死在代码里 | code | question | medium | implemented |
| [PHYS-0040](rules/PHYS-0040.md) | 重金属纳米微颗粒界面微步长限制 | code | violation | high | implemented |
| [PHYS-0057](rules/PHYS-0057.md) | 纳米低能电子径迹结构法 (Track Structure) | code | violation | medium | implemented |
| [PHYS-0061](rules/PHYS-0061.md) | 强偏转磁场最大弦长与弦积分器精度配置 | code | violation | high | implemented |

## M7（1）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0027](rules/PHYS-0027.md) | 用球体做灵敏体积（微剂量学） | code | question | high | implemented |

## M8（1）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0030](rules/PHYS-0030.md) | 程序输出了「活度」这类绝对量 | code | question | high | implemented |

## M9（4）

| ID | 标题 | 触发 | 输出 | 严重度 | 状态 |
|---|---|---|---|---|---|
| [PHYS-0002](rules/PHYS-0002.md) | 物理列表之外额外注册了构造器 | snap/physics_list | notice | high | implemented |
| [PHYS-0016](rules/PHYS-0016.md) | 链条里有衰变过程（延迟粒子会被输运） | snap/process | notice | medium | implemented |
| [PHYS-0017](rules/PHYS-0017.md) | 物理列表是自己拼的，不在 Geant4 官方清单里 | snap/physics_list | question | high | implemented |
| [PHYS-0018](rules/PHYS-0018.md) | 装了非弹性强子物理，却没装弹性 | snap/physics_list | violation | high | implemented |
