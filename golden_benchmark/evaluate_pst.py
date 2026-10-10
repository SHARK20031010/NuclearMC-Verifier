"""
Golden-PST Evaluation Script
Evaluates predicted PhysicsSemanticTree against ground-truth annotations.
Reports Node-level and Edge-level Precision, Recall, and F1-Scores.
"""

import json
from typing import Dict, Any, Tuple
from src.layer4_pst.tree import PhysicsSemanticTree


class PSTComparator:
    """Computes Node and Edge F1 scores between predicted PST and Ground Truth."""

    @staticmethod
    def evaluate(pred_pst: PhysicsSemanticTree, gt_data: Dict[str, Any]) -> Dict[str, float]:
        gt_nodes = gt_data.get("nodes", [])
        gt_edges = gt_data.get("edges", [])

        pred_nodes = list(pred_pst.nodes_dict.values())
        pred_edges = pred_pst.edges_list

        # 1. Node Matching
        matched_nodes = 0
        for gn in gt_nodes:
            for pn in pred_nodes:
                if (pn.action == gn["action"] and
                    pn.physical_operation == gn["physical_operation"] and
                    pn.expected_invariant == gn["expected_invariant"]):
                    matched_nodes += 1
                    break

        if not pred_nodes and not gt_nodes:
            node_prec, node_rec, node_f1 = 1.0, 1.0, 1.0
        else:
            node_prec = matched_nodes / len(pred_nodes) if pred_nodes else 0.0
            node_rec = matched_nodes / len(gt_nodes) if gt_nodes else 0.0
            node_f1 = (2 * node_prec * node_rec) / (node_prec + node_rec) if (node_prec + node_rec) > 0 else 0.0

        # 2. Edge Matching
        matched_edges = 0
        for ge in gt_edges:
            for pe in pred_edges:
                if (pe.edge_type == ge["edge_type"] and
                    pe.is_satisfied == ge["is_satisfied"]):
                    matched_edges += 1
                    break

        if not pred_edges and not gt_edges:
            edge_prec, edge_rec, edge_f1 = 1.0, 1.0, 1.0
        else:
            edge_prec = matched_edges / len(pred_edges) if pred_edges else 0.0
            edge_rec = matched_edges / len(gt_edges) if gt_edges else 0.0
            edge_f1 = (2 * edge_prec * edge_rec) / (edge_prec + edge_rec) if (edge_prec + edge_rec) > 0 else 0.0

        return {
            "node_precision": round(node_prec, 4),
            "node_recall": round(node_rec, 4),
            "node_f1": round(node_f1, 4),
            "edge_precision": round(edge_prec, 4),
            "edge_recall": round(edge_rec, 4),
            "edge_f1": round(edge_f1, 4)
        }
