import pytest
from src.pipeline import PSTPipeline


def test_pipeline_detects_unweighted_variance_reduction_flaw():
    buggy_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        // Buggy: accumulates without multiplying weight under geometric splitting!
        fFlux += 1.0;
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_code, filename="t1_6_buggy.cc")

    assert verdict.is_valid is False
    assert len(verdict.violations) >= 1
    assert any(v.rule_id == "PHYS-INV-WEIGHT-01" for v in verdict.violations)


def test_pipeline_detects_collapsed_pencil_beam_flaw():
    buggy_code = """
    void GeneratePrimaries(G4Event* event) {
        // Buggy: point source collapsed to fixed pencil beam
        gun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_code, filename="pencil_beam_buggy.cc")

    assert verdict.is_valid is False
    assert any(v.rule_id == "PHYS-INV-MEASURE-01" for v in verdict.violations)


def test_pipeline_passes_compliant_code():
    compliant_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        G4double w = step->GetTrack()->GetWeight();
        fFlux += w;
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(compliant_code, filename="compliant.cc")

    assert verdict.is_valid is True
    assert len(verdict.violations) == 0


def test_pipeline_detects_uncompensated_stacking_kill():
    buggy_code = """
    G4ClassificationOfNewTrack MyStackingAction::ClassifyNewTrack(const G4Track* aTrack) {
        if (aTrack->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
            if (G4UniformRand() < 0.5) return fKill; // Buggy: uncompensated killing
        }
        return fUrgent;
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_code, filename="stacking_buggy.cc")

    assert verdict.is_valid is False
    assert any(v.rule_id == "PHYS-INV-STACK-01" for v in verdict.violations)


def test_pipeline_detects_master_worker_action_violation():
    buggy_code = """
    void MyActionInitialization::BuildForMaster() const {
        SetUserAction(new MyRunAction());
        SetUserAction(new MySteppingAction()); // Buggy: SteppingAction attached to Master thread!
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_code, filename="action_init_buggy.cc")

    assert verdict.is_valid is False
    assert any(v.rule_id == "PHYS-INV-MT-01" for v in verdict.violations)


def test_pipeline_detects_uninitialized_beamon():
    buggy_code = """
    int main(int argc, char** argv) {
        auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
        runManager->SetUserInitialization(new MyDetectorConstruction());
        // Buggy: missing runManager->Initialize();
        runManager->BeamOn(1000);
        return 0;
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_code, filename="main_buggy.cc")

    assert verdict.is_valid is False
    assert any(v.rule_id == "PHYS-INV-RUN-01" for v in verdict.violations)


def test_pipeline_detects_unnormalized_absorbed_dose():
    buggy_code = """
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        G4double edep = step->GetTotalEnergyDeposit();
        // Buggy: absorbed dose accumulated as energy without dividing by mass
        fDose += edep;
    }
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_code, filename="dose_buggy.cc")

    assert verdict.is_valid is False
    assert any(v.rule_id == "PHYS-INV-DOSE-01" for v in verdict.violations)
