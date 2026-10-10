"""
Layer 4: Physics Semantic Tree (PST) Data Structure
NetworkX-backed directed graph representation of physical operations and conservation flows.
"""

import json
from typing import Dict, List, Optional, Any
import networkx as nx
from .node import PSTNode
from .edge import PSTEdge, EdgeType


class PhysicsSemanticTree:
    """The central Intermediate Representation (IR) graph: G_PST = (V_phys, E_flow)."""

    def __init__(self, name: str = "PST", raw_code: str = ""):
        self.name = name
        self.raw_code = raw_code
        self.graph = nx.DiGraph()
        self.nodes_dict: Dict[str, PSTNode] = {}
        self.edges_list: List[PSTEdge] = []

    def add_node(self, node: PSTNode) -> None:
        self.nodes_dict[node.node_id] = node
        self.graph.add_node(
            node.node_id,
            action=node.action,
            op=node.physical_operation,
            quantity=node.physical_quantity,
            domain=node.conservation_domain,
            invariant=node.expected_invariant,
            evidence=node.code_evidence,
            confidence=node.confidence,
            lifecycle=node.lifecycle_tier,
            guards=node.guard_conditions
        )

    def add_edge(self, edge: PSTEdge) -> None:
        self.edges_list.append(edge)
        self.graph.add_edge(
            edge.source_id,
            edge.target_id,
            edge_type=edge.edge_type,
            description=edge.description,
            satisfied=edge.is_satisfied
        )

    def get_node(self, node_id: str) -> Optional[PSTNode]:
        return self.nodes_dict.get(node_id)

    def get_nodes_by_action(self, action: str) -> List[PSTNode]:
        return [n for n in self.nodes_dict.values() if n.action == action]

    def get_nodes_by_domain(self, domain: str) -> List[PSTNode]:
        return [n for n in self.nodes_dict.values() if n.conservation_domain == domain]

    def get_incoming_edges(self, target_id: str, edge_type: Optional[EdgeType] = None) -> List[PSTEdge]:
        edges = [e for e in self.edges_list if e.target_id == target_id]
        if edge_type:
            edges = [e for e in edges if e.edge_type == edge_type]
        return edges

    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "node_count": len(self.nodes_dict),
            "edge_count": len(self.edges_list),
            "nodes": [n.to_dict() for n in self.nodes_dict.values()],
            "edges": [e.to_dict() for e in self.edges_list]
        }

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "PhysicsSemanticTree":
        tree = cls(name=data.get("name", "PST"))
        for n_data in data.get("nodes", []):
            tree.add_node(PSTNode(**n_data))
        for e_data in data.get("edges", []):
            tree.add_edge(PSTEdge(**e_data))
        return tree

    @classmethod
    def from_json(cls, json_str: str) -> "PhysicsSemanticTree":
        return cls.from_dict(json.loads(json_str))

    def to_mermaid(self) -> str:
        """Renders the PST as a Mermaid graph diagram."""
        lines = ["graph TD"]
        for node in self.nodes_dict.values():
            action_tag = f"[{node.action}]"
            label = f"{node.node_id}: {node.physical_operation}\\n{action_tag} {node.physical_quantity}"
            lines.append(f'    {node.node_id}["{label}"]')

        for edge in self.edges_list:
            style = "-->" if edge.is_satisfied else "-. missing .->"
            lines.append(f'    {edge.source_id} {style}|{edge.edge_type}| {edge.target_id}')

        return "\n".join(lines)
