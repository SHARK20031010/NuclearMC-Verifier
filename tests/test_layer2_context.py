import pytest
from src.layer1_ast.parser import CppASTParser
from src.layer2_context.context_packager import ContextPackager


def test_context_packager_captures_guards_and_weights():
    code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        if (track->GetCurrentStepNumber() == 1) {
            fYield++;
        }
        fFlux += step->GetTrack()->GetWeight();
    }
    """
    parser = CppASTParser()
    packager = ContextPackager()
    ast = parser.parse(code)
    fn_map = {fn.name: fn for fn in ast["functions"]}

    packages = []
    for idx, call in enumerate(ast["calls"]):
        packages.append(packager.package_call(call, fn_map, idx))

    for idx, assign in enumerate(ast["assignments"]):
        packages.append(packager.package_assign(assign, fn_map, idx + 10))

    # Check first-step guard detection on fYield++
    yield_pkg = next(p for p in packages if "fyield" in p.target_expression.lower())
    assert yield_pkg.has_first_step_guard is True

    # Check weight multiplication detection on fFlux
    flux_pkg = next(p for p in packages if "fflux" in p.target_expression.lower())
    assert flux_pkg.has_weight_multiplication is True
