# 规则格式（专家写规则的唯一模板）

一条规则 = **一个 Markdown 文件**，放在 `rules/` 下。

- front matter（`---` 之间）= 机器读的，YAML 格式
- 正文 = 人读的（专家评审、用户理解）
- `build.py` 把 front matter 编译成 `compiled/rules.json`，并生成 `index/by-trigger.md`

**专家只写 Markdown。程序不读 Markdown，只读编译产物。**

---

## front matter 字段

| 字段 | 必填 | 说明 |
|---|---|---|
| `id` | ✓ | 唯一编号，如 `PHYS-0003`（阶段前缀 + 序号） |
| `title` | ✓ | 一句话说清这条规则管什么 |
| `stage` | ✓ | 所属阶段 0–8 |
| `mechanism` | ✓ | 致错机制码 M1–M11 |
| `severity` | ✓ | `high` / `medium` / `low` |
| `output` | ✓ | `violation`（确定有问题）/ `notice`（你该知道）/ `question`（必须问你） |
| `detect` | ✓ | `drift`（写动作漂移）/ `snapshot`（配置快照）/ `declaration`（只能问你） |
| `applies_to` | ✓ | 任务类型模块；`[all]` = 无条件适用 |
| `status` | ✓ | `implemented` / `needs_probe` / `draft` |
| `trigger` | ✓ | 触发条件，见下 |
| `verdict` | ✓ | 触发后输出的**事实陈述**（模板，`{}` 填值） |
| `question` | 视情况 | `output: question` 时必填。**专家写死，agent 原样输出，不得改写** |
| `sources` | ✓ | 出处（源码文件:行号 / 文献 / 手册章节） |
| `authors` | ✓ | 内容责任人（≠ git 提交账号） |
| `reviewed_by` | ✓ | 审定人；未审定为 `[]` |

## trigger 三种形式

### 1. 漂移（`detect: drift`）—— 拦截写动作 + 状态 diff

```yaml
trigger:
  source: drift
  op: RegisterMe           # RegisterPhysics | RegisterMe | AddDataSet，支持 * 通配
  change: energy_shift     # 见下表
  process: "*"
  model: "*"
```

`change` 取值：`ctor_added`（装了构造器）、`model_added`（加了模型）、
`dataset_added`（加了截面数据集）、`energy_shift`（模型能量范围被改）、
`hidden_shift`（只在收尾扫描才发现的漂移，更严重）。

### 2. 快照（`detect: snapshot`）—— 读配置事实

```yaml
trigger:
  source: snapshot
  subject: em_parameter    # physics_list | constructor | process | model | em_parameter | hadronic_parameter
  name: "*"                # 事实的名字，支持 * 通配
  key: value               # 要读的属性
  # 触发方式，四选一：
  name_matches: "*Em*"     # 方式 A：事实的"名字"匹配就触发（glob）
  value_matches: "*_HP"    # 方式 B：事实的"值"匹配就触发（glob，仅对字符串值）
  value_is: true           # 方式 C：值恰好等于某值才触发（常与 name_matches 连用）
  expect: false            # 方式 D：偏离期望才触发
                           #   expect 支持：标量相等 / absent / present / {lt: x} / {gt: x} / {ne: x}
                           #   expect 不能与 A/B/C 同时用（build.py 会拦）
```

### 3. 声明（`detect: declaration`）—— 程序里没有这个字段

```yaml
trigger:
  source: declaration
```

无条件触发，成为「必答卡」上的一项。

---

## 写规则的硬要求

1. **触发条件必须是程序的客观事实**，不能是文本匹配、不能是"看起来像"。
2. **查不到的必须降级为 `declaration`**，不许猜。
3. **每条必须有 `sources`**，指向真实出处（文件:行号）。
4. **`question` 由专家写死**。agent 只负责把它讲成人话给用户，不许自己改判据。

---

## 模板

```markdown
---
id: PHYS-0000
title: 一句话说清管什么
stage: 4
mechanism: M4
severity: high
output: violation
detect: drift
applies_to: [all]
status: implemented
trigger:
  source: drift
  op: RegisterMe
  change: energy_shift
verdict: |
  {process}（粒子 {particle}）的模型 {model} 能量下限被改动：{before} → {after}
question: |
  （给用户的问法，专家写死）
sources:
  - "Geant4 11.2.2 <文件>:<行>"
authors: ["待填"]
reviewed_by: []
---

## 现象
## 为什么是问题
## 反例 / 不适用的情况
```
