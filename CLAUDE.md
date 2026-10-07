# Claude Code Guidelines for Monte Carlo Transport Simulation

This project enforces formal verification for Monte Carlo particle transport codes (Geant4 / OpenMC).
See [PHYSICS_SPEC.md](PHYSICS_SPEC.md) for full formal physics invariant contracts and pre-condition specifications.

## Key Development Rules
1. **Domain Physics Specifications**:
   - Refer to [PHYSICS_SPEC.md](PHYSICS_SPEC.md) before writing Geant4 / OpenMC simulations.
   - Strictly honor the 5 orthogonal conservation invariants (Energy/Momentum, Charge, Phase Space, Weight/Unbiasedness, Dimensionality).
2. **Formal Verification Check**:
   Before finalizing C++ Monte Carlo code, verify via:
   ```bash
   python3 guardrail/check.py --code <source_file.cc>
   ```
3. **Master Switch**:
   Check or toggle the verifier across all harnesses:
   ```bash
   ./mc-verifier status
   ./mc-verifier on
   ./mc-verifier off
   ```
