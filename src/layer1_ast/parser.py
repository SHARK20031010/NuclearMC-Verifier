"""
Layer 1: AST & C++ Syntax Parsing Frontend
Extracts functions, call expressions, variable declarations, and assignments from C++ code.
"""

import re
from typing import List, Dict, Any, Optional
from pydantic import BaseModel, Field


class ASTSpan(BaseModel):
    file: str = "source.cc"
    line_start: int
    line_end: int
    raw_text: str


class CallExpr(BaseModel):
    caller: Optional[str] = None          # e.g., 'step->GetTrack()'
    callee: str                          # e.g., 'GetWeight'
    arguments: List[str] = Field(default_factory=list)
    span: ASTSpan
    enclosing_function: str = ""
    enclosing_conditions: List[str] = Field(default_factory=list)


class AssignExpr(BaseModel):
    target: str                          # e.g., 'fEdep'
    operator: str                        # e.g., '+=', '='
    expression: str                      # e.g., 'step->GetTotalEnergyDeposit()'
    span: ASTSpan
    enclosing_function: str = ""
    enclosing_conditions: List[str] = Field(default_factory=list)


class FunctionDecl(BaseModel):
    name: str                            # e.g., 'UserSteppingAction'
    qualified_name: str = ""             # e.g., 'MySteppingAction::UserSteppingAction'
    return_type: str                     # e.g., 'void'
    parameters: List[str] = Field(default_factory=list)
    body: str
    span: ASTSpan


class CppASTParser:
    """Lightweight C++ syntax extractor for Monte Carlo simulation source code."""

    def __init__(self):
        pass

    def strip_comments(self, code: str) -> str:
        # Preserve newlines to maintain line numbers
        def repl_multiline(m):
            return "\n" * m.group(0).count("\n")
        code = re.sub(r"/\*.*?\*/", repl_multiline, code, flags=re.S)
        code = re.sub(r"//[^\n]*", "", code)
        return code

    def parse(self, code: str, filename: str = "source.cc") -> Dict[str, Any]:
        clean_code = self.strip_comments(code)
        lines = code.splitlines()

        functions = self._extract_functions(clean_code, lines, filename)
        calls = []
        assigns = []

        for fn in functions:
            fn_calls, fn_assigns = self._extract_function_internals(fn, filename)
            calls.extend(fn_calls)
            assigns.extend(fn_assigns)

        # Also extract global or fallback calls if no explicit class method was bound
        if not functions:
            fallback_fn = FunctionDecl(
                name="global_scope",
                return_type="void",
                body=clean_code,
                span=ASTSpan(file=filename, line_start=1, line_end=len(lines), raw_text=clean_code[:200])
            )
            fn_calls, fn_assigns = self._extract_function_internals(fallback_fn, filename)
            calls.extend(fn_calls)
            assigns.extend(fn_assigns)

        return {
            "functions": functions,
            "calls": calls,
            "assignments": assigns
        }

    def _extract_functions(self, clean_code: str, raw_lines: List[str], filename: str) -> List[FunctionDecl]:
        functions = []
        # Pattern to match methods like void MySteppingAction::UserSteppingAction(const G4Step* step) override { ... }
        fn_pattern = re.compile(
            r"(\b[A-Za-z0-9_<>\*&:]+\b)\s+([A-Za-z0-9_:]+)\s*\(([^)]*)\)[^{;]*\{",
            re.M
        )
        for m in fn_pattern.finditer(clean_code):
            ret_type = m.group(1).strip()
            fn_name = m.group(2).strip()
            params = [p.strip() for p in m.group(3).split(",") if p.strip()]
            start_pos = m.start()

            # Find matching closing brace
            brace_count = 0
            open_idx = clean_code.find("{", m.start())
            if open_idx == -1:
                continue
            
            end_pos = open_idx
            for idx in range(open_idx, len(clean_code)):
                char = clean_code[idx]
                if char == "{":
                    brace_count += 1
                elif char == "}":
                    brace_count -= 1
                    if brace_count == 0:
                        end_pos = idx + 1
                        break
            
            body = clean_code[open_idx + 1: end_pos - 1]
            line_start = clean_code[:start_pos].count("\n") + 1
            line_end = clean_code[:end_pos].count("\n") + 1

            base_name = fn_name.split("::")[-1]
            functions.append(FunctionDecl(
                name=base_name,
                qualified_name=fn_name,
                return_type=ret_type,
                parameters=params,
                body=body,
                span=ASTSpan(
                    file=filename,
                    line_start=line_start,
                    line_end=line_end,
                    raw_text="\n".join(raw_lines[line_start-1:line_end])
                )
            ))
        return functions

    def _extract_function_internals(self, fn: FunctionDecl, filename: str) -> tuple:
        calls = []
        assigns = []

        fn_lines = fn.body.splitlines()
        current_conditions = []

        # Line-by-line scanning within function body
        base_line = fn.span.line_start

        for i, line in enumerate(fn_lines):
            line_no = base_line + i
            trimmed = line.strip()
            if not trimmed:
                continue

            # Track basic condition scopes
            if trimmed.startswith("if ") or trimmed.startswith("if("):
                current_conditions.append(trimmed)

            # Match assignments: target += expr, target = expr
            assign_match = re.search(r"([A-Za-z0-9_]+)\s*(\+=|-=|\*=|\/=|=(?!=))\s*([^;]+);", trimmed)
            inc_match = re.search(r"([A-Za-z0-9_]+)\s*(\+\+|--)\s*;", trimmed)
            if assign_match:
                tgt = assign_match.group(1).strip()
                op = assign_match.group(2).strip()
                expr = assign_match.group(3).strip()
                assigns.append(AssignExpr(
                    target=tgt,
                    operator=op,
                    expression=expr,
                    span=ASTSpan(file=filename, line_start=line_no, line_end=line_no, raw_text=trimmed),
                    enclosing_function=fn.name,
                    enclosing_conditions=list(current_conditions)
                ))
            elif inc_match:
                tgt = inc_match.group(1).strip()
                op = inc_match.group(2).strip()
                assigns.append(AssignExpr(
                    target=tgt,
                    operator=op,
                    expression="1",
                    span=ASTSpan(file=filename, line_start=line_no, line_end=line_no, raw_text=trimmed),
                    enclosing_function=fn.name,
                    enclosing_conditions=list(current_conditions)
                ))

            # Match function/method calls: [caller->|caller.]method(args)
            line_for_calls = re.sub(r"\b(if|while|for|switch|catch)\s*\(", "  ( ", trimmed)
            call_matches = re.finditer(r"(?:([A-Za-z0-9_]+(?:\s*(?:->|\.)\s*[A-Za-z0-9_]+)*)\s*(?:->|\.))?\s*([A-Za-z0-9_]+)\s*\(([^)]*)\)", line_for_calls)
            for cm in call_matches:
                caller = cm.group(1)
                callee = cm.group(2)
                args = [a.strip() for a in cm.group(3).split(",") if a.strip()]

                if callee in {"if", "for", "while", "switch"}:
                    continue

                calls.append(CallExpr(
                    caller=caller,
                    callee=callee,
                    arguments=args,
                    span=ASTSpan(file=filename, line_start=line_no, line_end=line_no, raw_text=trimmed),
                    enclosing_function=fn.name,
                    enclosing_conditions=list(current_conditions)
                ))

                # Check if arguments contain nested call
                raw_arg_str = cm.group(3)
                nested_matches = re.finditer(r"(?:([A-Za-z0-9_]+(?:\s*(?:->|\.)\s*[A-Za-z0-9_]+)*)\s*(?:->|\.))?\s*([A-Za-z0-9_]+)\s*\(([^)]*)\)", raw_arg_str)
                for nm in nested_matches:
                    n_caller = nm.group(1)
                    n_callee = nm.group(2)
                    if n_callee not in {"if", "for", "while", "switch"}:
                        calls.append(CallExpr(
                            caller=n_caller,
                            callee=n_callee,
                            arguments=[a.strip() for a in nm.group(3).split(",") if a.strip()],
                            span=ASTSpan(file=filename, line_start=line_no, line_end=line_no, raw_text=trimmed),
                            enclosing_function=fn.name,
                            enclosing_conditions=list(current_conditions)
                        ))

        return calls, assigns
