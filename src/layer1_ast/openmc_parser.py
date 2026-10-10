"""
Layer 1: OpenMC Python AST Parser Frontend
Parses OpenMC Python simulation scripts into standardized FunctionDecl, CallExpr, and AssignExpr nodes.
"""

import ast
import textwrap
from typing import List, Dict, Any, Optional
from .parser import ASTSpan, CallExpr, AssignExpr, FunctionDecl


class OpenMCASTParser:
    """Lightweight Python AST extractor for OpenMC simulation scripts."""

    def __init__(self):
        pass

    def parse(self, code_text: str, filename: str = "model.py") -> Dict[str, Any]:
        clean_code = textwrap.dedent(code_text)
        lines = clean_code.splitlines()
        try:
            tree = ast.parse(clean_code, filename=filename)
        except SyntaxError:
            return {"functions": [], "calls": [], "assignments": []}

        functions: List[FunctionDecl] = []
        calls: List[CallExpr] = []
        assigns: List[AssignExpr] = []

        # Top-level global module scope
        global_fn = FunctionDecl(
            name="global_scope",
            qualified_name="global_scope",
            return_type="void",
            parameters=[],
            body=code_text,
            span=ASTSpan(file=filename, line_start=1, line_end=max(1, len(lines)), raw_text=code_text[:300])
        )
        functions.append(global_fn)

        class Visitor(ast.NodeVisitor):
            def __init__(self):
                self.current_function = "global_scope"
                self.current_conditions: List[str] = []

            def visit_FunctionDef(self, node: ast.FunctionDef):
                prev_fn = self.current_function
                self.current_function = node.name
                fn_start = node.lineno
                fn_end = getattr(node, "end_lineno", fn_start)
                fn_body = "\n".join(lines[fn_start - 1:fn_end]) if lines else ast.unparse(node)
                functions.append(FunctionDecl(
                    name=node.name,
                    qualified_name=node.name,
                    return_type="void",
                    parameters=[a.arg for a in node.args.args],
                    body=fn_body,
                    span=ASTSpan(file=filename, line_start=fn_start, line_end=fn_end, raw_text=fn_body[:300])
                ))
                self.generic_visit(node)
                self.current_function = prev_fn

            def visit_If(self, node: ast.If):
                test_str = ast.unparse(node.test)
                self.current_conditions.append(test_str)
                for stmt in node.body:
                    self.visit(stmt)
                self.current_conditions.pop()
                for stmt in node.orelse:
                    self.visit(stmt)

            def visit_Assign(self, node: ast.Assign):
                line_start = node.lineno
                line_end = getattr(node, "end_lineno", line_start)
                raw_text = "\n".join(lines[line_start - 1:line_end]) if lines else ast.unparse(node)
                expr_str = ast.unparse(node.value)

                for tgt in node.targets:
                    tgt_str = ast.unparse(tgt)
                    assigns.append(AssignExpr(
                        target=tgt_str,
                        operator="=",
                        expression=expr_str,
                        span=ASTSpan(file=filename, line_start=line_start, line_end=line_end, raw_text=raw_text.strip()),
                        enclosing_function=self.current_function,
                        enclosing_conditions=list(self.current_conditions)
                    ))
                self.generic_visit(node)

            def visit_AugAssign(self, node: ast.AugAssign):
                line_start = node.lineno
                line_end = getattr(node, "end_lineno", line_start)
                raw_text = "\n".join(lines[line_start - 1:line_end]) if lines else ast.unparse(node)
                expr_str = ast.unparse(node.value)
                tgt_str = ast.unparse(node.target)
                op_map = {
                    ast.Add: "+=",
                    ast.Sub: "-=",
                    ast.Mult: "*=",
                    ast.Div: "/="
                }
                op_str = op_map.get(type(node.op), "+=")
                assigns.append(AssignExpr(
                    target=tgt_str,
                    operator=op_str,
                    expression=expr_str,
                    span=ASTSpan(file=filename, line_start=line_start, line_end=line_end, raw_text=raw_text.strip()),
                    enclosing_function=self.current_function,
                    enclosing_conditions=list(self.current_conditions)
                ))
                self.generic_visit(node)

            def visit_Call(self, node: ast.Call):
                line_start = node.lineno
                line_end = getattr(node, "end_lineno", line_start)
                raw_text = "\n".join(lines[line_start - 1:line_end]) if lines else ast.unparse(node)

                func_str = ast.unparse(node.func)
                if "." in func_str:
                    parts = func_str.rsplit(".", 1)
                    caller = parts[0]
                    callee = parts[1]
                else:
                    caller = None
                    callee = func_str

                args_str = [ast.unparse(a) for a in node.args] + [f"{kw.arg}={ast.unparse(kw.value)}" for kw in node.keywords]

                calls.append(CallExpr(
                    caller=caller,
                    callee=callee,
                    arguments=args_str,
                    span=ASTSpan(file=filename, line_start=line_start, line_end=line_end, raw_text=raw_text.strip()),
                    enclosing_function=self.current_function,
                    enclosing_conditions=list(self.current_conditions)
                ))
                self.generic_visit(node)

        visitor = Visitor()
        visitor.visit(tree)

        return {
            "functions": functions,
            "calls": calls,
            "assignments": assigns
        }
