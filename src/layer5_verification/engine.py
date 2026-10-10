"""
Layer 5: Verification Engine on Physics Semantic Tree
Orchestrates invariant assertors and produces structured verification verdicts.
"""

from typing import List, Dict, Any
from pydantic import BaseModel, Field
from .rules_conservation import ConservationRules, PSTViolation
from ..layer4_pst.tree import PhysicsSemanticTree


class VerificationVerdict(BaseModel):
    is_valid: bool
    violations: List[PSTViolation] = Field(default_factory=list)
    node_count: int
    edge_count: int
    summary: str


class PSTVerificationEngine:
    """Executes conservation rules over a PhysicsSemanticTree instance."""

    def __init__(self):
        self.rules = ConservationRules()

    def verify(self, pst: PhysicsSemanticTree, is_variance_reduced: bool = True) -> VerificationVerdict:
        violations: List[PSTViolation] = []

        violations.extend(self.rules.check_weight_flux(pst, is_variance_reduced=is_variance_reduced))
        violations.extend(self.rules.check_phase_space_measure(pst))
        violations.extend(self.rules.check_secondary_stepping_filter(pst))
        violations.extend(self.rules.check_causality_clock(pst))
        violations.extend(self.rules.check_stack_classification_weight(pst))
        violations.extend(self.rules.check_master_worker_action_isolation(pst))
        violations.extend(self.rules.check_run_lifecycle_initialization(pst))
        violations.extend(self.rules.check_dose_mass_normalization(pst))
        violations.extend(self.rules.check_hadron_elastic_coupling(pst))
        violations.extend(self.rules.check_boundary_step_point_topology(pst))
        violations.extend(self.rules.check_scintillation_birks_quenching(pst))
        violations.extend(self.rules.check_event_container_lifecycle_reset(pst))
        violations.extend(self.rules.check_photonuclear_physics_completeness(pst))
        violations.extend(self.rules.check_optical_surface_and_refraction(pst))
        violations.extend(self.rules.check_magnetic_field_chord_finder(pst))
        violations.extend(self.rules.check_gaussian_energy_nonnegative_truncation(pst))
        violations.extend(self.rules.check_radial_source_measure_sampling(pst))
        violations.extend(self.rules.check_energy_resolution_fwhm_scaling(pst))
        violations.extend(self.rules.check_particle_physics_constructor_coupling(pst))
        violations.extend(self.rules.check_shield_material_authenticity(pst))
        violations.extend(self.rules.check_secondary_stepping_multiplicity(pst))
        violations.extend(self.rules.check_bateman_decay_chain_branching(pst))
        violations.extend(self.rules.check_cosmic_ray_zenith_angular_distribution(pst))
        violations.extend(self.rules.check_semiconductor_fano_fluctuation(pst))
        violations.extend(self.rules.check_detector_deadtime_pulse_pileup(pst))
        violations.extend(self.rules.check_water_radical_chemistry_completeness(pst))
        violations.extend(self.rules.check_thermal_neutron_sab_binding(pst))

        is_valid = len(violations) == 0
        summary = (
            f"PST Verification PASSED: All {len(pst.nodes_dict)} nodes and {len(pst.edges_list)} conservation flows satisfied."
            if is_valid else
            f"PST Verification FAILED: Detected {len(violations)} physical conservation invariant violations."
        )

        return VerificationVerdict(
            is_valid=is_valid,
            violations=violations,
            node_count=len(pst.nodes_dict),
            edge_count=len(pst.edges_list),
            summary=summary
        )
