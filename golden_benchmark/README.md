# Golden-PST Benchmark: Ground-Truth Physics Semantic Trees

## 1. Overview & Protocol

To scientifically validate the accuracy of the **Physics Semantic Tree (PST)** extraction pipeline (resolving **Pitfall 3: PST Verifiability**), we establish an expert-curated ground truth dataset: `Golden-PST-30`.

The dataset contains 30 canonical Monte Carlo simulation tasks selected from the 190-task benchmark, spanning all 6 domain areas:
- **Shielding & Deep Penetration** (Geometric splitting, weight windows)
- **Dosimetry & Fluence** (Absorbed dose, sensitive volume normalization)
- **Radionuclide Activation** (Bateman chains, isomer branching)
- **Detector Spectroscopy** (Optical quenching, coincidence gating)
- **Microdosimetry & Nanotargets** (Chord length, boundary crossing)
- **Accelerator Sources** (Phase space measure, relativistic kinematics)

---

## 2. Quantitative Evaluation Metrics

Given a ground-truth graph $\mathcal{G}_{GT} = (\mathcal{V}_{GT}, \mathcal{E}_{GT})$ and a predicted graph $\mathcal{G}_{Pred} = (\mathcal{V}_{Pred}, \mathcal{E}_{Pred})$:

### 2.1 Node-Level Metrics ($V\text{-Precision}, V\text{-Recall}, V\text{-F}_1$)
A predicted node $v_{pred}$ matches a ground-truth node $v_{gt}$ if:
1. Action category matches: $v_{pred}.action == v_{gt}.action$
2. Physical operation matches: $v_{pred}.physical\_operation == v_{gt}.physical\_operation$
3. Expected invariant matches: $v_{pred}.expected\_invariant == v_{gt}.expected\_invariant$

$$P_V = \frac{|\mathcal{V}_{matched}|}{|\mathcal{V}_{Pred}|}, \quad R_V = \frac{|\mathcal{V}_{matched}|}{|\mathcal{V}_{GT}|}, \quad F_{1, V} = \frac{2 P_V R_V}{P_V + R_V}$$

### 2.2 Edge-Level Metrics ($E\text{-Precision}, E\text{-Recall}, E\text{-F}_1$)
A predicted edge $e_{pred} = (u, v)$ matches $e_{gt}$ if:
1. Edge type matches: $e_{pred}.edge\_type == e_{gt}.edge\_type$
2. Satisfaction status matches: $e_{pred}.is\_satisfied == e_{gt}.is\_satisfied$
3. Source and target node types match corresponding matched endpoints.

$$P_E = \frac{|\mathcal{E}_{matched}|}{|\mathcal{E}_{Pred}|}, \quad R_E = \frac{|\mathcal{E}_{matched}|}{|\mathcal{E}_{GT}|}, \quad F_{1, E} = \frac{2 P_E R_E}{P_E + R_E}$$
