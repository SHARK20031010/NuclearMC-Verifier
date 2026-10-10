"""
Layer 2: Local Context Packager (Depth-1 Expansion)
Mitigates Pitfall 1 (Context Insensitivity).
Enriches isolated API nodes with surrounding control flow, variables, and stepping states.
"""

import re
from typing import List, Dict, Any, Optional
from pydantic import BaseModel, Field
from ..layer1_ast.parser import CallExpr, AssignExpr, FunctionDecl, ASTSpan


class LocalContextPackage(BaseModel):
    node_id: str
    target_expression: str
    target_type: str                     # "CallExpr" or "AssignExpr"
    span: ASTSpan
    enclosing_function_name: str
    enclosing_function_body: str
    enclosing_guards: List[str] = Field(default_factory=list)
    has_boundary_guard: bool = False
    has_first_step_guard: bool = False
    has_volume_filter: bool = False
    has_weight_multiplication: bool = False
    referenced_variables: List[str] = Field(default_factory=list)
    resolved_symbols: Dict[str, str] = Field(default_factory=dict)
    source_type: Optional[str] = None
    file_context_hints: List[str] = Field(default_factory=list)


class ContextPackager:
    """Constructs Depth-1 Local Context Packages for candidate AST nodes."""

    def __init__(self):
        pass

    @staticmethod
    def extract_local_symbols(fn_body: str) -> Dict[str, str]:
        symbols: Dict[str, str] = {}
        if not fn_body:
            return symbols

        lines = fn_body.splitlines()
        for line in lines:
            line_clean = line.strip()
            if not line_clean or line_clean.startswith("//"):
                continue

            # 1. Constructor syntax: Type var_name(args);
            m_ctor = re.match(
                r"^\s*(?:const\s+)?([A-Za-z0-9_:]+)\s+([A-Za-z0-9_]+)\s*\(([^;{]+)\)\s*;\s*$",
                line_clean
            )
            if m_ctor:
                type_name = m_ctor.group(1)
                var_name = m_ctor.group(2)
                args = m_ctor.group(3).strip()
                if type_name not in {"if", "for", "while", "switch", "catch", "return"}:
                    symbols[var_name] = f"{type_name}({args})"
                    continue

            # 2. Assignment declaration syntax: [Type] var_name = expr;
            m_decl = re.match(
                r"^\s*(?:const\s+)?(?:auto|G4[A-Za-z0-9_]+|[A-Za-z0-9_:]+)[\*&]?\s+([A-Za-z0-9_]+)\s*=\s*([^;]+);\s*$",
                line_clean
            )
            if m_decl:
                var_name = m_decl.group(1)
                expr = m_decl.group(2).strip()
                symbols[var_name] = expr
                continue

            # 3. Plain assignment: var_name = expr;
            m_assign = re.match(
                r"^\s*([A-Za-z0-9_]+)\s*=\s*([^;]+);\s*$",
                line_clean
            )
            if m_assign:
                var_name = m_assign.group(1)
                expr = m_assign.group(2).strip()
                symbols[var_name] = expr
                continue

        # Transitive propagation
        for _ in range(5):
            updated = False
            for k, v in list(symbols.items()):
                for other_k, other_v in symbols.items():
                    if k != other_k and re.search(r"\b" + re.escape(other_k) + r"\b", v):
                        new_v = re.sub(r"\b" + re.escape(other_k) + r"\b", other_v, v)
                        if new_v != v:
                            symbols[k] = new_v
                            updated = True
            if not updated:
                break

        return symbols

    def package_call(
        self,
        call: CallExpr,
        functions_map: Dict[str, FunctionDecl],
        node_idx: int,
        source_type: Optional[str] = None,
        file_context_hints: Optional[List[str]] = None
    ) -> LocalContextPackage:
        fn = functions_map.get(call.enclosing_function, None)
        fn_body = fn.body if fn else ""
        guards = call.enclosing_conditions

        symbols = self.extract_local_symbols(fn_body)
        raw_expr = call.span.raw_text
        resolved_expr = raw_expr

        for sym_k, sym_v in symbols.items():
            if any(re.search(r"\b" + re.escape(sym_k) + r"\b", arg) for arg in call.arguments):
                resolved_expr += f" /* resolved: {sym_k}={sym_v} */"

        if call.caller:
            for sym_k, sym_v in symbols.items():
                if re.search(r"\b" + re.escape(sym_k) + r"\b", call.caller):
                    resolved_expr += f" /* resolved: {sym_k}={sym_v} */"

        guards_text = " ".join(guards).lower()
        expr_text = resolved_expr.lower()

        has_boundary = "boundary" in guards_text or "fgeomboundary" in guards_text or "getstepstatus" in guards_text
        has_first_step = "getcurrentstepnumber() == 1" in guards_text or "stepnumber == 1" in guards_text or "step->getcurrentstepnumber() == 1" in guards_text
        has_volume = "getphysicalvolume" in guards_text or "volume" in guards_text or "logical" in guards_text
        has_weight = "getweight" in expr_text or "weight" in expr_text

        return LocalContextPackage(
            node_id=f"ctx_call_{node_idx:03d}",
            target_expression=resolved_expr,
            target_type="CallExpr",
            span=call.span,
            enclosing_function_name=call.enclosing_function,
            enclosing_function_body=fn_body[:2000],  # depth-1 snippet
            enclosing_guards=guards,
            has_boundary_guard=has_boundary,
            has_first_step_guard=has_first_step,
            has_volume_filter=has_volume,
            has_weight_multiplication=has_weight,
            referenced_variables=call.arguments,
            resolved_symbols=symbols,
            source_type=source_type,
            file_context_hints=file_context_hints or []
        )

    def package_assign(
        self,
        assign: AssignExpr,
        functions_map: Dict[str, FunctionDecl],
        node_idx: int,
        source_type: Optional[str] = None,
        file_context_hints: Optional[List[str]] = None
    ) -> LocalContextPackage:
        fn = functions_map.get(assign.enclosing_function, None)
        fn_body = fn.body if fn else ""
        guards = assign.enclosing_conditions

        symbols = self.extract_local_symbols(fn_body)
        raw_expr = f"{assign.target} {assign.operator} {assign.expression};"
        resolved_expr = raw_expr

        for sym_k, sym_v in symbols.items():
            if re.search(r"\b" + re.escape(sym_k) + r"\b", assign.expression):
                resolved_expr += f" /* resolved: {sym_k}={sym_v} */"

        guards_text = " ".join(guards).lower()
        expr_text = resolved_expr.lower()

        has_boundary = "boundary" in guards_text or "fgeomboundary" in guards_text or "getstepstatus" in guards_text
        has_first_step = "getcurrentstepnumber() == 1" in guards_text or "stepnumber == 1" in guards_text or "step->getcurrentstepnumber() == 1" in guards_text
        has_volume = "getphysicalvolume" in guards_text or "volume" in guards_text or "logical" in guards_text
        has_weight = "getweight" in expr_text or "weight" in expr_text or "w" in expr_text.split("*")

        return LocalContextPackage(
            node_id=f"ctx_assign_{node_idx:03d}",
            target_expression=resolved_expr,
            target_type="AssignExpr",
            span=assign.span,
            enclosing_function_name=assign.enclosing_function,
            enclosing_function_body=fn_body[:2000],
            enclosing_guards=guards,
            has_boundary_guard=has_boundary,
            has_first_step_guard=has_first_step,
            has_volume_filter=has_volume,
            has_weight_multiplication=has_weight,
            referenced_variables=[assign.target],
            resolved_symbols=symbols,
            source_type=source_type,
            file_context_hints=file_context_hints or []
        )
