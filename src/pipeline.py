"""
PST Pipeline: End-to-End Orchestrator
Executes the five layers from raw C++ source to verified Physics Semantic Tree.
"""

import re
from typing import Dict, Any, Tuple, List, Optional
from .layer1_ast.parser import CppASTParser
from .layer1_ast.openmc_parser import OpenMCASTParser
from .layer1_ast.api_extractor import APIExtractor
from .layer2_context.context_packager import ContextPackager, LocalContextPackage
from .layer3_semantic.mapper import SemanticMapper
from .layer4_pst.builder import PSTBuilder
from .layer4_pst.tree import PhysicsSemanticTree
from .layer5_verification.engine import PSTVerificationEngine, VerificationVerdict
from .layer5_verification.report import ReportFormatter


class PSTPipeline:
    """The unified 5-layer pipeline: C++ / Python Code -> AST -> Context -> Schema -> PST -> Verified Verdict."""

    def __init__(self):
        self.cpp_parser = CppASTParser()
        self.openmc_parser = OpenMCASTParser()
        self.extractor = APIExtractor()
        self.packager = ContextPackager()
        self.mapper = SemanticMapper()
        self.builder = PSTBuilder()
        self.verifier = PSTVerificationEngine()

    def run_code(
        self,
        code_text: str,
        filename: str = "simulation.cc",
        is_variance_reduced: bool = True,
        source_type: Optional[str] = None,
        engine: Optional[str] = None
    ) -> Tuple[PhysicsSemanticTree, VerificationVerdict]:
        # Determine simulation engine backend
        if engine is None:
            if filename.endswith(".py") or "import openmc" in code_text or "openmc." in code_text:
                engine = "openmc"
            else:
                engine = "geant4"

        # Layer 1: AST & Syntax Extraction
        if engine == "openmc":
            ast_dict = self.openmc_parser.parse(code_text, filename=filename)
        else:
            ast_dict = self.cpp_parser.parse(code_text, filename=filename)

        candidates = self.extractor.extract_candidates(ast_dict)

        functions_map = {fn.name: fn for fn in ast_dict.get("functions", [])}

        # Extract file-level domain context hints
        code_lower = code_text.lower()
        file_hints = []
        for kw in [
            "beam", "pencil", "incident", "parallel", "collimat",
            "target", "slab", "shield", "maze", "corridor", "phantom",
            "sobp", "linac", "pbs", "waterphantom", "concreteslab"
        ]:
            if kw in code_lower:
                file_hints.append(kw)

        effective_source_type = source_type
        if effective_source_type is None:
            has_point_source = any(k in code_lower for k in ["point source", "isotrop", "4pi", "点源", "各向同性"])
            if has_point_source:
                effective_source_type = "isotropic"
            elif file_hints:
                effective_source_type = "beam"

        # Layer 2: Local Context Packaging (Depth-1 Expansion)
        packages: List[LocalContextPackage] = []
        node_idx = 1
        for call in candidates["candidate_calls"]:
            pkg = self.packager.package_call(
                call, functions_map, node_idx,
                source_type=effective_source_type,
                file_context_hints=file_hints
            )
            packages.append(pkg)
            node_idx += 1

        for assign in candidates["candidate_accumulations"]:
            pkg = self.packager.package_assign(
                assign, functions_map, node_idx,
                source_type=effective_source_type,
                file_context_hints=file_hints
            )
            packages.append(pkg)
            node_idx += 1

        # Layer 3: Semantic Mapping to Schema
        annotated_items = []
        for pkg in packages:
            sem = self.mapper.map_package(pkg)
            if sem:
                annotated_items.append((pkg, sem))

        # Layer 4: Physics Semantic Tree (PST) Assembly
        pst = self.builder.build(annotated_items, tree_name=f"PST_{filename}", raw_code=code_text)

        # Layer 5: Conservation Rule Verification
        verdict = self.verifier.verify(pst, is_variance_reduced=is_variance_reduced)

        return pst, verdict

    def format_report(self, verdict: VerificationVerdict) -> str:
        return ReportFormatter.format_terminal(verdict)
