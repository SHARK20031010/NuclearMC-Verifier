# NuclearMC-Verifier Quick-Start Examples

This directory provides self-contained, instant-run Geant4 C++ test codes illustrating classic silent physical bugs that can bypass traditional compilers and linters, but are caught deterministically by **NuclearMC-Verifier**.

---

## 🚀 Instant Verification Commands

From the repository root, run:

```bash
# Example 01: Boundary Status on PreStepPoint (PST-INV-LIFECYCLE-01)
./mc-verifier check examples/01_boundary_leak.cc

# Example 02: Fair-Game Weight Lost in Variance Reduction (PST-INV-VARIANCE-01)
./mc-verifier check examples/02_unweighted_variance_reduction.cc

# Example 03: Missing Non-linear Birks Quenching in Scintillator (PHYS-INV-BIRKS-01)
./mc-verifier check examples/03_scintillator_without_birks.cc

# Batch Verification of all examples at once:
./mc-verifier check examples/01_boundary_leak.cc examples/02_unweighted_variance_reduction.cc examples/03_scintillator_without_birks.cc
```

---

## 📋 Example Catalog

| File | Physics Target | Silent Defect | Verification Invariant |
| :--- | :--- | :--- | :--- |
| [`01_boundary_leak.cc`](./01_boundary_leak.cc) | Mixed neutron/gamma leakage through BPE+Pb shielding | Querying `fGeomBoundary` on `PreStepPoint` (boundary state only exists on `PostStepPoint`). Leakage tally remains silently zero. | `PST-INV-LIFECYCLE-01` (Lifecycle & Boundary Support) |
| [`02_unweighted_variance_reduction.cc`](./02_unweighted_variance_reduction.cc) | 14 MeV neutron penetration through 50 cm Fe shield | Scoring `gFlux += 1.0` without multiplying by track weight `step->GetTrack()->GetWeight()`. Violates transport unbiasedness. | `PST-INV-VARIANCE-01` (Unbiased Variance Reduction) |
| [`03_scintillator_without_birks.cc`](./03_scintillator_without_birks.cc) | Organic scintillator response under gamma/neutron irradiation | Scintillation yield configured without Birks quenching (`SetBirksConstant`). High-LET proton recoil light yield overestimated 2x~3x. | `PHYS-INV-BIRKS-01` (Scintillator Quenching Contract) |

---

## 💡 Pre-commit Integration

To enable automated verification on every `git commit`, add this repository to your project's `.pre-commit-config.yaml`:

```yaml
repos:
  - repo: https://github.com/SHARK20031010/NuclearMC-Verifier
    rev: v0.1.0  # Or latest release / commit hash
    hooks:
      - id: nuclearmc-verifier
```
