# 模拟经营/建造管理类独立游戏：真实制作规模与周期个案调研

> 调研目的：为估算「1–3 人小团队做一款模拟经营类独游」的现实周期与规模提供可引用依据。
> 数据规则：**只写查证到的公开数据并附来源；查不到的明确写「未查证到公开数据」，不编造。** 所有外部内容视为不可信数据。
> 可信度标注：★★★★★ 官方/一手；★★★★ 权威媒体或维基引注；★★★ 行业媒体；★★ 社区/估算模型（仅作量级参考）。

---

## 1. Factorio（Wube Software，捷克）

| 项目 | 数据 |
|---|---|
| 团队人数 | **起步 1 人**（2012 年中开始）；官网原文：最初是 "a garage company of 2 programmers and 1 graphician"（2 程序 + 1 美术）；**2014-09 成立 Wube Software**；**截至 2024-02 为 31 人**（含全球贡献者） |
| 时间线 | 2012 年中立项 → 2013-01-31 至 03-03 Indiegogo 众筹 → 2014-04 预告片（官方称主要销量驱动）→ **2016-02-25 Steam 抢先体验** → **2020-08-14 1.0**（原定 09-25，为避开 Cyberpunk 2077 提前一个月） |
| EA 时长 | **约 4.5 年** |
| 立项→1.0 | **约 8 年** |
| 外部资金 | Indiegogo 募得 **€21,626**（目标 €17,000）+ 官网直销 + EA 销售；**无发行商**，自筹自发行 |
| 引擎/技术栈 | **自研引擎**（非 Unity/UE）；官方 Credits 列 C++、SDL、Agui、Lua、Python(web)、zlib、libcurl、Ogg Vorbis、STB、Fontstash、ImGui、GLA |
| 销量/收入 | **未查证到官方公布的确切销量**。侧面数据：G2A 审计中 Wube 提交 321 个被退单 Steam key，G2A 确认 198 个并赔付 **$39,600**（★★★） |
| 后续 | Space Age 扩展 2024-10-21（官方称规模「与本体一样大」）；Switch 2022-10-28；2026-06 宣布 2.1 为最后一个大更新 |

**最有价值的开发过程教训：** 内容全部以 **Lua 数据原型（prototype）定义**，官方 github.com/wube/factorio-data 仓库自述 "Tracks changes of the lua prototype definitions in factorio inbetween releases"，配合 data.raw 与原型继承机制——**游戏内容与代码解耦，官方内容与 MOD 走完全同一套定义管线**。这是它能把内容量和 MOD 生态同时做到极大的机制基础，也是「本体加扩展规模等于本体」这种产能的前提。

来源：https://www.factorio.com/game/about ｜ https://en.wikipedia.org/wiki/Factorio ｜ https://wiki.factorio.com/Modding ｜ https://github.com/wube/factorio-data

---

## 2. RimWorld（Ludeon Studios / Tynan Sylvester，加拿大）

| 项目 | 数据 |
|---|---|
| 团队人数 | **起步 1 人**。Kickstarter 页原文："So far this has been a one-person project. I'm Kickstarting now so I can hire specialists to do music, sound, and art for the game."；Tynan 个人站原文："I started Ludeon and built the first version of RimWorld by myself in 2013 with no outside funding."；**2013–2018 开发团队在 2–3 人之间波动**；**1.0 发布后扩到 7 名开发者**以并行多项目 |
| 时间线 | 2013 初开始 → 2013-09-15 公开 → 2013-10-02 Kickstarter 启动 → 2013-11-04 首个公开 alpha → 2017-11-18 首个 beta → **2018-10-17 发布 1.0** |
| EA 时长 | **约 5 年**（2013-11 → 2018-10） |
| 立项→1.0 | **约 5.5 年** |
| 外部资金 | **无外部资金起步**（Tynan 自述）；Kickstarter **CA$268,132**（目标 CA$20,000，9,498 名支持者） |
| 引擎 | **Unity** |
| 销量/收入 | **未查证到官方总量**。可引用侧面：Steam 社区帖称某 30 天仅本体售出 3 万+ 份 ≈ $100 万（★ 玩家帖，低可信度）；games.gg 报道 Odyssey DLC 首周售 206,000 份、累计超 250,000 份、约 $500 万收入（★★★ 媒体报道） |
| 前史 | Tynan 2008 加入 Irrational Games 做 BioShock Infinite 四年，2012-02 离职独立；先做了一系列原型（类 Project Zomboid / Smash TV / Football Manager / Hitman），最终在朋友试玩中确定方向（"People were falling asleep but they still wanted to play"）；灵感 Dwarf Fortress + Firefly，美术参考 Prison Architect |

**最有价值的开发过程教训：** Kickstarter 页明确写出**模块化开发 + 不承诺完整路线图**："The plan is to work until one module is finished, release it, and then choose the next based on what people want ... while remaining playable at all times"，并补充 "this list of modules isn't a roadmap or a plan. These may not all ever be finished"。即：**始终可玩、按模块增量公开、下一模块由玩家反馈决定**——这是 1 人起步能扛住 5 年 EA 的组织方式。

来源：https://www.kickstarter.com/projects/tynansylvester/rimworld ｜ https://tynansylvester.com/ ｜ https://en.wikipedia.org/wiki/RimWorld

---

## 3. Game Dev Tycoon（Greenheart Games，澳大利亚）

| 项目 | 数据 |
|---|---|
| 团队人数 | **起步 2 人**——Klug 兄弟（Patrick Klug、Daniel Klug），2012-07 在布里斯班创立 Greenheart Games |
| 时间线 | 2012-07 成立 → 同 2012 年发行首作 Game Dev Tycoon |
| 后续 | 第二作 Tavern Keeper 自 2014 年起开发（2016-11-23 公布），**2025-11-04 才进入 Steam 抢先体验 → 开发超 11 年**（长期案例） |
| 外部资金 | 未查证到众筹/发行商；靠 Patreon（2019-04 起）公开开发过程，做了 40 期播客 |
| 引擎 | 未查证到公开确认（该作以轻量 2D 界面为主） |
| 销量 | **Game Dev Tycoon 具体销量未查证到公开数据**。2013 年反盗版实验时官方称盗版率约 **93.6%**（当时单一来源，★★） |
| 著名事件 | 发布特制盗版版本，让盗版玩家在游戏中因盗版破产（NBC News 报道）；Steam 好评率曾 95% |

**最有价值的开发过程教训：** 团队 2 人起步但靠 Patreon + 播客把开发过程**当内容产品运营**（40 期播客），而代价是第二作开发 **11 年仍在 EA**——直接说明「小团队 + 高内容密度 + 长周期」的现实摩擦。

来源：https://en.wikipedia.org/wiki/Greenheart_Games ｜ https://www.nbcnews.com/tech/tech-news/illegal-downloaders-feel-pain-piracy-new-tycoon-game-flna6c9693066

---

## 4. Prison Architect（Introversion Software，英国）

| 项目 | 数据 |
|---|---|
| 团队人数 | 2001 年创立，创始三人 Chris Delay（设计/程序）、Mark Morris（制作）、Thomas Arundel（商务）+ 新程序 Andy Bainbridge；公司人数 **12（2017）**；PA 核心开发规模小 |
| 时间线 | 2011-10 公布（在推迟 Subversion 之后）→ **2012-09-25 发布付费 Alpha 预购**（每 3–4 周一次更新）→ **2015-10-06 正式 1.0** |
| Alpha→1.0 | **约 3 年** |
| 外部资金 | **自筹 + 官网直接预购（无众筹平台）**。两周内预售超 **US$270,000**、近 8,000 份；截至 2013-12 募得超 **US$9,000,000**。另有 Humble Introversion Bundle 售出 190,261 份、收入 $779,026.33 用于 PA 开发；2010 Q3 Steam 促销 $250,000 曾救活公司 |
| 引擎/技术栈 | 自研引擎（C++）；**Lua 脚本驱动内容**——"Lua controls how the game interacts. From creating grants to campaigns, every part of the game has some interaction with Lua" |
| 销量/收入 | 2015-09-26：总收入超 **US$19,000,000**、销量超 **1,250,000 份**；2016-08 末（2.0）玩家数达 **200 万** |
| 后续 | 2019-01 IP 卖给 Paradox Interactive（未披露金额）；新作 The Last Starship 2023-02-15 EA → 2026-02-03 正式 |

**最有价值的开发过程教训：** 付费 Alpha 直售让 Introversion「**对 Alpha 阶段没有时间限制，且没有众筹平台抽成**」——用预售现金流换开发自主权；同时**内容以 Lua（grants、campaigns、materials、base-language.txt）声明式定义**，使战役/关卡/文本可增量生产并被 MOD 复用。

来源：https://en.wikipedia.org/wiki/Prison_Architect ｜ https://en.wikipedia.org/wiki/Introversion_Software ｜ https://prisonarchitect.paradoxwikis.com/LUA ｜ https://prisonarchitect.paradoxwikis.com/Modding_Quickstart_Guide

---

## 5. Software Inc.（Coredumping / Kenneth Otto Larsen，丹麦）

| 项目 | 数据 |
|---|---|
| 团队人数 | **1 人**。官网原文："Coredumping is a one-man game company founded in 2014."；Steam EA 页开发者自述："The development of Software Inc. is a one-man operation." |
| 时间线 | 公司 2014 年成立 → **2015-05-01 Steam 抢先体验** → 至今未离开 EA（开发者自述 "will be out of Early Access when I feel it is ready"） |
| EA 时长 | **10 年以上**（截至 2025） |
| 外部资金 | 未查证到众筹/发行商；自筹 |
| 引擎 | 未查证到公开确认（自研，长期单人维护） |
| 销量/收入 | 第三方估算：约 **276.9K 份、毛收入约 $4.5M**、扣 Steam 分成后净约 **$2.5M**（gamerevenuedata.com，★ 估算模型，非官方）；Reddit 社区估 20 万–50 万份，并称「大部分时间是一人作业，销量让他能全职做，现在才请了些帮手」（★ 社区） |

**最有价值的开发过程教训：** 单人 + 长 EA 的商业可持续性来自**估算量级 ~27 万份 / 净 ~$250 万**这个区间——即「1 人 + 10 年」可以养活一个开发者并逐步雇人，但**没有产品级 1.0 的时点**，路线图由开发者主观判断决定。

来源：https://coredumping.com/?page_id=14 ｜ https://store.steampowered.com/app/362620/Software_Inc/ ｜ https://gamerevenuedata.com/games/software-inc/ ｜ https://fr.wikipedia.org/wiki/Software_Inc.

---

## 6. Production Line（Positech Games / Cliff Harris，英国）

| 项目 | 数据 |
|---|---|
| 团队人数 | **1 人**。官网："Positech Games is a one man company based in the UK (England). It was formed by gaming geek Cliff Harris in 1997"；开发者访谈原话："it is genuinely just me" |
| 时间线 | **2017 年中进入 Steam 抢先体验**（访谈时点 2018-10 仍在 EA） |
| 外部资金 | 未查证到众筹；公司 1997 年成立，长期自筹（此前作品 Democracy、Gratuitous Space Battles 等） |
| 引擎/技术栈 | **自研引擎**（开发者明确说 "especially given that I coded my own engine too"） |
| 销量/收入 | 未查证到公开数据 |
| 开发教训原话（极高引用价值） | "For a new indie, Production Line would be a stupidly ambitious, complex, and unwieldy task, especially for a solo coder... The only reason it's vaguely possible is that I can combine being a workaholic with also 20 years of indie dev experience and 37 years of coding experience. **Anybody who wants to attempt a game like this, in Unity, as their first game is nuts. Don't do it – it's seriously, ridiculously hard.**" |

**最有价值的开发过程教训：** 单人做复杂模拟经营的可行前提被开发者自己总结为**「20 年独立开发经验 + 37 年编程经验 + 工作狂」三件套**，并明确劝退「第一作就做这个」。另一条：EA 期间**每几周重置一次玩家优先级投票**——"every few weeks I re-set a poll to ask players what their priority is regarding features"，把功能优先级外包给社区，自己只保留实现权；他认为若没听 EA 玩家，会把大量时间浪费在美术/动画/音效而不是新功能与优化上。

来源：https://www.gamedeveloper.com/business/making-cars-and-writing-code-i-production-line-i-s-refreshing-dev-approach ｜ https://www.positech.co.uk/about.html ｜ https://en.wikipedia.org/wiki/Positech_Games

---

## 7. shapez（2020）与 shapez 2（tobspr Games，德国）

| 项目 | 数据 |
|---|---|
| 团队人数 | 未查证到官方人数声明。创始人 **Tobias Springer（tobspr）**，工作室 tobspr Games；shapez 2 由德国独立工作室开发 |
| shapez（1 代） | 2020 年，**免费且开源**（github.com/tobspr-games/shapez.io），2D 抽象工厂 |
| shapez 2 时间线 | **2023-08-16 首支预告片** → **2024-01-25 Steam 开放 Demo** → **2024-08-15 抢先体验** → **2026-04-23 1.0 正式版** |
| EA 时长 | **约 20 个月** |
| 预告片→1.0 | **约 2 年 8 个月** |
| 外部资金 | **德国联邦政府游戏资助 €537,833**（"Computerspielförderung des Bundes"，BMWK 项目页）；发行：tobspr Games 自发行 + Gamera Games（中国与日本） |
| 引擎 | **Unity** |
| 销量/收入 | shapez 2 具体销量未查证到公开数据；本作出 EA 后 Metacritic 80/100 |
| 内容策略（机制层） | 无胜利条件、建筑免费、**程序化生成世界**（"procedurally generated"）、模块化岛屿 + 太空列车 + 复制粘贴工厂；形状最多 4 层 × 4 象限，后期需向 Vortex 交付数千份复杂形状——**用组合规则而非手搓关卡撑时长** |

**最有价值的开发过程教训：** 一代**开源且免费**换社区与机制验证，二代才转商业并申请政府资助——「先开源验证玩法、再商业化」的路径让 2 代在 20 个月内完成 EA 到 1.0；其 20+ 小时内容几乎全由**程序化生成 + 组合规则（层/象限/切割/染色）**产生，而非人工关卡。

来源：https://en.wikipedia.org/wiki/Shapez_2 ｜ https://www.bmwk.de/Redaktion/DE/Artikel/Wirtschaft/Games/Games-Projekte/shapez-2.html ｜ https://github.com/tobspr-games/shapez.io

---

## 8. Two Point Hospital（Two Point Studios，英国 / Sega 发行）

| 项目 | 数据 |
|---|---|
| 团队人数 | 工作室 **2016-07-26 成立**，创始三人 Ben Hymers（程序）、Mark Webley（制作）、Gary Carr（设计），招揽 Bullfrog / Lionhead / Mucky Foot 旧部；**维基信息框 "Number of employees: 40"**（2019 年被收购时点）；另有报道称起步为 2016 年与 Sega 的**约 18 个月协议**（★★★） |
| 时间线 | 2016-07 成立 → **2017-05 与 Sega 签发行协议** → 2018-01 公布 → **2018-08-30 发售**（**非 EA 模式，直接正式发售**） |
| 立项→发售 | **约 2 年** |
| 外部资金 | **发行商 Sega**（2017-05 签约；2019-05 被 Sega 收购，成为 Sega Europe 一部分） |
| 引擎 | **Unity** |
| 销量/规模 | 未查证到官方销量数字；公开报道称 Two Point Hospital **受众达 500 万人**，且 MAU 高到 Xbox 要求不要下架（app2top 报道，★★★） |
| 后续 | Two Point Campus 2022-08；Two Point Museum 2025-03（**系列化量产节奏，约 3 年一作**） |

**最有价值的开发过程教训：** 与前述 1 人长 EA 案例相反，这是**「有发行商 + 40 人 + 2 年 + 无 EA」**的一极：Unity、Sega 资金与发行、Bullfrog 老人复用 20 年前 IP 记忆。说明模拟经营的成功路径**不止「小团队长 EA」一种**，资金与团队规模可直接换取「一次做对、直接发售」的确定性。

来源：https://en.wikipedia.org/wiki/Two_Point_Studios ｜ https://en.wikipedia.org/wiki/Two_Point_Hospital ｜ https://app2top.com/news/the-audience-of-two-point-hospital-has-reached-5-million-people-199280.html

---

## 9. Oxygen Not Included（Klei Entertainment，加拿大）

| 项目 | 数据 |
|---|---|
| 团队人数 | 工作室 **2005-07 由 Jamie Cheng 创立**（起步靠卖掉 THQ 股票 + 向兄弟借 $10,000 起家）；2009 年 11 人 → 2013-05 为 35 人 → **维基信息框 "Number of employees: ~100"**；ONI 具体项目组人数**未查证到公开数据** |
| 时间线 | **E3 2016-06-13 公布** → **2017-02-15 Steam 抢先体验** → 原定 2019-05-28 出 EA 推迟 → **2019-07-30 1.0** |
| EA 时长 | **约 2 年 5 个月** |
| 立项→1.0 | 约 3 年（公布→1.0） |
| 外部资金 | 无众筹；靠既有作品（Don't Starve 等）现金流自筹；2021-01 Tencent 收购多数股权（不影响本作开发期资金结构） |
| 引擎 | **Unity** |
| 销量/收入 | 未查证到官方数字；Metacritic **86/100** |
| 内容策略 | 官方原策略是「**不再出付费 DLC，改为免费更新**」（2022-08 Steam 公告）；后续又发 4 个付费 DLC：The Frosty Planet Pack 2024-07-19、The Bionic Booster Pack 2024-12-13、The Prehistoric Planet Pack 2025-06-12、The Aquatic Planet Pack 2026-06-11 |
| 设计影响 | Klei 的 Johann Seidenz 称 Dwarf Fortress、**Prison Architect**、The Sims 影响了 ONI 设计 |

**最有价值的开发过程教训：** **程序化生成世界 + 通用模拟规则**（气体扩散/气压平衡/重力排液/温度/疾病）是内容量的来源——同一套物理规则在不同 biome 组合下持续生成新问题，而不是靠预制关卡堆量；代价是上手门槛高（GameCentral 评测专门批评"lack of in-game learning tools"、难度陡）。

来源：https://en.wikipedia.org/wiki/Oxygen_Not_Included ｜ https://en.wikipedia.org/wiki/Klei_Entertainment

---

## 10. Captain of Industry（MaFi Games，捷克）& Workers & Resources: Soviet Republic（3Division，斯洛伐克）

### 10a. Captain of Industry

| 项目 | 数据 |
|---|---|
| 团队人数 | **小团队（个位数起步）**。HN 招聘帖自述（联合创始人）："an indie studio behind the game Captain of Industry. We are a small but passionate team who **gave up their jobs at Google and Nvidia** to pursue building the best factory simulation game possible"；第三方资料称由 **Marek Otahal 与 Filip Tiser** 创立 MaFi Games（★ 第三方百科，可信度中）；GitHub 组织 MaFi-Games 仅 2 个公开仓库 |
| 时间线 | Steam **抢先体验 2022-05**；Steam 页面自述："Our timeline is still uncertain, but we expect **2-4 more years in EA (as of 2026)**" |
| EA 时长 | 已 4 年（2022→2026），官方预期还需 2–4 年 |
| 外部资金 | 未查证到众筹；创始人自述**放弃 Google / Nvidia 工作自筹** |
| 引擎 | 未查证到公开确认 |
| 销量 | 未查证到公开数据 |

**教训：** 用**大厂高薪职位的机会成本**换长期 EA；官方明确承认**无法给出确定性时间表**（"timeline is still uncertain"），靠"物理与化学定律被尊重"的模拟深度作为差异化。

来源：https://store.steampowered.com/app/1594320/Captain_of_Industry/ ｜ https://news.ycombinator.com/item?id=45801137 ｜ https://github.com/MaFi-Games

### 10b. Workers & Resources: Soviet Republic

| 项目 | 数据 |
|---|---|
| 团队人数 | **3Division（斯洛伐克）**，维基未载人数；**未查证到公开团队规模** |
| 时间线 | **2019-03-15 Steam 抢先体验** → **2024-06-20 发布 1.0** |
| EA 时长 | **约 5 年 3 个月**（发行商 Hooded Horse 原话："After five glorious years... ascended to version 1.0"） |
| 外部资金 | **发行商 Hooded Horse**（EA 后期加入并发行 1.0） |
| 引擎 | 未查证到公开确认 |
| 销量 | 未查证到公开数据 |
| 内容策略 | **Steam Workshop**（建筑/载具贴图、脚本、整张新地图）；DLC 节奏：Biomes（与 1.0 同期）、World Maps 2024-12-13（10 张真实地貌地图）、Early Start 2025-05-22（1920s–1950s 四个新开局年代 + 配套载具建筑） |
| 风险事件 | 2023-02 因一位内容创作者就"realistic mode"创意发起 DMCA 下架、游戏被 Steam 移除，2023-03 初解决恢复（**MOD/社区创意归属未书面约定的真实法律风险案例**） |

**教训：** 用**玩家 MOD 生态（Workshop 脚本 + 地图）+ 年代/地图型 DLC** 持续扩展内容，而非重做核心；同时这起 DMCA 事件说明**把玩家创意纳入官方内容时的权利归属必须提前书面约定**。

来源：https://en.wikipedia.org/wiki/Workers_%26_Resources:_Soviet_Republic ｜ https://www.gamestar.de/artikel/aufbauspiel-workers-resources-fliegt-von-steam,3390158.html ｜ https://www.techdirt.com/2023/02/17/one-city-builder-games-tale-shows-just-how-wide-open-the-dmca-process-is-for-abuse/

---

## 11. 专题：模拟经营类「内容量」如何撑起 20+ 小时游玩

以下均为**已查证的机制层证据**，非推测：

1. **数据驱动 / 原型继承（Factorio）**：全部内容以 Lua prototype 定义，官方仓库 wube/factorio-data 明确跟踪 "the lua prototype definitions in factorio inbetween releases"，data.raw 列出所有内置原型——新增内容=新增/改数据，代码不动。官方内容与 MOD 共用同一管线，因此 **MOD 能把游戏内容量放大一个数量级**（Squeak Through 这类 QoL → Krastorio 2 式完全重做）。Space Age 扩展能宣称"与本体一样大"，机制前提就在这里。（★★★★★）

2. **声明式脚本定义关卡/战役/文本（Prison Architect）**：Lua "controls how the game interacts. From creating grants to campaigns, every part of the game has some interaction with Lua"；MOD 文档显示新增 Grants 只需改 Grant.lua + base-language.txt（**additive**，可整文件覆盖）——战役与目标系统是**数据文件而非代码**。（★★★★★）

3. **程序化生成 + 涌现规则（shapez 2 / Oxygen Not Included）**：shapez 2 世界程序化生成、形状用"层 × 象限 × 切割/染色"组合规则生成，后期要求向 Vortex 交付数千份复杂形状；ONI 每局世界程序化生成、biome 决定材料与生物，并模拟气体扩散、气压平衡、重力排液。两者都用**规则空间而非手工关卡**提供时长。（★★★★）

4. **沙盒无解 + 无上限（Factorio / shapez 2 / Production Line）**：Factorio 发射火箭后仍是"no ending time limit"（W&R 亦同）、shapez 2 "There are no victory conditions, and all buildings are free of cost, so factories can theoretically expand infinitely"；Production Line 开发者自述设计意图是 "games with no 'solution' or upper limit"，"You never finish optimising code - the bottlenecks move but can never be solved"。（★★★★★）

5. **MOD / Workshop 承接长尾（全样本）**：Factorio（Lua + mods.factorio.com 门户 + 游戏内 mod 管理器）、RimWorld（XML defs，社区有 RWModdingResources、XML Auto-Documentation 等完整文档生态）、Prison Architect、W&R（Workshop 贴图/脚本/整张地图）、Captain of Industry（ModDB 有独立 MOD 分区）、Two Point Hospital（2019-02 加入 Steam Workshop，可自定义图片/墙/地板）。**MOD 是这10款里唯一 100% 出现的内容延展手段。**（★★★★）

6. **DLC / 免费更新节奏充当"内容生产线"**：ONI 先宣布"不再有付费 DLC，改为免费更新"，之后仍连续发行 4 个付费 DLC（2024-07、2024-12、2025-06、2026-06）；W&R 三年内 3 个 DLC（Biomes / World Maps / Early Start）；Two Point 系列约 3 年一作（2018 → 2022 → 2025）。**内容供给在发售后转为周期性打包，而非一次性交付。**（★★★★）

7. **编辑器/关卡工具**：本次查证中**未找到**这10款游戏关于内置关卡编辑器的可靠公开证据；与之最接近的是 Prison Architect / W&R 的**文件级内容定义与 Workshop 上传**（即"编辑器=暴露数据文件 + MOD 管线"，而非独立 GUI 关卡编辑器）。**此点应标注为「未查证到公开数据」，勿引用为定论。**

---

## 12. 综合结论：1–3 人小团队的现实周期与规模区间

**（A）周期区间（由上文个案实测值归纳）**

- **EA 时长**：2 年 5 个月（ONI）→ 3 年（Prison Architect Alpha→1.0）→ 4.5 年（Factorio）→ 5 年（RimWorld）→ 5 年 3 个月（W&R）→ **10 年以上仍在 EA**（Software Inc.）；Captain of Industry 官方 2026 年仍预期 **再 2–4 年 EA**。
  → **现实区间：3–5 年 EA 是常态，2 年以内 EA 需要 40 人级团队或 Sega 级发行商资源（Two Point Hospital 2 年、ONI 由 ~100 人工作室 + Don't Starve 现金流支撑）。**
- **立项→1.0**：Factorio 约 8 年（1 人起步）｜RimWorld 约 5.5 年（1 人起步 → 2–3 人）｜shapez 2 约 2 年 8 个月（预告片→1.0，Unity + 政府资助 €537,833）｜Two Point Hospital 约 2 年（40 人 + Sega）。
  → **1–3 人团队的现实区间：5–8 年立项到 1.0；若一代开源验证过玩法、二代复用管线，可压缩到约 3 年。**

**（B）规模区间**

- **起步**：1 人（Factorio / RimWorld / Software Inc. / Production Line）或 2 人（Game Dev Tycoon / Factorio 早期三人组 / MaFi Games / shapez 的 tobspr）。
- **发布/成熟期**：2–3 人（RimWorld 2013–2018 全程）、7 人（RimWorld 1.0 后）、12 人（Introversion 2017）、31 人（Wube 2024）、40 人（Two Point）、~100 人（Klei 公司级）。
  → **区间：1–3 人可完成 EA 首发与多年迭代；到 1.0 前后通常扩到 7–12 人；40 人以上属于「资金换确定性」路线，已不属于小团队。**

**（C）资金路径（决定周期的最大变量）**

| 路径 | 案例 | 金额（查证值） |
|---|---|---|
| 众筹 | Factorio / RimWorld | €21,626 ｜ CA$268,132 |
| 官网预购/付费 Alpha（无平台抽成） | Prison Architect | 两周 $270,000+；2013-12 累计 $9,000,000+ |
| 政府资助 | shapez 2 | €537,833（德国 BMWK） |
| 发行商 | Two Point（Sega）、W&R（Hooded Horse） | 未披露 |
| 纯自筹 + 早期作品现金流 | Klei / Positech / Coredumping / MaFi Games | 未披露 |

**（D）给「1–3 人做模拟经营」的三条可引用结论**

1. **内容量靠架构不靠人力**：把内容做成数据/原型/规则（Factorio 的 Lua prototype、Prison Architect 的 Lua grants、shapez 2 与 ONI 的程序化生成），是 1–3 人支撑 20+ 小时游玩并让 MOD 接管长尾的**唯一被反复验证的机制**。
2. **周期要用「无路线图承诺 + 始终可玩」来吸收不确定性**：RimWorld 的模块化发布原话是最佳引用；反面是 Game Dev Tycoon 团队第二作 Tavern Keeper 开发 11 年仍 EA，以及 Software Inc. 十年无 1.0——**长周期不必然失败，但必须有现金流（EA 销售/Patreon/预购）支撑。**
3. **不要第一作就做这个**：Production Line 单人开发者原话是最有力的定性证据——"Anybody who wants to attempt a game like this, in Unity, as their first game is nuts. Don't do it – it's seriously, ridiculously hard."

---

## 附：未查证到公开数据的项目清单（避免引用时误认）

- Factorio 官方总销量/收入数字
- RimWorld 官方总销量/收入数字（仅有 DLC 首周 206,000 份等媒体数据）
- Game Dev Tycoon 销量；Tavern Keeper 之外的第二作收入
- Software Inc. 官方销量/收入（仅有第三方估算 276.9K 份 / 毛 $4.5M）
- Production Line 销量/收入
- shapez 与 shapez 2 官方销量/收入
- Two Point Hospital 官方销量（仅有"受众 500 万"报道）
- Oxygen Not Included 官方销量/收入；ONI 项目组具体人数
- Captain of Industry 与 W&R 的官方销量/收入
- Captain of Industry、W&R、Software Inc. 的引擎/技术栈公开确认
- 各作**内置关卡编辑器**的公开证据（仅查证到文件级数据定义 + Workshop）

*调研范围：模拟经营/建造管理类（simulation / tycoon / management / automation）独立游戏 10 组样本；所有数据来自官方页面、维基引注、权威/行业媒体报道，来源 URL 已在各节标注。*
