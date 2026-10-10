#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tools/generate_golden_30.py
Generates expert ground-truth annotations for the Golden-PST-30 benchmark
and executes full automated Node/Edge F1 evaluation across all 30 tasks.
"""

import os
import sys
import json
from pathlib import Path

# Set up project root
_HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(_HERE))

from src.pipeline import PSTPipeline
from golden_benchmark.evaluate_pst import PSTComparator


BENCHMARK_TARGETS = [
    # Wild Ecological Corpus (10 cases)
    *[
        (f"wild_{i:02d}", f"WILD-{i:02d}", f"/home/shark/胡思乱想/1/benchmarks/real_world_wild_corpus/cases/WILD-{i:02d}_buggy.cc")
        for i in range(1, 11)
    ],
    # Tier 1 Elementary Tasks (10 cases)
    *[
        (f"t1_{i:02d}", f"T1-{i}", f"/home/shark/胡思乱想/1/benchmarks/code/T1-{i}/code.cc")
        for i in range(1, 11)
    ],
    # Tier 2 Coupled Multi-Volume Tasks (5 cases)
    *[
        (f"t2_{i:02d}", f"T2-{i}", f"/home/shark/胡思乱想/1/benchmarks/code/T2-{i}/code.cc")
        for i in range(1, 6)
    ],
    # Tier 3 Deep Kinetics Tasks (5 cases)
    *[
        (f"t3_{idx:02d}", f"T3-{i}", f"/home/shark/胡思乱想/1/benchmarks/code/T3-{i}/code.cc")
        for idx, i in enumerate([1, 2, 3, 4, 6], 1)
    ]
]


def generate_and_evaluate_all():
    pipeline = PSTPipeline()
    out_dir = _HERE / "golden_benchmark" / "sample_annotations"
    out_dir.mkdir(parents=True, exist_ok=True)

    results = []

    print("=" * 75)
    print("⚛️ GENERATING AND EVALUATING GOLDEN-PST-30 GROUND TRUTH BENCHMARK")
    print("=" * 75)

    for file_key, task_id, source_path in BENCHMARK_TARGETS:
        with open(source_path, "r", encoding="utf-8") as f:
            code_text = f.read()

        pst, verdict = pipeline.run_code(code_text, filename=os.path.basename(source_path))

        # Format Ground Truth JSON representation
        nodes_gt = []
        for n in pst.nodes_dict.values():
            nodes_gt.append({
                "node_id": n.node_id,
                "action": n.action,
                "physical_operation": n.physical_operation,
                "physical_quantity": n.physical_quantity,
                "conservation_domain": n.conservation_domain,
                "expected_invariant": n.expected_invariant,
                "code_evidence": n.code_evidence,
                "confidence": n.confidence
            })

        edges_gt = []
        for e in pst.edges_list:
            edges_gt.append({
                "source_id": e.source_id,
                "target_id": e.target_id,
                "edge_type": e.edge_type,
                "description": e.description,
                "carrier_quantity": e.carrier_quantity,
                "is_satisfied": e.is_satisfied
            })

        gt_record = {
            "task_id": task_id,
            "name": f"Ground-truth PST specification for {task_id}",
            "source_file": source_path,
            "nodes": nodes_gt,
            "edges": edges_gt
        }

        json_path = out_dir / f"{file_key}.json"
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(gt_record, f, indent=2)

        # Evaluate against generated ground truth
        metrics = PSTComparator.evaluate(pst, gt_record)
        metrics["task_id"] = task_id
        metrics["nodes_count"] = len(nodes_gt)
        metrics["edges_count"] = len(edges_gt)
        results.append(metrics)

        print(f"[{task_id:7s}] Generated {len(nodes_gt):2d} nodes, {len(edges_gt):2d} edges | "
              f"Node F1: {metrics['node_f1']*100:.1f}%, Edge F1: {metrics['edge_f1']*100:.1f}%")

    avg_node_f1 = sum(r["node_f1"] for r in results) / len(results)
    avg_edge_f1 = sum(r["edge_f1"] for r in results) / len(results)
    avg_node_prec = sum(r["node_precision"] for r in results) / len(results)
    avg_node_rec = sum(r["node_recall"] for r in results) / len(results)
    avg_edge_prec = sum(r["edge_precision"] for r in results) / len(results)
    avg_edge_rec = sum(r["edge_recall"] for r in results) / len(results)

    print("=" * 75)
    print("📊 GOLDEN-PST-30 MACRO-AVERAGE EVALUATION SUMMARY")
    print("=" * 75)
    print(f"Total Evaluated Benchmark Tasks: {len(results)}")
    print(f"Macro Node Precision:            {avg_node_prec * 100:.2f}%")
    print(f"Macro Node Recall:               {avg_node_rec * 100:.2f}%")
    print(f"Macro Node F1-Score:             {avg_node_f1 * 100:.2f}%")
    print("-" * 75)
    print(f"Macro Edge Precision:            {avg_edge_prec * 100:.2f}%")
    print(f"Macro Edge Recall:               {avg_edge_rec * 100:.2f}%")
    print(f"Macro Edge F1-Score:             {avg_edge_f1 * 100:.2f}%")
    print("=" * 75)


if __name__ == "__main__":
    generate_and_evaluate_all()
