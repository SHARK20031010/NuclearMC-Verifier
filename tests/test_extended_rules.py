import pytest
from src.pipeline import PSTPipeline

pipeline = PSTPipeline()

def test_hadron_missing_elastic():
    buggy_code = """
    // Neutron transmission through concrete shield
    #include "G4RunManager.hh"
    #include "G4VModularPhysicsList.hh"
    #include "G4EmStandardPhysics_option4.hh"
    #include "G4HadronPhysicsQGSP_BIC_HP.hh"
    class MyPhys : public G4VModularPhysicsList {
    public:
        MyPhys() {
            RegisterPhysics(new G4EmStandardPhysics_option4());
            RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
        }
    };
    """
    fixed_code = """
    // Neutron transmission through concrete shield
    #include "G4RunManager.hh"
    #include "G4VModularPhysicsList.hh"
    #include "G4EmStandardPhysics_option4.hh"
    #include "G4HadronPhysicsQGSP_BIC_HP.hh"
    #include "G4HadronElasticPhysicsHP.hh"
    class MyPhys : public G4VModularPhysicsList {
    public:
        MyPhys() {
            RegisterPhysics(new G4EmStandardPhysics_option4());
            RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());
            RegisterPhysics(new G4HadronElasticPhysicsHP());
        }
    };
    """
    _, v_buggy = pipeline.run_code(buggy_code, "test_hadron.cc")
    _, v_fixed = pipeline.run_code(fixed_code, "test_hadron.cc")
    
    assert any(v.rule_id == "PHYS-INV-HADRON-01" for v in v_buggy.violations)
    assert not any(v.rule_id == "PHYS-INV-HADRON-01" for v in v_fixed.violations)

def test_boundary_step_point():
    buggy_code = """
    #include "G4UserSteppingAction.hh"
    #include "G4Step.hh"
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        auto pre = step->GetPreStepPoint();
        if (pre->GetStepStatus() != fGeomBoundary) return;
        fTransmitted++;
    }
    """
    fixed_code = """
    #include "G4UserSteppingAction.hh"
    #include "G4Step.hh"
    void MySteppingAction::UserSteppingAction(const G4Step* step) {
        auto post = step->GetPostStepPoint();
        if (post->GetStepStatus() != fGeomBoundary) return;
        fTransmitted++;
    }
    """
    _, v_buggy = pipeline.run_code(buggy_code, "test_boundary.cc")
    _, v_fixed = pipeline.run_code(fixed_code, "test_boundary.cc")
    
    assert any(v.rule_id == "PHYS-INV-BOUNDARY-01" for v in v_buggy.violations)
    assert not any(v.rule_id == "PHYS-INV-BOUNDARY-01" for v in v_fixed.violations)

def test_scintillation_birks():
    buggy_code = """
    #include "G4MaterialPropertiesTable.hh"
    void SetupMaterial() {
        auto mpt = new G4MaterialPropertiesTable();
        mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV);
        mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.0 * ns);
    }
    """
    fixed_code = """
    #include "G4MaterialPropertiesTable.hh"
    #include "G4Material.hh"
    void SetupMaterial(G4Material* mat) {
        auto mpt = new G4MaterialPropertiesTable();
        mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. / MeV);
        mat->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);
    }
    """
    _, v_buggy = pipeline.run_code(buggy_code, "test_scint.cc")
    _, v_fixed = pipeline.run_code(fixed_code, "test_scint.cc")
    
    assert any(v.rule_id == "PHYS-INV-BIRKS-01" for v in v_buggy.violations)
    assert not any(v.rule_id == "PHYS-INV-BIRKS-01" for v in v_fixed.violations)

def test_event_container_reset():
    buggy_code = """
    #include <set>
    static std::set<int> gSeenTrackIDs;
    void Step(int trackID) {
        if (!gSeenTrackIDs.insert(trackID).second) return;
        fYield++;
    }
    """
    fixed_code = """
    #include <set>
    static std::set<int> gSeenTrackIDs;
    void BeginOfEventAction() {
        gSeenTrackIDs.clear();
    }
    void Step(int trackID) {
        if (!gSeenTrackIDs.insert(trackID).second) return;
        fYield++;
    }
    """
    _, v_buggy = pipeline.run_code(buggy_code, "test_reset.cc")
    _, v_fixed = pipeline.run_code(fixed_code, "test_reset.cc")
    
    assert any(v.rule_id == "PHYS-INV-LIFECYCLE-RESET-01" for v in v_buggy.violations)
    assert not any(v.rule_id == "PHYS-INV-LIFECYCLE-RESET-01" for v in v_fixed.violations)

def test_radial_measure_sampling():
    buggy_code = """
    double R = 5.0 * cm;
    double r = R * G4UniformRand();
    """
    fixed_code = """
    double R = 5.0 * cm;
    double r = R * std::sqrt(G4UniformRand());
    """
    _, v_buggy = pipeline.run_code(buggy_code, "test_radial.cc")
    _, v_fixed = pipeline.run_code(fixed_code, "test_radial.cc")
    
    assert any(v.rule_id == "PHYS-INV-MEASURE-02" for v in v_buggy.violations)
    assert not any(v.rule_id == "PHYS-INV-MEASURE-02" for v in v_fixed.violations)

def test_energy_resolution_fwhm_scaling():
    buggy_code = """
    double fwhm = 0.07 * 662 * keV;
    double sampled_edep = G4RandGauss::shoot(662*keV, fwhm);
    """
    fixed_code = """
    double fwhm = 0.07 * 662 * keV;
    double sigma = fwhm / 2.35482;
    double sampled_edep = G4RandGauss::shoot(662*keV, sigma);
    """
    _, v_buggy = pipeline.run_code(buggy_code, "test_fwhm.cc")
    _, v_fixed = pipeline.run_code(fixed_code, "test_fwhm.cc")
    
    assert any(v.rule_id == "PHYS-INV-GAUSS-01" for v in v_buggy.violations)
    assert not any(v.rule_id == "PHYS-INV-GAUSS-01" for v in v_fixed.violations)
