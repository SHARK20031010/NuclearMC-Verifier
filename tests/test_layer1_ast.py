import pytest
from src.layer1_ast.parser import CppASTParser
from src.layer1_ast.api_extractor import APIExtractor


def test_ast_parser_extracts_functions_and_calls():
    code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        if (volume == fTarget) {
            G4double edep = step->GetTotalEnergyDeposit();
            fEdep += edep;
        }
    }
    """
    parser = CppASTParser()
    ast = parser.parse(code)

    assert len(ast["functions"]) == 1
    assert ast["functions"][0].name == "UserSteppingAction"
    assert len(ast["calls"]) >= 1
    assert any(c.callee == "GetTotalEnergyDeposit" for c in ast["calls"])
    assert len(ast["assignments"]) >= 1
    assert any(a.target == "fEdep" for a in ast["assignments"])


def test_api_extractor_identifies_mc_entities():
    code = """
    void GeneratePrimaries(G4Event* event) {
        gun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    }
    """
    parser = CppASTParser()
    extractor = APIExtractor()
    ast = parser.parse(code)
    candidates = extractor.extract_candidates(ast)

    assert len(candidates["candidate_calls"]) >= 1
    assert candidates["candidate_calls"][0].callee == "SetParticleMomentumDirection"
