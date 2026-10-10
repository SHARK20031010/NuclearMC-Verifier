"""
Layer 3: Dual-Track Semantic Mapper
Combines offline-distilled deterministic rules (Fast Path) with LLM Schema fallback (Slow Path).
"""

import re
from typing import Optional, List, Dict, Any
from .schema import PhysicalSemanticAnnotation
from ..layer2_context.context_packager import LocalContextPackage


class SemanticMapper:
    """Maps LocalContextPackages to PhysicalSemanticAnnotations using dual-track architecture."""

    def __init__(self):
        pass

    def map_package(self, pkg: LocalContextPackage) -> Optional[PhysicalSemanticAnnotation]:
        """Fast-Path: Deterministic offline rule-based semantic mapping."""
        expr = pkg.target_expression
        expr_lower = expr.lower()

        # 1. Direction Sampling
        if any(k in expr_lower for k in ["setparticlemomentumdirection", "direction", "source.angle", "monodirectional", "stats.isotropic", "stats.polarazimuthal"]):
            is_iso = any(k in expr_lower for k in ["g4randomdirection", "costheta", "sintheta", "openmc.stats.isotropic", "stats.isotropic", "isotropic()"])
            is_fixed_vec = (
                bool(re.search(r"g4threevector\s*\(\s*0\s*,\s*0\s*,\s*1\s*\)", expr_lower))
                or "monodirectional" in expr_lower
                or bool(re.search(r"\[\s*0(?:\.0)?\s*,\s*0(?:\.0)?\s*,\s*1(?:\.0)?\s*\]", expr_lower))
            )
            
            # Phase space measure evaluation
            if pkg.source_type == "isotropic":
                is_collapsed = is_fixed_vec
            elif pkg.source_type == "beam":
                is_collapsed = False
            else:
                ctx_text = (pkg.enclosing_function_body + " " + " ".join(pkg.enclosing_guards) + " " + " ".join(pkg.file_context_hints)).lower()
                is_beam = any(k in ctx_text for k in ["beam", "pencil", "incident", "parallel", "collimat", "target", "slab", "shield", "maze", "corridor", "phantom", "sobp", "linac", "pbs", "waterphantom"])
                is_collapsed = is_fixed_vec and not is_beam

            return PhysicalSemanticAnnotation(
                action="Sample",
                physical_operation="sample_direction",
                physical_quantity="solid_angle",
                conservation_domain="phase_space",
                expected_invariant="COLLAPSED_TO_PENCIL_BEAM" if is_collapsed else ("collimated_beam" if is_fixed_vec else "4pi isotropic"),
                code_evidence=expr,
                confidence=1.0,
                observable="AngularDistribution",
                unit_dimension="dimensionless",
                lifecycle_tier="Initialization" if "primary" in pkg.enclosing_function_name.lower() or "gun" in expr_lower else "StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 2. Dynamic Weight Fetching
        if "getweight" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Fetch",
                physical_operation="fetch_track_weight",
                physical_quantity="track_weight",
                conservation_domain="variance_weight",
                expected_invariant="fair_game_weight_coupling",
                code_evidence=expr,
                confidence=1.0,
                observable="StatisticalWeight",
                unit_dimension="dimensionless",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 3. Energy Deposit Fetching & Accumulating / Absorbed Dose
        if "gettotalenergydeposit" in expr_lower or "edep" in expr_lower or "dose" in expr_lower:
            action = "Accumulate" if pkg.target_type == "AssignExpr" else "Fetch"
            is_dose = "dose" in expr_lower
            return PhysicalSemanticAnnotation(
                action=action,
                physical_operation="accumulate_absorbed_dose" if is_dose and action == "Accumulate" else ("accumulate_energy_deposit" if action == "Accumulate" else "fetch_energy_deposit"),
                physical_quantity="absorbed_dose" if is_dose else "energy",
                conservation_domain="dimension_linearity" if is_dose else "energy_momentum",
                expected_invariant="dose_mass_normalization" if is_dose else "energy_conservation_and_weight_coupling",
                code_evidence=expr,
                confidence=1.0,
                observable="Dose" if is_dose else "EnergyDeposit",
                unit_dimension="[L]^2[T]^-2" if is_dose else "[M][L]^2[T]^-2",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 4. Secondary Yield & Vertex Tally
        if "secondary" in expr_lower or "parentid" in expr_lower or "fyield" in expr_lower or "yield" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Accumulate",
                physical_operation="accumulate_secondary_yield",
                physical_quantity="particle_count",
                conservation_domain="quantum_numbers",
                expected_invariant="first_step_creation_filter" if pkg.has_first_step_guard else "UNFILTERED_STEPPING_INFLATION",
                code_evidence=expr,
                confidence=1.0,
                observable="SecondaryYield",
                unit_dimension="dimensionless",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 5. Transmission / Fluence Scoring
        if any(k in expr_lower for k in ["flux", "fluence", "count", "trans", "pass", "escap", "tally", "yield", "hit"]):
            has_w = pkg.has_weight_multiplication or "weightwindows" in expr_lower or "weight_windows" in expr_lower
            return PhysicalSemanticAnnotation(
                action="Accumulate",
                physical_operation="accumulate_fluence",
                physical_quantity="particle_count_or_weight",
                conservation_domain="variance_weight",
                expected_invariant="weighted_fluence_accumulation" if has_w else "UNWEIGHTED_COUNT_ACCUMULATION",
                code_evidence=expr,
                confidence=1.0,
                observable="Fluence",
                unit_dimension="[L]^-2",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 6. Global vs Local vs Proper Time Clock Fetching
        if "getglobaltime" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Fetch",
                physical_operation="fetch_global_clock",
                physical_quantity="time",
                conservation_domain="causality_clock",
                expected_invariant="absolute_laboratory_time",
                code_evidence=expr,
                confidence=1.0,
                observable="FlightTime",
                unit_dimension="[T]",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )
        elif "getlocaltime" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Fetch",
                physical_operation="fetch_local_decay_clock",
                physical_quantity="proper_time",
                conservation_domain="causality_clock",
                expected_invariant="particle_proper_lifetime_reset_on_step",
                code_evidence=expr,
                confidence=1.0,
                observable="LocalDecayLifetime",
                unit_dimension="[T]",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )
        elif "getpropertime" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Fetch",
                physical_operation="fetch_rest_frame_proper_time",
                physical_quantity="rest_frame_time",
                conservation_domain="causality_clock",
                expected_invariant="lorentz_invariant_proper_lifetime",
                code_evidence=expr,
                confidence=1.0,
                observable="ProperTime",
                unit_dimension="[T]",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 7. Step Boundary & Topology Status
        if "getstepstatus" in expr_lower or "fgeomboundary" in expr_lower or "fworldboundary" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Fetch",
                physical_operation="inspect_boundary_status",
                physical_quantity="topological_boundary_flag",
                conservation_domain="geometry_topology",
                expected_invariant="post_step_boundary_surface_evaluation",
                code_evidence=expr,
                confidence=1.0,
                observable="BoundaryCrossing",
                unit_dimension="dimensionless",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 8. Physical Process & Creator Process Verification
        if "getcreatorprocess" in expr_lower or "getprocessdefinedstep" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Fetch",
                physical_operation="fetch_reaction_process",
                physical_quantity="physical_process_type",
                conservation_domain="quantum_numbers",
                expected_invariant="process_specific_reaction_filtering",
                code_evidence=expr,
                confidence=1.0,
                observable="ReactionChannel",
                unit_dimension="dimensionless",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 9. Particle Species Definition & Ion Table Lookup
        if "setparticledefinition" in expr_lower or "getion" in expr_lower or "findparticle" in expr_lower:
            is_ion_table = "getion" in expr_lower or "iontable" in expr_lower
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="define_particle_species",
                physical_quantity="particle_species",
                conservation_domain="quantum_numbers",
                expected_invariant="ion_table_dynamic_creation" if is_ion_table else "valid_particle_lookup",
                code_evidence=expr,
                confidence=1.0,
                observable="ParticleType",
                unit_dimension="dimensionless",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 10. Source Energy Sampling & Definition
        if "setparticleenergy" in expr_lower or "setmonoenergy" in expr_lower or "setparticlemomentum" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Sample" if "rand" in expr_lower or "gauss" in expr_lower else "Define",
                physical_operation="assign_source_energy",
                physical_quantity="kinetic_energy",
                conservation_domain="energy_momentum",
                expected_invariant="positive_definite_kinetic_energy",
                code_evidence=expr,
                confidence=1.0,
                observable="PrimaryEnergy",
                unit_dimension="[M][L]^2[T]^-2",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 11. Source Spatial Vertex & Position Assignment
        if any(k in expr_lower for k in ["setparticleposition", "setcentrecoords", "generateprimaryvertex", "source.space", "stats.point"]):
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="assign_source_position",
                physical_quantity="spatial_coordinates",
                conservation_domain="phase_space",
                expected_invariant="source_spatial_containment",
                code_evidence=expr,
                confidence=1.0,
                observable="PrimaryPosition",
                unit_dimension="[L]",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 12. CSG Solid Geometry Definition (Half-Length Constraint)
        if "g4box" in expr_lower or "g4tubs" in expr_lower or "g4sphere" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="define_solid_geometry",
                physical_quantity="volume_measure",
                conservation_domain="geometry_topology",
                expected_invariant="half_length_measure_consistency",
                code_evidence=expr,
                confidence=1.0,
                observable="DetectorGeometry",
                unit_dimension="[L]^3",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 13. Physical Placement & Overlap Invariant
        if "g4pvplacement" in expr_lower or "checkoverlaps" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="place_physical_volume",
                physical_quantity="spatial_placement",
                conservation_domain="geometry_topology",
                expected_invariant="containment_and_no_overlap",
                code_evidence=expr,
                confidence=1.0,
                observable="GeometryPlacement",
                unit_dimension="[L]",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 14. Medium Mass Fetching (Dose Denominator)
        if "getmass" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Fetch",
                physical_operation="fetch_volume_mass",
                physical_quantity="mass",
                conservation_domain="dimension_linearity",
                expected_invariant="positive_target_mass_coupling",
                code_evidence=expr,
                confidence=1.0,
                observable="TargetMass",
                unit_dimension="[M]",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 15. Material & Elemental Mass Fraction Composition
        if any(k in expr_lower for k in ["addelement", "addmaterial", "findorbuildmaterial", "g4material", "openmc.material", "set_density"]):
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="define_material_composition",
                physical_quantity="material_density",
                conservation_domain="quantum_numbers",
                expected_invariant="mass_fraction_unity_sum",
                code_evidence=expr,
                confidence=1.0,
                observable="MaterialDensity",
                unit_dimension="[M][L]^-3",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 16. Physics Constructor Registration & Decay/EM/Hadron Package Binding
        if "registerphysics" in expr_lower or "physics" in expr_lower or "physicsconstructor" in expr_lower:
            is_radioactive = "radioactive" in expr_lower
            is_hp_neutron = "hp" in expr_lower or "bic_hp" in expr_lower
            is_em_opt4 = "option4" in expr_lower or "livermore" in expr_lower
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="register_physics_constructor",
                physical_quantity="reaction_model_family",
                conservation_domain="quantum_numbers",
                expected_invariant="radioactive_decay_active" if is_radioactive else ("neutron_high_precision_eval" if is_hp_neutron else "appropriate_physics_package_binding"),
                code_evidence=expr,
                confidence=1.0,
                observable="PhysicsPackage",
                unit_dimension="dimensionless",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 17. Production Range Cut Definition (Micro-Geometry Invariant)
        if "setdefaultcutvalue" in expr_lower or "setproductioncut" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="set_production_range_cut",
                physical_quantity="range_threshold",
                conservation_domain="energy_momentum",
                expected_invariant="range_cut_smaller_than_detector_scale",
                code_evidence=expr,
                confidence=1.0,
                observable="ProductionCut",
                unit_dimension="[L]",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 18. User Stepping & Tracking Limits
        if "setuserlimits" in expr_lower or "setmaxallowedstep" in expr_lower or "setuserminekine" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="set_user_transport_limits",
                physical_quantity="step_length_limit",
                conservation_domain="geometry_topology",
                expected_invariant="step_limiter_process_active",
                code_evidence=expr,
                confidence=1.0,
                observable="UserStepLimit",
                unit_dimension="[L]",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 19. Analysis Manager Histogram / Ntuple Filling
        if "fillh1" in expr_lower or "fillntuple" in expr_lower or "addntuplerow" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Accumulate",
                physical_operation="fill_analysis_histogram",
                physical_quantity="statistical_frequency",
                conservation_domain="variance_weight",
                expected_invariant="weighted_statistical_scoring",
                code_evidence=expr,
                confidence=1.0,
                observable="HistogramSpectrum",
                unit_dimension="dimensionless",
                lifecycle_tier="EventLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 20. Run Normalization & Per-Primary Scaling
        if "getnumberofevent" in expr_lower or "norm" in expr_lower or "per_primary" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Convert",
                physical_operation="normalize_per_primary",
                physical_quantity="normalization_scale",
                conservation_domain="dimension_linearity",
                expected_invariant="per_primaries_or_becquerel_normalization",
                code_evidence=expr,
                confidence=1.0,
                observable="NormalizedObservable",
                unit_dimension="dimensionless",
                lifecycle_tier="RunLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 21. Sensitive Detector Hit Processing
        if "processhits" in expr_lower or "hitscollection" in expr_lower:
            return PhysicalSemanticAnnotation(
                action="Accumulate",
                physical_operation="accumulate_detector_hit",
                physical_quantity="energy_deposit",
                conservation_domain="energy_momentum",
                expected_invariant="thread_safe_detector_accumulation",
                code_evidence=expr,
                confidence=1.0,
                observable="DetectorHit",
                unit_dimension="[M][L]^2[T]^-2",
                lifecycle_tier="StepLevel",
                guard_conditions=pkg.enclosing_guards
            )

        # 22. Stacking Action & Russian Roulette Track Classification
        if ("classifynewtrack" in expr_lower or "fkill" in expr_lower or "fwaiting" in expr_lower
            or "classifynewtrack" in pkg.enclosing_function_name.lower()):
            evidence = expr
            if "fkill" in pkg.enclosing_function_body.lower():
                evidence = pkg.enclosing_function_body[:300]
            return PhysicalSemanticAnnotation(
                action="Sample",
                physical_operation="classify_track_stack",
                physical_quantity="particle_count",
                conservation_domain="quantum_numbers",
                expected_invariant="stack_classification_weight_conservation",
                code_evidence=evidence,
                confidence=1.0,
                observable="TrackClassification",
                unit_dimension="dimensionless",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 23. Run Driver Lifecycle & Initialization Protocol
        if any(k in expr_lower for k in ["createrunmanager", "beamon", "initialize", "setuserinitialization", "openmc.run"]):
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="configure_run_manager",
                physical_quantity="execution_lifecycle",
                conservation_domain="causality_clock",
                expected_invariant="initialization_precedes_beamon",
                code_evidence=expr,
                confidence=1.0,
                observable="RunLifecycle",
                unit_dimension="dimensionless",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        # 24. Action Initialization Master/Worker Topology Isolation
        is_master = "buildformaster" in expr_lower or "buildformaster" in pkg.enclosing_function_name.lower()
        if (is_master
            or ("build" in expr_lower and "actioninit" in pkg.enclosing_function_name.lower())
            or ("setuseraction" in expr_lower and ("build" in pkg.enclosing_function_name.lower() or "actioninit" in pkg.enclosing_function_name.lower()))):
            return PhysicalSemanticAnnotation(
                action="Define",
                physical_operation="action_initialization_hook",
                physical_quantity="thread_role",
                conservation_domain="geometry_topology",
                expected_invariant="master_thread_action_isolation" if is_master else "master_worker_action_isolation",
                code_evidence=expr,
                confidence=1.0,
                observable="ActionTopology",
                unit_dimension="dimensionless",
                lifecycle_tier="Initialization",
                guard_conditions=pkg.enclosing_guards
            )

        return None

    def build_llm_prompt(self, pkg: LocalContextPackage) -> Dict[str, Any]:
        """Prepares a structured LLM prompt with Depth-1 context and strict JSON schema."""
        prompt = f"""You are a computational physics formal semantics parser.
Analyze the following C++ Monte Carlo radiation transport code element and produce a structured PhysicalSemanticAnnotation.

Target Expression:
`{pkg.target_expression}`

Enclosing Function:
`{pkg.enclosing_function_name}`

Surrounding Guards / Conditions:
{pkg.enclosing_guards}

Enclosing Function Snippet:
```cpp
{pkg.enclosing_function_body}
```

Output ONLY a JSON object conforming to this schema:
{{
  "action": "Define" | "Sample" | "Fetch" | "Accumulate" | "Convert",
  "physical_operation": "short_snake_case_name",
  "physical_quantity": "solid_angle" | "energy" | "track_weight" | "time" | "particle_count",
  "conservation_domain": "energy_momentum" | "phase_space" | "variance_weight" | "causality_clock" | "quantum_numbers" | "dimension_linearity",
  "expected_invariant": "4pi isotropic" | "fair_game_weight_coupling" | "first_step_creation_filter" | etc.,
  "code_evidence": "{pkg.target_expression}",
  "confidence": float (0.0 to 1.0)
}}
"""
        return {"system_prompt": "You are a computational nuclear physics formal analyzer.", "user_prompt": prompt}
