"""
Layer 5: Verification Report Formatter
Renders terminal and JSON diagnostics for human developers and LLM reflection loops.
"""

from typing import Dict, Any
from .engine import VerificationVerdict


class ReportFormatter:
    """Formats verification verdicts into human-readable and structured outputs."""

    @staticmethod
    def format_terminal(verdict: VerificationVerdict) -> str:
        lines = []
        lines.append("=" * 70)
        lines.append("⚛️ PHYSICS SEMANTIC TREE (PST) FORMAL VERIFICATION REPORT")
        lines.append("=" * 70)
        lines.append(f"Graph Status: {verdict.node_count} Physical Nodes | {verdict.edge_count} Conservation Edges")
        lines.append("-" * 70)

        if verdict.is_valid:
            lines.append("✅ VERIFICATION PASSED: No conservation invariant violations detected.")
            lines.append("   All physical flows, measures, and weights are mathematically closed.")
        else:
            lines.append(f"🚨 VERIFICATION FAILED: Intercepted {len(verdict.violations)} Physical Violations:\n")
            for idx, v in enumerate(verdict.violations, 1):
                lines.append(f"[{idx}] Rule: {v.rule_id} ({v.severity}) · {v.invariant_name}")
                lines.append(f"    - Target Node: {v.violating_node_id}")
                lines.append(f"    - Code Evidence: `{v.code_evidence}`")
                lines.append(f"    - Defect Diagnosis: {v.diagnosis}")
                lines.append(f"    - Prescribed Remediation: {v.remediation}\n")

        lines.append("=" * 70)
        return "\n".join(lines)

    @staticmethod
    def format_json(verdict: VerificationVerdict) -> str:
        return verdict.model_dump_json(indent=2)
