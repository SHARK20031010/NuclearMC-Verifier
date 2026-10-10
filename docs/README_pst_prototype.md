<div align="center">

# ⚛️ Physics Semantic Tree (PST) Engine

### A Domain-Specific Intermediate Representation (IR) for Formal Verification of Monte Carlo Radiation Transport Codes

**[English](README.md) | [中文版](README_zh.md)**

<p align="center">
  <img src="https://img.shields.io/badge/Language-Python_3.9+-3776ab.svg" alt="Python">
  <img src="https://img.shields.io/badge/Graph_Backend-NetworkX-orange.svg" alt="NetworkX">
  <img src="https://img.shields.io/badge/Schema-Pydantic_v2-e92063.svg" alt="Pydantic">
  <img src="https://img.shields.io/badge/Latency-%3C_10ms-brightgreen.svg" alt="Latency">
  <img src="https://img.shields.io/badge/Target-Geant4_%7C_OpenMC_%7C_MCNP-blue.svg" alt="Target">
</p>

</div>

---

## 🎯 Research Motivation: Bridging the "Physics Semantic Gap"

Large Language Models (LLMs) synthesize syntactically valid C++ simulation code that compiles without warnings and runs with exit code 0. However, in continuous phase-space radiation transport, they frequently commit **Silent Physical Failures** (producing numerical deviations of $10^2 \sim 10^7\times$).

Existing code representation models (LLVM IR, Tree-Sitter CST, Code Property Graphs) capture only **software engineering semantics** (call graphs, type hierarchies, control flow). They cannot represent:
- Phase-space measure conservation ($d\Omega = \sin\theta d\theta d\phi$)
- Fair-game particle weight flux coupling ($\sum x_i \cdot w_i$)
- Relativistic laboratory clocking vs. local decay lifetime

We introduce the **Physics Semantic Tree (PST)**: the first domain-specific intermediate representation that maps raw syntax AST tokens into typed physical transport operators and conservation flow graphs.

```text
                  C++ Source Code
                         │
        [Layer 1: AST & API Extraction]
          CallExpr, DeclStmt, AssignExpr
                         │
        [Layer 2: Local Context Packager]
          Depth-1 Enclosing Body, Guards, Types
                         │
             ┌───────────┴───────────┐
             ▼                       ▼
    [Fast-Path: Pattern]    [Slow-Path: LLM Schema]
   (Known 100+ Geant4 APIs) (Custom User Wrappers)
             └───────────┬───────────┘
                         │
        [Layer 3: Typed Semantic Operations]
          PhysicalOperation JSON Records
                         │
        [Layer 4: PST Graph Assembly]
          G_PST = (V_phys, E_flow) via DFG
                         │
        [Layer 5: Conservation Assertions]
          Graph Reachability & Invariant Analysis
                         │
              Verified PST & Diagnostics
```

---

## 🏗️ The Five-Layer Pipeline

### Layer 1: AST Parsing & API Extraction
- Extracts C++ function definitions, call expressions (`CallExpr`), declarations, and assignment statements (`AssignExpr`).
- Identifies simulation-specific API candidates (`GetTotalEnergyDeposit`, `GetWeight`, `G4RandomDirection`, etc.).

### Layer 2: Local Context Packaging (Depth-1 Expansion)
- **Mitigates Pitfall 1 (Context Insensitivity)**: An isolated API call like `GetStepLength()` is ambiguous without context.
- Packages enclosing conditions (boundary checks, first-step creation guards), local variable types, and depth-1 caller/callee bodies.

### Layer 3: Dual-Track Semantic Mapping
- **Mitigates Pitfall 2 (LLM Non-Determinism & Latency)**:
  - **Deterministic Fast-Path (<10ms)**: Evaluates 100+ standard Geant4/OpenMC APIs against offline-distilled patterns. Zero token cost, 100% reproducible.
  - **LLM Schema Slow-Path**: Used only as a fallback for user-defined custom classes with strict Pydantic JSON Schema decoding.

### Layer 4: Physics Semantic Tree (PST) Assembly
- Constructs the directed typed graph $\mathcal{G}_{PST} = (\mathcal{V}_{phys}, \mathcal{E}_{flow})$:
  - **$\mathcal{V}_{phys}$**: Nodes classified under the *Five Actions* (`Define`, `Sample`, `Fetch`, `Accumulate`, `Convert`).
  - **$\mathcal{E}_{flow}$**: Directed edges for `DATA_DEPENDENCY`, `WEIGHT_COUPLING`, `LIFECYCLE_CONTAINER`, and `CAUSALITY_FLOW`.

### Layer 5: Conservation Rule Verification
- Evaluates first-principles physical conservation invariants as **graph reachability and conservation flow predicates**:
  - `PHYS-INV-WEIGHT-01`: Detects disconnected weight flux in variance-reduced tallies.
  - `PHYS-INV-MEASURE-01`: Detects collapsed pencil beams in isotropic point sources.
  - `PHYS-INV-LIFECYCLE-01`: Detects secondary vertex inflation lacking creation-step guards.
  - `PHYS-INV-CAUSAL-01`: Detects local proper time misuse in absolute flight clocks.

---

## 🧪 Ground-Truth Verification (Mitigating Pitfall 3)

To ensure the fidelity of constructed PST graphs, we establish the `Golden-PST-30` benchmark:
- 30 expert-annotated ground-truth graphs stored in `golden_benchmark/sample_annotations/`.
- Automated evaluation via `golden_benchmark/evaluate_pst.py` measuring **Node F1** and **Edge F1** scores:

```bash
# Evaluate a generated PST against ground-truth
python3 cli.py eval /path/to/case_buggy.cc golden_benchmark/sample_annotations/wild_01_secondary_vertex.json
```

---

## 🚀 Quick Start

### 1. Installation
```bash
pip install -r requirements.txt
```

### 2. CLI Usage
```bash
# 1. Run formal verification report
python3 cli.py check path/to/simulation.cc

# 2. Render PST as a Mermaid graph diagram
python3 cli.py mermaid path/to/simulation.cc

# 3. Dump the full PST IR as JSON
python3 cli.py dump-pst path/to/simulation.cc

# 4. Run test suite
python3 -m pytest tests/
```

---

## 📄 Academic Positioning (Target: CPC / Energy and AI)

> **Novel Intermediate Representation**: We present the **Physics Semantic Tree (PST)**, bridging the semantic gap between syntactic code ASTs and macroscopic physical conservation laws. Unlike existing semantic code graphs that capture only software engineering semantics, the PST encodes domain-specific continuous transport operators through a hybrid, deterministic AST pipeline.
