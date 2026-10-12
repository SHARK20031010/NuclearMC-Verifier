# NuclearMC-Verifier Benchmark Suite

This directory contains the verification benchmarks and expert ground-truth annotations for **NuclearMC-Verifier**.

---

## 1. Overview & Benchmark Hierarchy

To rigorously evaluate the formal verification of Monte Carlo radiation transport simulation codes, NuclearMC-Verifier utilizes a three-tier hierarchical benchmark alongside a real-world wild corpus:

| Hierarchy / Subset | Description | Scope & Challenge | Reference Suite |
| :--- | :--- | :--- | :--- |
| **Tier 1: Single-Slot Invariants** | 60 single-slot boundary violations | Tests atomic conservation across PhaseSpace, Causality, Variance, and Lifecycle | Representative cases in `cases/tier1_single_slot/` |
| **Tier 2: Multi-Slot Coupling** | 60 coupled violations | Cross-slot dependencies, e.g., dead layer combined with pulse rate | Zenodo Archive |
| **Tier 3: Deep Penetration** | 60 complex transport problems | High-attenuation deep penetration, weight window meshes, optical transport | Zenodo Archive |
| **Wild Corpus** | 10 real-world production codes | Complex Geant4/OpenMC applications with multi-physics interactions | Representative cases in `cases/wild_corpus/` |

---

## 2. Full 190-Task Replication Package (Zenodo)

The full 190-task experimental dataset (including full prompts, task specifications, baseline LLM generations Arm A / B / C, and empirical results) is archived on **Zenodo**:

- **Zenodo DOI**: [10.5281/zenodo.23231733](https://doi.org/10.5281/zenodo.23231733)
- **Title**: *NuclearMC-Verifier Benchmark Suite and Replication Package*

---

## 3. Directory Layout

```
benchmarks/
├── README.md                      # This benchmark overview and guide
├── evaluate_pst.py                # PST automated evaluation comparator (Node & Edge F1)
├── sample_annotations/            # Expert-annotated Golden-PST ground-truth graphs (.json)
└── cases/                         # Representative C++ source test cases
    ├── tier1_single_slot/         # Representative Tier 1 cases (T1-4, T1-6, T2-5, T4-1..T4-8)
    └── wild_corpus/               # Representative real-world cases (WILD-01..WILD-10)
```

---

## 4. Evaluation Commands

### Run Full PST Formal Verification Benchmark (8 Representative Tasks, 16 Sources)
```bash
./mc-verifier eval-pst
```

### Evaluate a Generated PST Against Golden Ground-Truth Graph
```bash
python3 cli.py eval benchmarks/cases/wild_corpus/WILD-01/code/case_buggy.cc benchmarks/sample_annotations/wild_01.json
```
