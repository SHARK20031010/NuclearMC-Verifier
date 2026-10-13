import pytest
from src.layer4_pst.node import PSTNode
from src.layer4_pst.edge import PSTEdge
from src.layer4_pst.tree import PhysicsSemanticTree


def test_pst_graph_structure_and_mermaid_generation():
    pst = PhysicsSemanticTree(name="TestTree")

    n1 = PSTNode(
        node_id="n1",
        action="Fetch",
        physical_operation="fetch_track_weight",
        physical_quantity="track_weight",
        conservation_domain="variance_weight",
        expected_invariant="fair_game_weight_coupling",
        code_evidence="step->GetTrack()->GetWeight()"
    )
    n2 = PSTNode(
        node_id="n2",
        action="Accumulate",
        physical_operation="accumulate_fluence",
        physical_quantity="particle_count_or_weight",
        conservation_domain="variance_weight",
        expected_invariant="weighted_fluence_accumulation",
        code_evidence="fFlux += w;"
    )

    pst.add_node(n1)
    pst.add_node(n2)

    e1 = PSTEdge(
        source_id="n1",
        target_id="n2",
        edge_type="WEIGHT_COUPLING",
        description="Couples track weight to tally",
        is_satisfied=True
    )
    pst.add_edge(e1)

    assert len(pst.nodes_dict) == 2
    assert len(pst.edges_list) == 1

    mermaid = pst.to_mermaid()
    assert "graph TD" in mermaid
    assert "WEIGHT_COUPLING" in mermaid
    assert "fetch_track_weight" in mermaid

    # Serialization roundtrip
    json_str = pst.to_json()
    reconstructed = PhysicsSemanticTree.from_json(json_str)
    assert len(reconstructed.nodes_dict) == 2
    assert len(reconstructed.edges_list) == 1
