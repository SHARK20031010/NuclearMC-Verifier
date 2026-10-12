import json
import pytest
from src.pipeline import PSTPipeline
try:
    from benchmarks.evaluate_pst import PSTComparator
except ImportError:
    from golden_benchmark.evaluate_pst import PSTComparator


def test_golden_evaluator_scores_weight_leak():
    buggy_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        fFlux += 1.0;
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_code, filename="t1_6_buggy.cc")

    gt_data = {
        "nodes": [
            {
                "action": "Accumulate",
                "physical_operation": "accumulate_fluence",
                "expected_invariant": "UNWEIGHTED_COUNT_ACCUMULATION"
            }
        ],
        "edges": [
            {
                "edge_type": "WEIGHT_COUPLING",
                "is_satisfied": False
            },
            {
                "edge_type": "LIFECYCLE_CONTAINER",
                "is_satisfied": True
            }
        ]
    }

    metrics = PSTComparator.evaluate(pst, gt_data)
    assert metrics["node_recall"] == 1.0
    assert metrics["node_f1"] == 1.0
    assert metrics["edge_recall"] == 1.0
    assert metrics["edge_f1"] == 1.0


def test_golden_pst_30_sample_regression():
    from pathlib import Path
    anno_dir = Path(__file__).resolve().parent.parent / "benchmarks" / "sample_annotations"
    if not anno_dir.exists():
        anno_dir = Path(__file__).resolve().parent.parent / "golden_benchmark" / "sample_annotations"
    pipeline = PSTPipeline()

    sample_file = anno_dir / "wild_01.json"
    with open(sample_file, "r", encoding="utf-8") as f:
        gt_data = json.load(f)

    src_path = Path(gt_data.get("source_file", ""))
    if not src_path.exists():
        src_path = Path(__file__).resolve().parent.parent / "benchmarks" / "cases" / "wild_corpus" / "WILD-01" / "code" / "case_buggy.cc"
    if not src_path.exists():
        src_path = Path(__file__).resolve().parent.parent / "benchmark_tasks_reference" / "wild_corpus" / "WILD-01" / "code" / "case_buggy.cc"

    with open(src_path, "r", encoding="utf-8") as f:
        code_text = f.read()

    pst, _ = pipeline.run_code(code_text, filename="WILD-01_buggy.cc")
    metrics = PSTComparator.evaluate(pst, gt_data)

    assert metrics["node_f1"] >= 0.95
    assert metrics["edge_f1"] >= 0.95
