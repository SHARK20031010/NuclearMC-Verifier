"""
Layer 4: Physics Semantic Tree (PST) Builder
Assembles physical operation nodes and synthesizes conservation flow edges.
"""

from typing import List, Dict, Tuple
from .node import PSTNode
from .edge import PSTEdge
from .tree import PhysicsSemanticTree
from ..layer2_context.context_packager import LocalContextPackage
from ..layer3_semantic.schema import PhysicalSemanticAnnotation


class PSTBuilder:
    """Assembles G_PST = (V_phys, E_flow) from semantic annotations and context packages."""

    def __init__(self):
        pass

    def build(self, items: List[Tuple[LocalContextPackage, PhysicalSemanticAnnotation]], tree_name: str = "Simulation_PST", raw_code: str = "") -> PhysicsSemanticTree:
        pst = PhysicsSemanticTree(name=tree_name, raw_code=raw_code)

        # 1. Register all physical nodes
        fetch_weight_nodes = []
        accumulate_nodes = []
        sample_nodes = []

        for idx, (pkg, sem) in enumerate(items):
            node_id = f"node_{idx+1:02d}_{sem.action.lower()}"
            pst_node = PSTNode(
                node_id=node_id,
                action=sem.action,
                physical_operation=sem.physical_operation,
                physical_quantity=sem.physical_quantity,
                conservation_domain=sem.conservation_domain,
                expected_invariant=sem.expected_invariant,
                code_evidence=sem.code_evidence,
                confidence=sem.confidence,
                observable=sem.observable,
                unit_dimension=sem.unit_dimension,
                domain_scope=sem.domain_scope,
                lifecycle_tier=sem.lifecycle_tier,
                ast_span=pkg.span,
                guard_conditions=pkg.enclosing_guards
            )
            pst.add_node(pst_node)

            if sem.physical_operation == "fetch_track_weight":
                fetch_weight_nodes.append((pst_node, pkg))
            elif sem.action == "Accumulate":
                accumulate_nodes.append((pst_node, pkg))
            elif sem.action == "Sample":
                sample_nodes.append((pst_node, pkg))

        # 2. Synthesize Conservation Flow Edges
        # 2.1 Weight Coupling Edges
        for acc_node, acc_pkg in accumulate_nodes:
            if acc_node.physical_quantity in {"energy", "particle_count_or_weight"} or "flux" in acc_node.physical_operation or "edep" in acc_node.physical_operation:
                if acc_pkg.has_weight_multiplication:
                    # Satisfied weight coupling
                    src_id = fetch_weight_nodes[0][0].node_id if fetch_weight_nodes else "injected_weight"
                    pst.add_edge(PSTEdge(
                        source_id=src_id,
                        target_id=acc_node.node_id,
                        edge_type="WEIGHT_COUPLING",
                        description="Track weight coupled into tally accumulation",
                        carrier_quantity="track_weight",
                        is_satisfied=True
                    ))
                else:
                    # Unsatisfied / Missing weight coupling edge
                    pst.add_edge(PSTEdge(
                        source_id="MISSING_WEIGHT_SOURCE",
                        target_id=acc_node.node_id,
                        edge_type="WEIGHT_COUPLING",
                        description="DISCONNECTED: Tally accumulates without multiplying GetWeight()",
                        carrier_quantity="track_weight",
                        is_satisfied=False
                    ))

        # 2.2 Lifecycle Container Edges
        for acc_node, acc_pkg in accumulate_nodes:
            pst.add_edge(PSTEdge(
                source_id=acc_node.node_id,
                target_id="EndOfEventAction_Closure",
                edge_type="LIFECYCLE_CONTAINER",
                description=f"Accumulator {acc_node.observable} scoped in {acc_pkg.enclosing_function_name}",
                is_satisfied=True
            ))

        return pst
