import pytest
from src.layer1_ast.parser import CppASTParser
from src.layer2_context.context_packager import ContextPackager
from src.layer3_semantic.mapper import SemanticMapper
from src.layer3_semantic.schema import PhysicalSemanticAnnotation


def test_semantic_mapper_outputs_strict_schema():
    code = """
    void UserSteppingAction(const G4Step* step) {
        G4double w = step->GetTrack()->GetWeight();
        fFlux += w;
    }
    """
    parser = CppASTParser()
    packager = ContextPackager()
    mapper = SemanticMapper()

    ast = parser.parse(code)
    fn_map = {fn.name: fn for fn in ast["functions"]}

    for call in ast["calls"]:
        pkg = packager.package_call(call, fn_map, 1)
        sem = mapper.map_package(pkg)
        if sem and "getweight" in call.callee.lower():
            assert isinstance(sem, PhysicalSemanticAnnotation)
            assert sem.action == "Fetch"
            assert sem.physical_quantity == "track_weight"
            assert sem.conservation_domain == "variance_weight"
