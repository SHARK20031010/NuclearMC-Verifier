import pytest
from src.pipeline import PSTPipeline


def test_openmc_detects_collapsed_pencil_beam_flaw():
    buggy_openmc_code = """
    import openmc

    mat = openmc.Material(name="shield")
    mat.set_density("g/cm3", 2.3)

    # Buggy: isotropic point source collapsed to fixed monodirectional pencil beam
    source = openmc.Source()
    source.space = openmc.stats.Point((0.0, 0.0, 0.0))
    source.angle = openmc.stats.Monodirectional(reference_uvw=[0.0, 0.0, 1.0])

    tally = openmc.Tally(name="flux_tally")
    tally.scores = ["flux"]
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(buggy_openmc_code, filename="openmc_buggy.py", is_variance_reduced=False)

    assert verdict.is_valid is False
    assert len(verdict.violations) >= 1
    assert any(v.rule_id == "PHYS-INV-MEASURE-01" for v in verdict.violations)


def test_openmc_passes_compliant_isotropic_source():
    compliant_openmc_code = """
    import openmc

    mat = openmc.Material(name="shield")
    mat.set_density("g/cm3", 2.3)

    # Compliant: 4pi spherical isotropic sampling
    source = openmc.Source()
    source.space = openmc.stats.Point((0.0, 0.0, 0.0))
    source.angle = openmc.stats.Isotropic()

    tally = openmc.Tally(name="flux_tally")
    tally.scores = ["flux"]
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(compliant_openmc_code, filename="openmc_compliant.py", is_variance_reduced=False)

    assert verdict.is_valid is True
    assert len(verdict.violations) == 0


def test_openmc_passes_compliant_collimated_beam():
    beam_openmc_code = """
    import openmc

    # Incident accelerator proton beam hitting concrete slab target
    source = openmc.Source()
    source.space = openmc.stats.Point((0.0, 0.0, -10.0))
    source.angle = openmc.stats.Monodirectional(reference_uvw=[0.0, 0.0, 1.0])

    tally = openmc.Tally(name="transmission_flux")
    tally.scores = ["flux"]
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(beam_openmc_code, filename="openmc_beam.py", is_variance_reduced=False)

    # Collimated beam on target geometry should not trigger collapsed point source alarm
    assert not any(v.rule_id == "PHYS-INV-MEASURE-01" for v in verdict.violations)


def test_openmc_detects_unweighted_variance_reduction():
    unweighted_vr_code = """
    import openmc

    # Deep penetration shielding with geometric splitting
    source = openmc.Source()
    source.space = openmc.stats.Point((0.0, 0.0, 0.0))
    source.angle = openmc.stats.Isotropic()

    # Unweighted tally under variance reduction splitting
    tally = openmc.Tally(name="leakage_flux")
    tally.scores = ["flux"]
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(unweighted_vr_code, filename="openmc_vr_buggy.py", is_variance_reduced=True)

    assert verdict.is_valid is False
    assert any(v.rule_id == "PHYS-INV-WEIGHT-01" for v in verdict.violations)


def test_openmc_pst_graph_integrity():
    model_code = """
    import openmc

    mat = openmc.Material(name="water")
    mat.set_density("g/cm3", 1.0)

    source = openmc.Source()
    source.space = openmc.stats.Point((0.0, 0.0, 0.0))
    source.angle = openmc.stats.Isotropic()

    tally = openmc.Tally(name="my_tally")
    tally.scores = ["flux"]
    """
    pipeline = PSTPipeline()
    pst, verdict = pipeline.run_code(model_code, filename="model.py", is_variance_reduced=False)

    actions = {n.action for n in pst.nodes_dict.values()}
    assert "Define" in actions
    assert "Sample" in actions
    assert "Accumulate" in actions
    assert len(pst.nodes_dict) >= 3
