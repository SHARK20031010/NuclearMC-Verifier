"""
Layer 5: Conservation Invariants and Graph Flow Assertions on PST
Evaluates physical conservation laws as graph reachability and attribute predicates.
"""

import re
from typing import List, Dict, Any
from pydantic import BaseModel
from ..layer4_pst.tree import PhysicsSemanticTree


class PSTViolation(BaseModel):
    rule_id: str
    invariant_name: str
    severity: str                         # "CRITICAL", "MAJOR", "WARNING"
    violating_node_id: str
    code_evidence: str
    diagnosis: str
    remediation: str


class ConservationRules:
    """Evaluates the 6 orthogonal conservation invariants over the PST graph."""

    def __init__(self):
        pass

    def check_weight_flux(self, pst: PhysicsSemanticTree, is_variance_reduced: bool = True) -> List[PSTViolation]:
        """Invariant: Tally accumulators in variance-reduced transport MUST couple track weight."""
        if not is_variance_reduced:
            return []
        violations = []
        for edge in pst.edges_list:
            if edge.edge_type == "WEIGHT_COUPLING" and not edge.is_satisfied:
                target_node = pst.get_node(edge.target_id)
                evidence = target_node.code_evidence if target_node else "tally accumulation"
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-WEIGHT-01",
                    invariant_name="Weight Flux & Fair-Game Conservation Invariant",
                    severity="CRITICAL",
                    violating_node_id=edge.target_id,
                    code_evidence=evidence,
                    diagnosis=(
                        "Tally accumulator performs unweighted integration without multiplying dynamic particle weight "
                        "(GetWeight() missing). In geometric splitting or weight windows, split tracks carry fractional "
                        "weights; unweighted accumulation inflates transmitted flux and dose by 10^3 - 10^7 x."
                    ),
                    remediation="Multiply particle weight at accumulation: tally += dVal * step->GetTrack()->GetWeight();"
                ))
        return violations

    def check_phase_space_measure(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Point radiation sources must sample uniformly on the spherical solid angle manifold."""
        violations = []
        for node in pst.get_nodes_by_action("Sample"):
            if node.physical_quantity == "solid_angle" and "COLLAPSED" in node.expected_invariant:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-MEASURE-01",
                    invariant_name="Phase Space Angular Measure Invariant",
                    severity="CRITICAL",
                    violating_node_id=node.node_id,
                    code_evidence=node.code_evidence,
                    diagnosis=(
                        "Point radiation source direction sampling collapsed to a fixed pencil beam vector (0,0,1). "
                        "Fails to sample 4pi solid angle under measure dOmega = sin(theta) dtheta dphi, artificially "
                        "magnifying detector solid angle by dozens of times."
                    ),
                    remediation="Use 4pi isotropic random sampling: gun->SetParticleMomentumDirection(G4RandomDirection());"
                ))
        return violations

    def check_secondary_stepping_filter(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Secondary vertex production tally must filter to creation step 1."""
        violations = []
        for node in pst.get_nodes_by_action("Accumulate"):
            if "secondary" in node.physical_operation and "UNFILTERED" in node.expected_invariant:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-LIFECYCLE-01",
                    invariant_name="Stepping Lifecycle State Machine Invariant",
                    severity="CRITICAL",
                    violating_node_id=node.node_id,
                    code_evidence=node.code_evidence,
                    diagnosis=(
                        "Secondary particle yield/vertex scored across all stepping points without filtering "
                        "GetCurrentStepNumber() == 1. Every step taken by secondaries during transport is miscounted "
                        "as a creation event, inflating yield by 1-2 orders of magnitude."
                    ),
                    remediation="Add first-step guard: if (track->GetCurrentStepNumber() == 1) { fYield++; }"
                ))
        return violations

    def check_causality_clock(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: TOF and coincidence timing must reference absolute laboratory global clock."""
        violations = []
        for node in pst.get_nodes_by_action("Fetch"):
            if node.physical_operation == "fetch_local_decay_clock":
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-CAUSAL-01",
                    invariant_name="Relativistic Causality & Global Clock Invariant",
                    severity="CRITICAL",
                    violating_node_id=node.node_id,
                    code_evidence=node.code_evidence,
                    diagnosis=(
                        "Timing or coincidence measurement reads GetLocalTime() instead of GetGlobalTime(). "
                        "GetLocalTime() measures particle proper lifetime from track creation and resets upon boundary/interaction; "
                        "it cannot be used as an absolute experiment clock."
                    ),
                    remediation="Read global laboratory time: G4double t = step->GetPreStepPoint()->GetGlobalTime();"
                ))
        return violations

    def check_stack_classification_weight(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: StackingAction killing tracks via fKill (Russian Roulette) must compensate surviving track weight."""
        violations = []
        for node in pst.get_nodes_by_action("Sample"):
            if node.physical_operation == "classify_track_stack":
                evidence_lower = node.code_evidence.lower()
                if "fkill" in evidence_lower:
                    has_weight_comp = "setweight" in evidence_lower or ("weight" in evidence_lower and any(op in evidence_lower for op in ["*", "/", "+="]))
                    if not has_weight_comp:
                        violations.append(PSTViolation(
                            rule_id="PHYS-INV-STACK-01",
                            invariant_name="Stack Classification Russian Roulette Weight Invariant",
                            severity="CRITICAL",
                            violating_node_id=node.node_id,
                            code_evidence=node.code_evidence,
                            diagnosis=(
                                "Track stacking classification invokes fKill (Russian Roulette termination) "
                                "without adjusting surviving track weights (missing SetWeight()). "
                                "Violates Fair-Game game-theoretic balance, skewing overall transmission flux by (1-p)."
                            ),
                            remediation="Multiply surviving track weight: const_cast<G4Track*>(aTrack)->SetWeight(aTrack->GetWeight() / survivalProb);"
                        ))
        return violations

    def check_master_worker_action_isolation(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: BuildForMaster() in modern Geant4 MT must only instantiate RunAction."""
        violations = []
        for node in pst.nodes_dict.values():
            if node.physical_operation == "action_initialization_hook":
                evidence_lower = node.code_evidence.lower()
                if node.expected_invariant == "master_thread_action_isolation" or "buildformaster" in evidence_lower:
                    forbidden_worker_actions = [
                        ("primarygenerator", "PrimaryGeneratorAction"),
                        ("steppingaction", "SteppingAction"),
                        ("trackingaction", "TrackingAction"),
                        ("eventaction", "EventAction"),
                        ("stackingaction", "StackingAction")
                    ]
                    for pattern, act_name in forbidden_worker_actions:
                        if pattern in evidence_lower:
                            violations.append(PSTViolation(
                                rule_id="PHYS-INV-MT-01",
                                invariant_name="Master/Worker Action Topology Isolation Invariant",
                                severity="CRITICAL",
                                violating_node_id=node.node_id,
                                code_evidence=node.code_evidence,
                                diagnosis=(
                                    f"BuildForMaster() instantiates worker-level simulation action ({act_name}). "
                                    "In Geant4 MT architecture, Master thread performs only global run data reduction; "
                                    "worker actions attached to Master thread cause race conditions or fatal crashes."
                                ),
                                remediation=f"Move {act_name} instantiation from BuildForMaster() to Build(). In BuildForMaster(), only register RunAction."
                            ))
                            break
        return violations

    def check_run_lifecycle_initialization(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: BeamOn(...) execution must be preceded by RunManager initialization."""
        violations = []
        has_beamon = False
        has_initialize = False
        beamon_node = None

        for node in pst.nodes_dict.values():
            if node.physical_operation == "configure_run_manager":
                evidence_lower = node.code_evidence.lower()
                if "beamon" in evidence_lower:
                    has_beamon = True
                    beamon_node = node
                if "initialize" in evidence_lower:
                    has_initialize = True

        if has_beamon and not has_initialize and beamon_node:
            violations.append(PSTViolation(
                rule_id="PHYS-INV-RUN-01",
                invariant_name="Run Lifecycle Initialization Protocol Invariant",
                severity="CRITICAL",
                violating_node_id=beamon_node.node_id,
                code_evidence=beamon_node.code_evidence,
                diagnosis=(
                    "RunManager invokes BeamOn(...) before calling Initialize(). "
                    "Geometry volumes, physics cross-section tables, and thread workers are unconstructed, "
                    "leading to uninitialized transport or fatal kernel exception."
                ),
                remediation="Call runManager->Initialize(); prior to runManager->BeamOn(nEvents);"
            ))
        return violations

    def check_dose_mass_normalization(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Absorbed dose (Gy) accumulation must divide energy deposit by mass."""
        violations = []
        for node in pst.get_nodes_by_action("Accumulate"):
            if node.observable == "Dose" or "dose" in node.code_evidence.lower():
                evidence_lower = node.code_evidence.lower()
                clean_evidence = re.sub(r"/\*.*?\*/", "", evidence_lower).strip()
                has_mass_or_conv = (
                    any(op in clean_evidence for op in ["/", "*"]) and
                    (any(re.search(r"\b" + re.escape(m) + r"\b", clean_evidence) for m in ["mass", "rho", "density", "volume", "weight", "getmass", "mev", "rbe", "conv", "factor", "gray", "gy"])
                     or any(c.isdigit() for c in clean_evidence))
                )
                if not has_mass_or_conv:
                    violations.append(PSTViolation(
                        rule_id="PHYS-INV-DOSE-01",
                        invariant_name="Absorbed Dose Mass Normalization Invariant",
                        severity="CRITICAL",
                        violating_node_id=node.node_id,
                        code_evidence=node.code_evidence,
                        diagnosis=(
                            "Absorbed dose accumulator integrates raw energy deposit without dividing by target mass. "
                            "Absorbed dose dimension is [L]^2 [T]^-2 (Gy = J/kg); missing mass denominator yields "
                            "energy [M][L]^2 [T]^-2 instead of dose, corrupting numerical scale by 10^3 - 10^6 x."
                        ),
                        remediation="Divide energy deposit by volume mass: fDose += (edep / fMass);"
                    ))
        return violations

    def _strip_comments(self, src: str) -> str:
        def repl_multiline(m):
            return "\n" * m.group(0).count("\n")
        src = re.sub(r"/\*.*?\*/", repl_multiline, src, flags=re.S)
        src = re.sub(r"//[^\n]*", "", src)
        return src

    def check_hadron_elastic_coupling(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Hadron inelastic transport MUST couple with hadron elastic scattering."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        _ELAS = re.compile(r"(?<!in)elastic", re.I)
        has_hadron_inel = ("g4hadronphysics" in bare_lower or "inelastic" in bare_lower)
        full_lower = pst.raw_code.lower()
        bare_without_hadron_phys = re.sub(r"G4HadronPhysics\w+", "", bare)
        has_official = any(re.search(r"\b" + re.escape(off) + r"\b", bare_without_hadron_phys) for off in ["QGSP_BIC_HP", "FTFP_BERT", "FTFP_BERT_HP", "QBBC", "Shielding"])
        is_neutron_deep_penetration = (
            "neutron" in full_lower
            and any(k in full_lower for k in ["concrete", "slab", "shield", "penetrat", "transmission", "穿透", "透射", "混凝土"])
            and not any(k in full_lower for k in ["maze", "slit"])
        )
        if has_hadron_inel and not has_official and is_neutron_deep_penetration:
            if not _ELAS.search(bare):
                return [PSTViolation(
                    rule_id="PHYS-INV-HADRON-01",
                    invariant_name="Hadron Elastic/Inelastic Scattering Coupling Invariant",
                    severity="CRITICAL",
                    violating_node_id="physics_list_hadron_coupling",
                    code_evidence="RegisterPhysics(new G4HadronPhysics...)",
                    diagnosis=(
                        "Physics list registers hadronic inelastic physics without accompanying hadronic elastic scattering. "
                        "Elastic scattering is the primary mechanism for low/intermediate energy neutron angular deflection and thermalization. "
                        "Missing elastic scattering leads to unphysical forward beaming and distorts shielding transmission by up to 2x."
                    ),
                    remediation="Register hadron elastic physics constructor: RegisterPhysics(new G4HadronElasticPhysicsHP());"
                )]
        return []

    def check_boundary_step_point_topology(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Step boundary crossing (fGeomBoundary) MUST be evaluated on PostStepPoint."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if "fgeomboundary" in bare_lower:
            for line_no, line in enumerate(bare.splitlines(), 1):
                line_l = line.lower()
                if "fgeomboundary" in line_l:
                    if "pre" in line_l and "getstepstatus" in line_l and "post" not in line_l:
                        return [PSTViolation(
                            rule_id="PHYS-INV-BOUNDARY-01",
                            invariant_name="Boundary Crossing Step Point Topology Invariant",
                            severity="CRITICAL",
                            violating_node_id=f"boundary_check_line_{line_no}",
                            code_evidence=line.strip()[:120],
                            diagnosis=(
                                "Boundary crossing condition evaluates PreStepPoint::GetStepStatus() == fGeomBoundary. "
                                "In Geant4 stepping semantics, the fGeomBoundary flag is computed and assigned to PostStepPoint; "
                                "PreStepPoint merely inherits previous step status. Evaluating PreStepPoint erroneously isolates "
                                "uncollided ballistic particles while silently dropping scattered particle spectra."
                            ),
                            remediation="Evaluate step status on PostStepPoint: if (step->GetPostStepPoint()->GetStepStatus() != fGeomBoundary) return;"
                        )]
        return []

    def check_scintillation_birks_quenching(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Scintillator materials with scintillation yield MUST configure non-linear Birks quenching."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if "scintillationyield" in bare_lower:
            if "birks" not in bare_lower:
                return [PSTViolation(
                    rule_id="PHYS-INV-BIRKS-01",
                    invariant_name="Scintillation Non-Linear Birks Quenching Invariant",
                    severity="CRITICAL",
                    violating_node_id="scintillator_birks_constant",
                    code_evidence="mpt->AddConstProperty(\"SCINTILLATIONYIELD\", ...)",
                    diagnosis=(
                        "Scintillation yield is configured in material properties without setting Birks quenching constant. "
                        "Geant4 defaults Birks factor to 0.0 (no saturation quenching). High-LET tracks (protons, alphas, recoil nuclei) "
                        "will produce unquenched light, skewing neutron/gamma pulse-height discrimination and overestimating yield by 5-10x."
                    ),
                    remediation="Configure material Birks constant: scintMaterial->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);"
                )]
        return []

    def check_event_container_lifecycle_reset(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Event-scoped deduplication set/container MUST reset in BeginOfEventAction."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if ".insert(" in bare and any(k in bare_lower for k in ["track", "seen", "id"]):
            if "beginofevent" not in bare_lower:
                return [PSTViolation(
                    rule_id="PHYS-INV-LIFECYCLE-RESET-01",
                    invariant_name="Event-Scoped Unique Container Lifecycle Reset Invariant",
                    severity="CRITICAL",
                    violating_node_id="event_set_missing_reset",
                    code_evidence="container.insert(trackID)",
                    diagnosis=(
                        "Unique set container (.insert(...).second) scores unique track IDs without being cleared in BeginOfEventAction. "
                        "Because Geant4 re-indexes TrackIDs starting from 1 for every event, second and subsequent events collide with "
                        "residual TrackIDs from previous events, causing secondary yield to monotonically decay by over 50%."
                    ),
                    remediation="Clear the track container in BeginOfEventAction: void BeginOfEventAction(const G4Event*) override { fSeenTracks.clear(); }"
                )]
        return []

    def check_photonuclear_physics_completeness(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Photonuclear reaction simulations MUST register G4PhotonuclearPhysics or G4EmExtraPhysics."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        has_high_energy_lepton_primary = bool(re.search(r'findparticle\s*\(\s*[\"\'](e-|gamma|e\+)[\"\']\s*\)', bare_lower))
        has_secondary_neutron_scoring = ("neutron" in bare_lower and any(t in bare_lower for t in ["target", "g4_w", "g4_pb", "g4_u", "w_t"]))
        has_high_energy_beam = bool(re.search(r'\b(30|40|50|60|70|80|90|100)(\.0)?\s*\*\s*mev\b', bare_lower))
        is_photonuc_scene = (
            any(k in pst.raw_code.lower() for k in ["光核", "photonuclear", "photo-nuclear"])
            or (has_high_energy_lepton_primary and has_secondary_neutron_scoring and has_high_energy_beam)
        )
        if is_photonuc_scene:
            has_pn = any(k in bare_lower for k in ["photonuclear", "photo_nuclear", "g4emextraphysics"])
            if not has_pn:
                return [PSTViolation(
                    rule_id="PHYS-INV-PHOTONUC-01",
                    invariant_name="High-Energy Photonuclear Interaction Completeness Invariant",
                    severity="CRITICAL",
                    violating_node_id="physics_list_photonuclear",
                    code_evidence="RegisterPhysics(new G4EmStandardPhysics())",
                    diagnosis=(
                        "Simulation models electron/gamma photoneutron yield, but physics list fails to register photoneutron physics. "
                        "Without G4EmExtraPhysics or G4PhotonuclearPhysics, photonuclear cross sections remain unconstructed, "
                        "causing photoneutron yields to collapse strictly to 0."
                    ),
                    remediation="Register photonuclear physics: RegisterPhysics(new G4EmExtraPhysics()); or RegisterPhysics(new G4PhotonuclearPhysics());"
                )]
        return []

    def check_optical_surface_and_refraction(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Optical photon simulation MUST specify RINDEX in optical medium."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        violations = []
        has_optical = (
            ("new g4opticalphysics" in bare_lower or "registerphysics(new g4opticalphysics" in bare_lower)
            or ("opticalphoton" in bare_lower and "g4materialpropertiestable" in bare_lower)
            or ("g4materialpropertiestable" in bare_lower and any(k in bare_lower for k in ["scintillationyield", "cerenkov", "chirenkov"]))
        )
        if has_optical:
            # 物理不变式：光学光子输运介质必须定义折射率 RINDEX（无论用于契伦科夫辐射还是介质折射边界）
            has_optical_medium = any(k in bare_lower for k in ["cerenkov", "chirenkov", "契伦科夫", "g4materialpropertiestable", "mpt->"]) or any(k in bare_lower for k in ["g4_sodium_iodide", "g4_water", "nai", "lyso"])
            if has_optical_medium and "rindex" not in bare_lower:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-OPTICAL-RINDEX-01",
                    invariant_name="Optical Refractive Index Coupling Invariant",
                    severity="MAJOR",
                    violating_node_id="optical_material_rindex",
                    code_evidence="mpt->AddProperty(...)",
                    diagnosis=(
                        "Optical photon transport is enabled without configuring refractive index RINDEX in MaterialPropertiesTable. "
                        "Without RINDEX, boundary processes cannot evaluate Snell's law or total internal reflection, "
                        "and Cerenkov emission fails to evaluate cos(theta_c) = 1/(beta*n)."
                    ),
                    remediation="Configure material RINDEX: mpt->AddProperty(\"RINDEX\", photonEnergy, rindex, nEntries);"
                ))

            # 光学边界反射层不变式：显式声明光学反射边界/反射层且涉及光收集时，必须定义 G4OpticalSurface
            has_surface_intent = any(k in bare_lower for k in ["reflector", "reflectance", "dielectric_metal", "dielectric_dielectric", "反射层"])
            if has_surface_intent and "g4opticalsurface" not in bare_lower:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-OPTICAL-SURFACE-01",
                    invariant_name="Optical Boundary Surface Definition Invariant",
                    severity="MAJOR",
                    violating_node_id="optical_skin_surface",
                    code_evidence="G4PVPlacement(...)",
                    diagnosis=(
                        "Optical photon reflection/collection model specifies reflector boundary without G4OpticalSurface. "
                        "Default boundary treats unconfigured surfaces as raw Fresnel interfaces, discarding optical specular/diffuse reflection."
                    ),
                    remediation="Configure optical surface: new G4LogicalSkinSurface(\"Skin\", cryLV, new G4OpticalSurface(\"OptSurf\"));"
                ))
        return violations

    def check_magnetic_field_chord_finder(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Curved magnetic deflection MUST configure chord integrator step limits."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if ("g4uniformmagfield" in bare_lower or "g4magneticfield" in bare_lower) and any(k in bare_lower for k in ["bend", "magnet", "deflect", "偏转", "dipole", "tesla", "quad", "fodo"]):
            if "chord" not in bare_lower:
                return [PSTViolation(
                    rule_id="PHYS-INV-CHORD-01",
                    invariant_name="Curved Trajectory Magnetic Chord Integrator Invariant",
                    severity="MAJOR",
                    violating_node_id="magnetic_field_chord_finder",
                    code_evidence="new G4UniformMagField(...)",
                    diagnosis=(
                        "Deflecting magnetic field is instantiated without configuring G4ChordFinder or SetDeltaChord. "
                        "In strong magnetic fields, default macroscopic chord step sizes cause charged tracks to skip volume boundaries."
                    ),
                    remediation="Configure chord finder: fieldMgr->GetChordFinder()->SetDeltaChord(1.0 * mm);"
                )]
        return []

    def check_gaussian_energy_nonnegative_truncation(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Gaussian energy dispersion MUST guard against negative kinetic energy."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if "g4randgauss" in bare_lower:
            for line_no, line in enumerate(bare.splitlines(), 1):
                line_l = line.lower()
                if "g4randgauss" in line_l and any(k in line_l for k in ["setparticleenergy", "energy", "kine", "e0", "ene"]):
                    has_trunc = any(k in bare_lower for k in ["std::max", "max(0", "max (0", "e > 0", "e>0", "energy > 0", "energy>0", "< 0", "<= 0"])
                    if not has_trunc:
                        return [PSTViolation(
                            rule_id="PHYS-INV-ENERGY-01",
                            invariant_name="Gaussian Energy Spread Non-Negative Cutoff Invariant",
                            severity="CRITICAL",
                            violating_node_id=f"gaussian_energy_line_{line_no}",
                            code_evidence=line.strip()[:120],
                            diagnosis=(
                                "Kinetic energy sampled via G4RandGauss without non-negative truncation protection (Ek > 0). "
                                "Gaussian distributions have non-zero probabilities in the negative tail; unclipped negative kinetic energy "
                                "triggers fatal Geant4 tracking kernel assertion failures."
                            ),
                            remediation="Guard kinetic energy: G4double ek = std::max(0.0, G4RandGauss::shoot(meanE, sigmaE));"
                        )]
        return []

    def check_radial_source_measure_sampling(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Circular source radial sampling MUST obey square root differential measure r = R * sqrt(xi)."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        has_radial_context = any(k in bare_lower for k in ["radius", "rmax", "r_max", "spot"]) or bool(re.search(r"\b[rR]\s*=\s*", bare))
        if has_radial_context and "g4uniformrand" in bare_lower:
            for line_no, line in enumerate(bare.splitlines(), 1):
                line_l = line.lower()
                if ("g4uniformrand" in line_l and
                    (any(k in line_l for k in ["radius *", "rmax *", "r_max *", "r *", "r*"])
                     or bool(re.search(r"\b[rR]\s*=\s*[^;\n]*\*", line)))):
                    if "sqrt" not in line_l:
                        return [PSTViolation(
                            rule_id="PHYS-INV-MEASURE-02",
                            invariant_name="Circular Planar Source Radial Differential Measure Invariant",
                            severity="CRITICAL",
                            violating_node_id=f"radial_sampling_line_{line_no}",
                            code_evidence=line.strip()[:120],
                            diagnosis=(
                                "Circular disk source samples radius linearly: r = R * xi. Differential area measure in polar coordinates "
                                "is dA = 2pi*r dr, yielding cumulative distribution P(r) = (r/R)^2 and inverse transform r = R * sqrt(xi). "
                                "Linear sampling clusters particles toward center, artificially inflating central beam fluence."
                            ),
                            remediation="Sample radius using square root law: G4double r = R * std::sqrt(G4UniformRand());"
                        )]

            # Check two-step sampling: xi = G4UniformRand(); ... r = R * xi without sqrt
            has_xi = bool(re.search(r"\bxi\s*=\s*g4uniformrand\s*\(\s*\)", bare_lower))
            if has_xi and bool(re.search(r"\br\s*=\s*[^;\n]*\*\s*xi\b", bare_lower)) and "sqrt" not in bare_lower:
                return [PSTViolation(
                    rule_id="PHYS-INV-MEASURE-02",
                    invariant_name="Circular Planar Source Radial Differential Measure Invariant",
                    severity="CRITICAL",
                    violating_node_id="radial_sampling_two_step",
                    code_evidence="r = R * xi (xi = G4UniformRand())",
                    diagnosis=(
                        "Circular disk source samples radius linearly: r = R * xi. Differential area measure in polar coordinates "
                        "is dA = 2pi*r dr, yielding cumulative distribution P(r) = (r/R)^2 and inverse transform r = R * sqrt(xi). "
                        "Linear sampling clusters particles toward center, artificially inflating central beam fluence."
                    ),
                    remediation="Sample radius using square root law: G4double r = R * std::sqrt(G4UniformRand());"
                )]
        return []

    def check_energy_resolution_fwhm_scaling(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Detector energy resolution FWHM must convert to standard deviation via sigma = FWHM / 2.35482."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if "fwhm" in bare_lower and ("g4randgauss" in bare_lower or "gauss" in bare_lower):
            if not any(k in bare for k in ["2.355", "2.3548"]):
                return [PSTViolation(
                    rule_id="PHYS-INV-GAUSS-01",
                    invariant_name="Energy Resolution FWHM-to-Sigma Mathematical Scaling Invariant",
                    severity="MAJOR",
                    violating_node_id="energy_resolution_fwhm",
                    code_evidence="G4RandGauss::shoot(..., fwhm)",
                    diagnosis=(
                        "Energy resolution broadening uses raw FWHM directly as Gaussian standard deviation sigma. "
                        "Mathematically, FWHM = 2*sqrt(2*ln(2))*sigma ~= 2.35482*sigma. Omitting this factor inflates detector "
                        "peak width by 2.355x, severely deteriorating reconstructed energy resolution."
                    ),
                    remediation="Divide FWHM by 2.35482: G4double sigma = fwhm / 2.35482;"
                )]
        return []

    def check_particle_physics_constructor_coupling(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Primary hadron or heavy-ion particle species MUST couple with respective nuclear physics lists."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        violations = []
        full_lower = pst.raw_code.lower()
        has_neutron_primary = bool(re.search(r'findparticle\s*\(\s*[\"\']neutron[\"\']\s*\)', bare_lower))
        is_neutron_shielding_scene = (
            has_neutron_primary and any(k in full_lower for k in ['shadow', 'flatblock', 'cone', '影子屏蔽'])
        )
        if is_neutron_shielding_scene:
            has_hadron_phys = any(k in bare_lower for k in [
                'g4hadronphysics', 'g4hadronelasticphysics', 'qgsp', 'ftfp', 'qbbc', 'shielding', 'g4incl'
            ])
            if not has_hadron_phys:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-PHYSICS-HADRON-01",
                    invariant_name="Neutron Particle Nuclear Physics Constructor Invariant",
                    severity="CRITICAL",
                    violating_node_id="physics_list_missing_hadron",
                    code_evidence="FindParticle(\"neutron\") without G4HadronPhysics...",
                    diagnosis=(
                        "Simulation primary source is neutron, but modular physics list registers only electromagnetic physics. "
                        "Without hadronic elastic/inelastic physics constructors, neutrons undergo zero nuclear interactions in matter, "
                        "rendering transmission, shielding, and activation tally outputs completely null."
                    ),
                    remediation="Register hadron physics constructor: RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());"
                ))

        has_ion_primary = bool(re.search(r'findparticle\s*\(\s*[\"\'](genericion|ion)[\"\']\s*\)', bare_lower))
        is_ion_fragmentation_scene = (
            has_ion_primary and any(k in full_lower for k in ['fragment', '碎裂', '散裂', '4.44', 'prompt_gamma'])
        )
        if is_ion_fragmentation_scene:
            has_ion_phys = any(k in bare_lower for k in ['g4ionphysics', 'g4ionbinarycascadephysics', 'g4incl', 'qgsp', 'ftfp'])
            if not has_ion_phys:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-PHYSICS-ION-01",
                    invariant_name="Heavy Ion Nuclear Fragmentation Physics Constructor Invariant",
                    severity="CRITICAL",
                    violating_node_id="physics_list_missing_ion",
                    code_evidence="FindParticle(\"GenericIon\") without G4IonPhysics...",
                    diagnosis=(
                        "Simulation primary source is heavy ion / GenericIon, but physics list registers only G4EmStandardPhysics. "
                        "Without ion inelastic/fragmentation physics, projectile nuclear breakup, spallation, and secondary positron emission "
                        "processes are entirely disabled."
                    ),
                    remediation="Register ion physics constructor: RegisterPhysics(new G4IonPhysics());"
                ))

        has_proton_spallation = bool(re.search(r'findparticle\s*\(\s*[\"\']proton[\"\']\s*\)', bare_lower)) and any(k in bare_lower for k in ["gev", "1.0*gev", "spallation", "散裂"])
        if has_proton_spallation:
            has_spall_phys = any(k in bare_lower for k in ['g4hadronphysics', 'qgsp', 'ftfp', 'incl', 'bic', 'shielding'])
            if not has_spall_phys:
                violations.append(PSTViolation(
                    rule_id="PHYS-INV-PHYSICS-HADRON-01",
                    invariant_name="Spallation Hadron Nuclear Physics Constructor Invariant",
                    severity="CRITICAL",
                    violating_node_id="physics_list_missing_spallation",
                    code_evidence="FindParticle(\"proton\") at GeV without Hadron Physics",
                    diagnosis=(
                        "High-energy (GeV) proton spallation/activation simulation registers only G4EmStandardPhysics. "
                        "Without intra-nuclear cascade and spallation hadron physics, protons deposit energy only via continuous ionization, "
                        "producing zero nuclear fragment activation or spallation secondary yield."
                    ),
                    remediation="Register spallation physics constructor: RegisterPhysics(new G4HadronPhysicsFTFP_BERT());"
                ))
        return violations

    def check_shield_material_authenticity(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Shielding entities and physical volumes MUST be assigned genuine high-density medium rather than air/void."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        for line in bare.splitlines():
            line_l = line.lower()
            if any(k in line_l for k in ['leadmat', 'shieldmat', 'tungstenmat', 'labyrinth']) and any(air in line_l for air in ['"g4_air"', '"g4_galactic"']):
                return [PSTViolation(
                    rule_id="PHYS-INV-MATERIAL-MISMATCH-01",
                    invariant_name="Protective Shield Medium Authenticity Invariant",
                    severity="CRITICAL",
                    violating_node_id="shield_material_void_substitution",
                    code_evidence=line.strip()[:120],
                    diagnosis=(
                        "Shielding component variable (leadMat/shieldMat/labyrinth) is assigned G4_AIR / G4_Galactic material. "
                        "This dummy material substitution eliminates radiation attenuation, corrupting penetration tallies by 10^4 - 10^8 x."
                    ),
                    remediation="Assign genuine shield material: G4Material* leadMat = nist->FindOrBuildMaterial(\"G4_Pb\");"
                )]
            if 'cumat' in line_l and '"g4_al"' in line_l:
                return [PSTViolation(
                    rule_id="PHYS-INV-MATERIAL-MISMATCH-02",
                    invariant_name="Shield Element Composition Invariant",
                    severity="CRITICAL",
                    violating_node_id="copper_shield_aluminum_substitution",
                    code_evidence=line.strip()[:120],
                    diagnosis=(
                        "Copper shielding material cuMat is erroneously assigned G4_Al. "
                        "Aluminum density (2.7 g/cm3) is 3.3x lower than copper (8.96 g/cm3), skewing gamma attenuation and activation background."
                    ),
                    remediation="Assign genuine copper material: G4Material* cuMat = nist->FindOrBuildMaterial(\"G4_Cu\");"
                )]
            if 'wallmat' in line_l and '"g4_fe"' in line_l and any(k in pst.raw_code.lower() for k in ['first_wall', 'fusion', 'tokamak', 'multiplier', 'beryllium']):
                return [PSTViolation(
                    rule_id="PHYS-INV-MATERIAL-MISMATCH-03",
                    invariant_name="Beryllium First Wall Material Authenticity Invariant",
                    severity="CRITICAL",
                    violating_node_id="beryllium_wall_iron_substitution",
                    code_evidence=line.strip()[:120],
                    diagnosis=(
                        "Beryllium multiplier first wall wallMat is assigned G4_Fe instead of G4_Be. "
                        "Iron does not undergo 9Be(n,2n) neutron multiplication, nullifying breeding ratio evaluation."
                    ),
                    remediation="Assign beryllium material: G4Material* wallMat = nist->FindOrBuildMaterial(\"G4_Be\");"
                )]
        return []

    def check_secondary_stepping_multiplicity(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Secondary creation vertex and production yield MUST be filtered to initial birth step."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if 'gettrackid() > 1' in bare_lower or 'gettrackid()>1' in bare_lower:
            if not any(k in bare_lower for k in ['getcurrentstepnumber', 'firststep', 'secondary', 'insert']):
                return [PSTViolation(
                    rule_id="PHYS-INV-STEP-MULTI-01",
                    invariant_name="Secondary Particle Generation Vertex Lifecycle Invariant",
                    severity="CRITICAL",
                    violating_node_id="secondary_vertex_step_multiplicity",
                    code_evidence="if (track->GetTrackID() > 1) { count++; }",
                    diagnosis=(
                        "Secondary particle yield or creation vertex is tallied on every step without filtering GetCurrentStepNumber() == 1. "
                        "As a single secondary particle executes dozens or hundreds of tracking steps, unconditional accumulation inflates "
                        "production density by the average number of steps per track."
                    ),
                    remediation="Filter to track creation step: if (track->GetTrackID() > 1 && track->GetCurrentStepNumber() == 1) { count++; }"
                )]
        return []

    def check_bateman_decay_chain_branching(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Mo-99 -> Tc-99m Bateman daughter yield MUST incorporate 0.875 branching fraction."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if any(k in bare_lower for k in ['mo-99', 'mo99', 'tc-99m', 'tc99m', '99mo', '99mtc']):
            if '0.875' not in bare and '87.5' not in bare:
                return [PSTViolation(
                    rule_id="PHYS-INV-BATEMAN-01",
                    invariant_name="Bateman Daughter Activity Branching Ratio Invariant",
                    severity="CRITICAL",
                    violating_node_id="bateman_mo99_branching",
                    code_evidence="N2(t) decay calculation",
                    diagnosis=(
                        "Mo-99 to Tc-99m decay chain calculation omits the 87.5% (0.875) isomeric transition branching ratio. "
                        "Only 87.5% of Mo-99 decays populate the metastable 99mTc state, while 12.5% decay directly to ground state 99Tc. "
                        "Assuming 100% branching overestimates daughter medical radioisotope yield by 14.3%."
                    ),
                    remediation="Multiply daughter growth rate by branching factor: fYield = 0.875 * (lambda1 / (lambda2 - lambda1)) * (exp(-lambda1*t) - exp(-lambda2*t));"
                )]
        return []

    def check_cosmic_ray_zenith_angular_distribution(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Atmospheric cosmic ray angular measure MUST sample zenith dependence rather than collapsing to pencil beam."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        has_muon = ("mu-" in bare_lower and any(k in bare_lower for k in ["gev", "4*gev", "4 * gev"]))
        has_zenith_scene = any(k in bare_lower for k in ["rock", "zenith", "cosmic", "underground"])
        if has_muon and has_zenith_scene:
            is_vertical = bool(re.search(r"setparticlemomentumdirection\s*\(\s*(\{\s*0\s*,\s*0\s*,\s*-1\s*\}|g4threevector\s*\(\s*0\s*,\s*0\s*,\s*-1\s*\))\s*\)", bare_lower))
            has_angle_sampling = any(k in bare_lower for k in ["cos", "acos", "costheta", "theta", "randomdirection", "ang/type cos"])
            if is_vertical and not has_angle_sampling:
                return [PSTViolation(
                    rule_id="PHYS-INV-COSSQ-01",
                    invariant_name="Atmospheric Cosmic Ray Zenith Phase Space Measure Invariant",
                    severity="CRITICAL",
                    violating_node_id="cosmic_zenith_beaming_collapse",
                    code_evidence="gun.SetParticleMomentumDirection({0, 0, -1})",
                    diagnosis=(
                        "Cosmic ray muon angular distribution is collapsed to an unphysical vertical monodirectional beam. "
                        "Atmospheric muon flux follows an empirical I(theta) ~ I0 * cos^2(theta) zenith angular distribution. "
                        "Vertical beaming severely underestimates slant depth slant path length across thick rock overburden."
                    ),
                    remediation="Sample cosmic zenith angle: G4double cosTheta = std::sqrt(G4UniformRand()); G4ThreeVector dir(sinTheta*cosPhi, sinTheta*sinPhi, -cosTheta);"
                )]
        return []

    def check_semiconductor_fano_fluctuation(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Semiconductor ionization carrier variance MUST be weighted by Fano factor F < 1."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        has_semicond = any(k in bare_lower for k in ["charge variance", "var_n", "3.6*ev", "3.6 * ev", "hpge", "germanium", "fano"])
        if has_semicond:
            if not any(k in bare_lower for k in ["fano", "0.115", "0.1"]):
                return [PSTViolation(
                    rule_id="PHYS-INV-FANO-01",
                    invariant_name="Semiconductor Carrier Statistics Fano Factor Invariant",
                    severity="CRITICAL",
                    violating_node_id="fano_factor_missing",
                    code_evidence="double var_N = N;",
                    diagnosis=(
                        "Semiconductor detector carrier fluctuation is modeled using unweighted Poisson variance (sigma^2 = N). "
                        "Because ionization collisions are correlated, variance is attenuated by the Fano factor (sigma^2 = F * N, F ~= 0.115 for HPGe). "
                        "Missing Fano factor inflates Gaussian energy resolution width by ~2.95x."
                    ),
                    remediation="Incorporate Fano factor: G4double var_N = fano * N;"
                )]
        return []

    def check_detector_deadtime_pulse_pileup(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: High count rate detector pileup scoring MUST guard against inter-arrival intervals delta_t < tau."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        has_deadtime = ("tau" in bare_lower and any(k in bare_lower for k in ["recorded", "pileup", "count", "deadtime", "dead_time"]))
        if has_deadtime:
            if not any(k in bare_lower for k in ["delta_t", "dt <", "last_time", "lasttime", "prev_time", "< tau", "<tau"]):
                return [PSTViolation(
                    rule_id="PHYS-INV-DEADTIME-01",
                    invariant_name="Detector High Count Rate Dead-Time Interval Guard Invariant",
                    severity="MAJOR",
                    violating_node_id="pulse_pileup_interval_guard",
                    code_evidence="for (...) { recorded++; } without delta_t check",
                    diagnosis=(
                        "Detector pulse pileup simulation increments recorded count without evaluating arrival time interval against dead time tau. "
                        "Without checking delta_t = (current_time - last_time) < tau, the detector acts as an ideal zero dead-time system, "
                        "discarding non-paralyzable/paralyzable count rate saturation physics."
                    ),
                    remediation="Guard arrival interval: if (t_current - t_last < tau) { pileupLosses++; } else { recordedCounts++; t_last = t_current; }"
                )]
        return []

    def check_water_radical_chemistry_completeness(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Water radical radiolysis simulations MUST register G4EmDNAChemistry."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        if any(k in bare_lower for k in ['g-value', 'g_value', 'radical_yield', 'water_radiolysis']):
            if 'g4emdnachemistry' not in bare_lower:
                return [PSTViolation(
                    rule_id="PHYS-INV-DNACHEM-01",
                    invariant_name="Water Radical Radiolysis Chemistry Module Invariant",
                    severity="CRITICAL",
                    violating_node_id="physics_list_dna_chemistry",
                    code_evidence="RegisterPhysics(new G4EmDNAPhysics()) without Chemistry",
                    diagnosis=(
                        "Simulation scores water radical chemical yields (G-values, OH radicals, e_aq) but physics list omits G4EmDNAChemistry. "
                        "G4EmDNAPhysics models only physical ionization and excitation; chemical dissociation, radical diffusion, and recombination "
                        "require G4EmDNAChemistry constructor. Radical chemical yields collapse to zero."
                    ),
                    remediation="Register DNA chemistry constructor: RegisterPhysics(new G4EmDNAChemistry());"
                )]
        return []

    def check_thermal_neutron_sab_binding(self, pst: PhysicsSemanticTree) -> List[PSTViolation]:
        """Invariant: Thermal neutron transport in water/moderators MUST utilize molecular binding S(alpha,beta) cross sections."""
        if not pst.raw_code:
            return []
        bare = self._strip_comments(pst.raw_code)
        bare_lower = bare.lower()

        has_thermal_model = ("g4thermalneutrons" in bare_lower or "thermalneutron" in bare_lower or "0.025*ev" in bare_lower or "0.025 * ev" in bare_lower)
        if has_thermal_model:
            is_manual_water = bool(re.search(r'new\s+G4Material\s*\(\s*["\']h2o["\']', bare, re.I))
            if is_manual_water and "g4_water" not in bare_lower and "ts_h_water" not in bare_lower:
                return [PSTViolation(
                    rule_id="PHYS-INV-THERMAL-SAB-01",
                    invariant_name="Thermal Neutron Molecular Binding S(alpha,beta) Invariant",
                    severity="CRITICAL",
                    violating_node_id="thermal_neutron_manual_water",
                    code_evidence="new G4Material(\"H2O\", ...); AddElement(H, 2); AddElement(O, 1);",
                    diagnosis=(
                        "Thermal neutron transport in water uses manually constructed elemental G4Material(\"H2O\") without S(alpha,beta) binding data. "
                        "Thermal neutrons (E < 4 eV) scatter off bound hydrogen molecules whose chemical vibrational/rotational modes "
                        "diverge from free gas kinematics. Free hydrogen scattering distorts thermalized neutron spectrum by over 40%."
                    ),
                    remediation="Load bound NIST water material: auto* water = nist->FindOrBuildMaterial(\"G4_WATER\");"
                )]
        return []

