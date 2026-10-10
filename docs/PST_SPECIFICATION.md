# Physics Semantic Tree (PST) Specification: A Domain-Specific IR for Radiation Transport

## 1. Mathematical Formalism

Let a Monte Carlo radiation transport simulation program be defined as an execution model over phase space $\Gamma = \mathbb{R}^3 \times S^2 \times \mathbb{R}^+ \times \mathbb{R}^+$.
A **Physics Semantic Tree (PST)** is a directed, typed, attributed graph:

$$\mathcal{G}_{PST} = (\mathcal{V}_{phys}, \mathcal{E}_{flow}, \Phi)$$

where:
- $\mathcal{V}_{phys}$ is the set of **Physical Operation Nodes**, representing domain-level transport actions.
- $\mathcal{E}_{flow}$ is the set of **Conservation and Dataflow Edges**, representing physical dependencies, causal ordering, and weight flows.
- $\Phi: \mathcal{V} \cup \mathcal{E} \to \mathcal{A}$ is a valuation function mapping nodes and edges to attribute records (physical quantity, dimension, scope, confidence).

---

## 2. Node Schema: The "Five Actions $\times$ Thirty-One Slots" Basis

Every node $v \in \mathcal{V}_{phys}$ belongs to one of the five canonical Monte Carlo transport actions:

| Action Category | Formal Semantic | Typical Geant4 / MC Construct |
| :--- | :--- | :--- |
| **`Define`** | Establishes geometric entities, materials, source spectra, or tally containers. | `G4Box`, `G4NistManager::FindOrBuildMaterial`, `G4ParticleGun` |
| **`Sample`** | Stochastically draws a point in phase space (energy, position, direction, or time). | `G4RandomDirection()`, `cosTheta = 2*G4UniformRand()-1`, `G4SPSEneDistribution` |
| **`Fetch`** | Accesses the instantaneous dynamic state of a particle or step. | `step->GetTotalEnergyDeposit()`, `track->GetWeight()`, `preStep->GetGlobalTime()` |
| **`Accumulate`** | Aggregates physical quantities into statistical accumulators across steps or events. | `fEdep += dE`, `fFlux += w`, `fYield++`, `analysisManager->FillH1(...)` |
| **`Convert`** | Performs dimensional scaling, fluence-to-dose conversion, or normalization. | `dose = (edep * joule) / mass`, `rate = count / time_window` |

### Typed Attributes of $v \in \mathcal{V}_{phys}$
```json
{
  "node_id": "op_014",
  "action": "Accumulate",
  "observable": "AbsorbedDose",
  "carrier_quantity": "TotalEnergyDeposit",
  "unit_dimension": "[M][L]^2[T]^-2",
  "domain_scope": "TumorTargetVolume",
  "lifecycle_tier": "StepLevel",
  "ast_span": {
    "file": "MySteppingAction.cc",
    "line_start": 42,
    "line_end": 42,
    "expression": "fEdep += step->GetTotalEnergyDeposit() * step->GetTrack()->GetWeight();"
  },
  "guard_conditions": [
    "volume == fTargetLogical",
    "track->GetCurrentStepNumber() == 1"
  ],
  "confidence": 1.0
}
```

---

## 3. Edge Schema: The Four Conservation Flow Types

Edges $e = (u, v) \in \mathcal{E}_{flow}$ encode the physical relationship and propagation laws between operations:

### 3.1 `DATA_DEPENDENCY` ($u \xrightarrow{data} v$)
Values produced by operation $u$ are directly consumed by operation $v$ (e.g., `Fetch(dE)` $\to$ `Accumulate(fEdep)`).

### 3.2 `WEIGHT_COUPLING` ($u \xrightarrow{weight} v$)
Under variance reduction (splitting, Russian roulette, weight windows), track weight $w$ must be coupled into tally accumulator $v$.
- **Invariant Requirement**: For any $v \in \mathcal{V}_{Accumulate}$ inside a variance-reduced tracking loop, there MUST exist a node $u \in \mathcal{V}_{Fetch}$ where $u.carrier\_quantity = \text{TrackWeight}$, such that $(u, v) \in \mathcal{E}_{flow}^{weight}$.
- **Violation Flag**: `DEFECT_WEIGHT_FLUX_DISCONNECTED` (Tally inflation by $10^2 \sim 10^7\times$).

### 3.3 `LIFECYCLE_CONTAINER` ($u \xrightarrow{container} v$)
Relates step-level micro-accumulations to event-level closures and run-level reductions.
- **Invariant Requirement**: Step-level variables must be encapsulated and cleared upon `EndOfEventAction`. Unreset variables across events trigger `DEFECT_STALE_TALLY_ACCUMULATION`.

### 3.4 `CAUSALITY_FLOW` ($u \xrightarrow{causal} v$)
Enforces monotonic time evolution and proper clock references (Laboratory Global Time vs. Proper Local Lifetime).
- **Invariant Requirement**: Coincidence tallies and time-of-flight (TOF) filters must draw from `GetGlobalTime()`, not `GetLocalTime()`.

---

## 4. Graph Assertions for Physical Conservation

| Invariant ID | Domain | Assertion Predicate on $\mathcal{G}_{PST}$ |
| :--- | :--- | :--- |
| **`INV-PST-01`** | **Weight Conservation** | $\forall v \in \mathcal{V}_{Acc}, \text{IsBiased}(v) \implies \exists u \in \mathcal{V}_{Fetch}(Weight): (u, v) \in \mathcal{E}_{weight}$ |
| **`INV-PST-02`** | **Measure Isotropy** | $\forall v \in \mathcal{V}_{Sample}(Direction): \text{IsPointSource}(v) \implies v.measure\_type = \text{"SphericalSolidAngle"}$ |
| **`INV-PST-03`** | **First-Step Vertex** | $\forall v \in \mathcal{V}_{Acc}(SecondaryProduction): \text{"step == 1"} \in v.guard\_conditions$ |
| **`INV-PST-04`** | **Causal Clocking** | $\forall v \in \mathcal{V}_{Fetch}(Time): \text{IsCoincidenceContext}(v) \implies v.clock\_source = \text{"GlobalTime"}$ |
| **`INV-PST-05`** | **Mass Normalization** | $\forall v \in \mathcal{V}_{Conv}(Dose): \exists u \in \mathcal{V}_{Fetch}(Mass): (u, v) \in \mathcal{E}_{data}$ |
