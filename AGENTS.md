# Repository Guidelines

## 1. 架构布局与规范
- `guardrail/`：粒子输运物理静态分析与形式化核验引擎（AST 解析、守恒量断言与生命周期检测）。
- `adapters/`：多 Harness 桥接适配层（Antigravity 钩子、MCP 协议服务、DSH Cordis 插件与 Skill）。
- `.agents/`：项目私有插件与智能体扩展库（含 `mc-formal-verifier` 完整自包含插件包）。
- `benchmarks/`：三级分级评估基准集（Tier 1–3）与野外真实语料库（Wild Corpus）。
- `scripts/`：运维总控与统一命令行工具（`mc-verifier`）。

## 2. 领域物理规约 (Domain Physics Specification)
本项目涉及蒙特卡罗辐射输运（Geant4 / OpenMC）代码生成时，受严格的形式化物理守恒不变式约束。  
- 详细物理守恒不变式与输入规约契约请查阅领域白皮书：[PHYSICS_SPEC.md](PHYSICS_SPEC.md)。
- 该规约已由插件 `mc-formal-verifier` 自包含提供，在启用插件时自动由 Harness 加载，无需污染通用全局规则。

## 3. 总控与测试命令
- 形式化核验器总控状态：
  ```bash
  ./mc-verifier status
  ./mc-verifier on
  ./mc-verifier off
  ```
- 源码核验命令：
  ```bash
  python3 guardrail/check.py --code <source_file.cc>
  ```
