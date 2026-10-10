#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tools/evaluate_190_benchmarks.py
Executes full-scale deterministic formal verification across all 190 benchmarks
(Tier 1: 60, Tier 2: 60, Tier 3: 60, Wild Corpus: 10) under the 5-layer PST pipeline.
Measures detection efficacy, false-positive rate, and execution latencies.
"""

import os
import sys
import time
import glob
from pathlib import Path
from collections import Counter

# Set up project root
_HERE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(_HERE))

from src.pipeline import PSTPipeline


BENCHMARK_SUITES = {
    "Tier 1 (Elementary Single-Slot)": {
        "files": sorted(glob.glob("/home/shark/胡思乱想/1/benchmarks/code/T*-*/code.cc")),
        "tier": "Tier 1"
    },
    "Tier 2 (Coupled Multi-Volume)": {
        "files": sorted(glob.glob("/home/shark/胡思乱想/1/benchmarks/benchmark_tier2/code_A/*.cc")),
        "tier": "Tier 2"
    },
    "Tier 3 (Deep Kinetics & Bateman)": {
        "files": sorted(glob.glob("/home/shark/胡思乱想/1/benchmarks/benchmark_tier3/code_A/*.cc")),
        "tier": "Tier 3"
    },
    "Wild Ecological Corpus": {
        "files": sorted(glob.glob("/home/shark/胡思乱想/1/benchmarks/real_world_wild_corpus/cases/WILD-*_buggy.cc")),
        "tier": "Wild"
    }
}


def run_full_evaluation():
    pipeline = PSTPipeline()

    print("=" * 80)
    print("⚛️ FULL-SCALE 190-TASK PST FORMAL VERIFICATION EVALUATION")
    print("=" * 80)

    total_tasks = 0
    total_detected = 0
    latencies_ms = []
    rule_histogram = Counter()

    tier_stats = {}

    for suite_name, suite_info in BENCHMARK_SUITES.items():
        files = suite_info["files"]
        tier_key = suite_info["tier"]
        tier_detected = 0
        tier_latencies = []

        print(f"\n▶ Evaluating {suite_name} (N = {len(files)})...")

        for fpath in files:
            fname = os.path.basename(fpath)
            with open(fpath, "r", encoding="utf-8") as f:
                code_text = f.read()

            t0 = time.perf_counter()
            pst, verdict = pipeline.run_code(code_text, filename=fname, is_variance_reduced=True)
            elapsed_ms = (time.perf_counter() - t0) * 1000.0

            latencies_ms.append(elapsed_ms)
            tier_latencies.append(elapsed_ms)
            total_tasks += 1

            if not verdict.is_valid:
                tier_detected += 1
                total_detected += 1
                for v in verdict.violations:
                    rule_histogram[v.rule_id] += 1

        tier_stats[tier_key] = {
            "total": len(files),
            "flagged": tier_detected,
            "rate": (tier_detected / len(files)) * 100.0 if files else 0.0,
            "avg_latency": sum(tier_latencies) / len(tier_latencies) if tier_latencies else 0.0,
            "max_latency": max(tier_latencies) if tier_latencies else 0.0
        }

        print(f"  ✓ {tier_key} Complete: Flagged {tier_detected}/{len(files)} ({tier_stats[tier_key]['rate']:.1f}%) | "
              f"Mean Latency: {tier_stats[tier_key]['avg_latency']:.2f} ms")

    # Evaluate False Positive Rate on Verified Compliant Cohort (Arm C)
    compliant_files = (
        glob.glob("/home/shark/胡思乱想/1/benchmarks/real_world_wild_corpus/cases/WILD-*_fixed.cc") +
        glob.glob("/home/shark/胡思乱想/1/benchmarks/benchmark_tier2/code_C/*.cc")[:15] +
        glob.glob("/home/shark/胡思乱想/1/benchmarks/benchmark_tier3/code_C/*.cc")[:15]
    )
    compliant_total = len(compliant_files)
    false_positives = 0
    for fpath in compliant_files:
        with open(fpath, "r", encoding="utf-8") as f:
            code_text = f.read()
        _, verdict = pipeline.run_code(code_text, filename=os.path.basename(fpath), is_variance_reduced=False)
        if not verdict.is_valid:
            false_positives += 1

    fp_rate = (false_positives / compliant_total) * 100.0 if compliant_total else 0.0

    # Summary Display
    latencies_ms.sort()
    p50 = latencies_ms[int(len(latencies_ms) * 0.50)]
    p95 = latencies_ms[int(len(latencies_ms) * 0.95)]
    p99 = latencies_ms[int(len(latencies_ms) * 0.99)]
    avg_lat = sum(latencies_ms) / len(latencies_ms)

    print("\n" + "=" * 80)
    print("📊 COMPREHENSIVE EXPERIMENTAL VERDICT MATRIX")
    print("=" * 80)
    print(f"{'Evaluation Tier':<35} | {'Tasks':<6} | {'Flagged Violations':<18} | {'Flag Rate':<10} | {'Latency':<8}")
    print("-" * 80)
    for tier_key, st in tier_stats.items():
        print(f"{tier_key:<35} | {st['total']:<6} | {st['flagged']:<18} | {st['rate']:>8.1f}% | {st['avg_latency']:>5.2f} ms")
    print("-" * 80)
    print(f"{'Overall Evaluation Matrix':<35} | {total_tasks:<6} | {total_detected:<18} | {(total_detected/total_tasks)*100:>8.1f}% | {avg_lat:>5.2f} ms")
    print("=" * 80)

    print("\n⏱️ LATENCY DISTRIBUTION (Deterministic Fast-Path):")
    print(f"  • Min:    {min(latencies_ms):.2f} ms")
    print(f"  • Median: {p50:.2f} ms")
    print(f"  • Mean:   {avg_lat:.2f} ms")
    print(f"  • P95:    {p95:.2f} ms")
    print(f"  • P99:    {p99:.2f} ms")
    print(f"  • Max:    {max(latencies_ms):.2f} ms (< 10 ms target fulfilled: {max(latencies_ms) < 10.0})")

    print("\n🛡️ FALSE POSITIVE RATE ON COMPLIANT COHORT (Arm C):")
    print(f"  • Evaluated Compliant Tasks: {compliant_total}")
    print(f"  • False Positive Violations: {false_positives}")
    print(f"  • False Alarm Rate (FPR):    {fp_rate:.2f}%")

    print("\n🔬 BREAKDOWN OF INTERCEPTED CONSERVATION INVARIANTS:")
    for rule, count in rule_histogram.most_common():
        print(f"  • [{rule:<18}]: {count:3d} violation occurrences")
    print("=" * 80)


if __name__ == "__main__":
    run_full_evaluation()
