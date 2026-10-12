#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
cli.py —— Physics Semantic Tree (PST) Verification CLI
Command-line driver for the 5-layer AST -> Context -> Schema -> PST -> Rule Verification pipeline.
"""

import sys
import os
import argparse
import json
from pathlib import Path

# Add project root to path
_HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(_HERE))

from src.pipeline import PSTPipeline
try:
    from benchmarks.evaluate_pst import PSTComparator
except ImportError:
    from golden_benchmark.evaluate_pst import PSTComparator


def main():
    parser = argparse.ArgumentParser(
        description="Physics Semantic Tree (PST) Formal Verifier for Monte Carlo Simulation Codes"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # 1. check command
    check_p = subparsers.add_parser("check", help="Run 5-layer PST verification on a C++ file")
    check_p.add_argument("file", help="Path to C++ source file (.cc, .cpp)")
    check_p.add_argument("--json", action="store_true", help="Output machine-readable JSON verdict")
    check_p.add_argument("--no-vr", action="store_true", help="Disable variance reduction weight coupling requirement")

    # 2. mermaid command
    mermaid_p = subparsers.add_parser("mermaid", help="Render the generated PST as a Mermaid graph")
    mermaid_p.add_argument("file", help="Path to C++ source file (.cc, .cpp)")

    # 3. dump-pst command
    dump_p = subparsers.add_parser("dump-pst", help="Dump the full PST graph as JSON")
    dump_p.add_argument("file", help="Path to C++ source file (.cc, .cpp)")

    # 4. eval command
    eval_p = subparsers.add_parser("eval", help="Evaluate generated PST against Ground-Truth annotation")
    eval_p.add_argument("file", help="Path to C++ source file (.cc, .cpp)")
    eval_p.add_argument("gt", help="Path to Ground-Truth JSON file")

    args = parser.parse_args()
    pipeline = PSTPipeline()

    if not os.path.exists(args.file):
        print(f"Error: file not found: {args.file}", file=sys.stderr)
        sys.exit(1)

    with open(args.file, "r", encoding="utf-8") as f:
        code_text = f.read()

    pst, verdict = pipeline.run_code(
        code_text,
        filename=os.path.basename(args.file),
        is_variance_reduced=not getattr(args, "no_vr", False)
    )

    if args.command == "check":
        if args.json:
            print(verdict.model_dump_json(indent=2))
        else:
            print(pipeline.format_report(verdict))
        sys.exit(0 if verdict.is_valid else 1)

    elif args.command == "mermaid":
        print(pst.to_mermaid())

    elif args.command == "dump-pst":
        print(pst.to_json(indent=2))

    elif args.command == "eval":
        if not os.path.exists(args.gt):
            print(f"Error: ground truth file not found: {args.gt}", file=sys.stderr)
            sys.exit(1)
        with open(args.gt, "r", encoding="utf-8") as f:
            gt_data = json.load(f)
        metrics = PSTComparator.evaluate(pst, gt_data)
        print("=" * 60)
        print("📊 GOLDEN-PST ACCURACY BENCHMARK EVALUATION")
        print("=" * 60)
        print(f"Node Precision: {metrics['node_precision'] * 100:.1f}%")
        print(f"Node Recall:    {metrics['node_recall'] * 100:.1f}%")
        print(f"Node F1-Score:  {metrics['node_f1'] * 100:.1f}%")
        print("-" * 60)
        print(f"Edge Precision: {metrics['edge_precision'] * 100:.1f}%")
        print(f"Edge Recall:    {metrics['edge_recall'] * 100:.1f}%")
        print(f"Edge F1-Score:  {metrics['edge_f1'] * 100:.1f}%")
        print("=" * 60)


if __name__ == "__main__":
    main()
