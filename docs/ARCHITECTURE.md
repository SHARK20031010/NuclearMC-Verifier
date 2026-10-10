# Five-Layer Pipeline Architecture: From AST to Physics Verification

The **Physics Semantic Tree (PST) Engine** transforms raw C++ simulation source code into an actionable physical verification graph through five distinct processing layers:

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

## Layer 1: AST Parsing & API Extraction
- **Objective**: Parse C++ syntax without requiring full Geant4 build headers.
- **Implementation**: Extracts statement-level syntax trees using lightweight AST and token visitors:
  - Function declarations and member bodies (`GeneratePrimaries`, `UserSteppingAction`, `EndOfEventAction`).
  - Method calls (`step->GetTotalEnergyDeposit()`, `gun->SetParticleMomentumDirection(...)`).
  - Assignments and accumulator updates (`fEdep += ...`, `fCount++`).

---

## Layer 2: Local Context Packaging (Depth-1 Expansion)
- **Objective**: Mitigate **Pitfall 1 (Context Insensitivity)**.
- **Problem**: Examining an isolated API call like `GetStepLength()` is insufficient because its meaning changes under boundary crossing vs. continuous stepping.
- **Implementation**: Builds an enriched context packet for each candidate node:
  - The exact callsite AST expression and line span.
  - The enclosing condition block (e.g., `if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary)`).
  - Depth-1 callee definition or signature.
  - Referenced variables, local declarations, and typedefs.

---

## Layer 3: Semantic Mapping (Dual-Track Architecture)
- **Objective**: Mitigate **Pitfall 2 (LLM Non-Determinism & Latency)**.
- **Dual-Track Mechanism**:
  1. **Deterministic Fast Path (<10ms)**:
     - 100+ standard Geant4/OpenMC API patterns are mapped to physical operations offline.
     - 100% reproducible, zero token cost, sub-millisecond execution.
  2. **LLM Schema Slow Path (Fallback)**:
     - Invoked only when user code introduces unknown custom classes or wrapper functions.
     - Enforces strict Pydantic JSON Schema output (`physical_operation`, `physical_quantity`, `conservation_domain`, `expected_invariant`, `code_evidence`, `confidence`).

---

## Layer 4: Physics Semantic Tree Assembly
- **Objective**: Synthesize isolated physical operations into a unified Directed Acyclic Graph (DAG).
- **Graph Assembly**:
  - **Nodes**: Physical operation instances ($V_{phys}$).
  - **Edges**:
    - `DATA_DEPENDENCY`: Variable def-use chains.
    - `WEIGHT_COUPLING`: Track weight injection into tally accumulators.
    - `LIFECYCLE_CONTAINER`: Hierarchical step $\to$ event $\to$ run relationships.
    - `CAUSALITY_FLOW`: Temporal clock propagation.

---

## Layer 5: Conservation Rule Verification
- **Objective**: Evaluate 6 orthogonal conservation invariants over the PST.
- **Mechanics**: Instead of heuristic regexes, verification runs as **graph reachability and conservation flow analysis**:
  - Example: `HasPath(SplitNode, TallyNode)` $\land$ `¬InFlow(WeightNode, TallyNode)` $\implies$ Flag `CRITICAL: UNWEIGHTED_SPLIT_TALLY`.
  - Generates machine-verifiable proofs and human-readable physical remediation prescriptions.
