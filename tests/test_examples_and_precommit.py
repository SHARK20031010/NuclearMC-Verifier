import os
from pathlib import Path
import yaml
import pytest
from guardrail.check import verify_file_joint


def test_pre_commit_hooks_yaml_validity():
    root = Path(__file__).resolve().parents[1]
    cfg_path = root / ".pre-commit-hooks.yaml"
    assert cfg_path.exists(), ".pre-commit-hooks.yaml must exist in root"
    
    with open(cfg_path, "r", encoding="utf-8") as f:
        hooks = yaml.safe_load(f)
    
    assert isinstance(hooks, list), "hooks must be a YAML list"
    assert len(hooks) >= 1
    
    hook_ids = [h["id"] for h in hooks]
    assert "nuclearmc-verifier" in hook_ids
    
    main_hook = next(h for h in hooks if h["id"] == "nuclearmc-verifier")
    assert main_hook["language"] == "python"
    assert "mc-verifier check" in main_hook["entry"]


def test_examples_exist_and_fail_verification():
    root = Path(__file__).resolve().parents[1]
    examples_dir = root / "examples"
    
    cases = [
        ("01_boundary_leak.cc", ["PST-INV-LIFECYCLE-01"]),
        ("02_unweighted_variance_reduction.cc", ["PST-INV-VARIANCE-01"]),
        ("03_scintillator_without_birks.cc", ["PHYS-INV-BIRKS-01", "SCINTILLATOR_BIRKS_QUENCHING_CONTRACT"]),
    ]
    
    for filename, expected_rules in cases:
        file_path = examples_dir / filename
        assert file_path.exists(), f"Example {filename} must exist"
        
        report = verify_file_joint(file_path)
        assert not report["is_valid"], f"{filename} should fail verification due to physical flaw"
        
        detected_rule_ids = set()
        for pst_v in report.get("pst", {}).get("violations", []):
            detected_rule_ids.add(pst_v.get("rule_id"))
        for rule_d in report.get("rules", {}).get("defects", []):
            detected_rule_ids.add(rule_d.get("contract") or rule_d.get("rule_id"))
            
        matched = any(exp in detected_rule_ids for exp in expected_rules)
        assert matched, f"{filename} did not trigger expected rules {expected_rules}, got {detected_rule_ids}"
