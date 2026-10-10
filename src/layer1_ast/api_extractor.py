"""
Layer 1: Monte Carlo API Entity Extractor
Filters and categorizes raw AST CallExpr and AssignExpr nodes into candidate MC actions.
"""

from typing import List, Dict, Any
from .parser import CallExpr, AssignExpr, FunctionDecl


MC_CORE_APIS = {
    # Partition 1: Stepping & Tracking Fetch APIs
    "GetTotalEnergyDeposit": "Fetch:TotalEnergyDeposit",
    "GetNonIonizingEnergyDeposit": "Fetch:NonIonizingEnergyDeposit",
    "GetStepLength": "Fetch:StepLength",
    "GetWeight": "Fetch:TrackWeight",
    "SetWeight": "State:SetWeight",
    "GetGlobalTime": "Fetch:GlobalTime",
    "GetLocalTime": "Fetch:LocalTime",
    "GetProperTime": "Fetch:ProperTime",
    "GetCurrentStepNumber": "Fetch:StepNumber",
    "GetParentID": "Fetch:ParentID",
    "GetTrackID": "Fetch:TrackID",
    "GetPostStepPoint": "Fetch:PostStepPoint",
    "GetPreStepPoint": "Fetch:PreStepPoint",
    "GetStepStatus": "Fetch:StepStatus",
    "GetTrackStatus": "Fetch:TrackStatus",
    "SetTrackStatus": "State:SetTrackStatus",
    "GetCreatorProcess": "Fetch:CreatorProcess",
    "GetCreatorModelID": "Fetch:CreatorModelID",
    "GetCreatorModelName": "Fetch:CreatorModelName",
    "GetKineticEnergy": "Fetch:KineticEnergy",
    "GetTotalEnergy": "Fetch:TotalEnergy",
    "GetMomentumDirection": "Fetch:MomentumDirection",
    "GetMomentum": "Fetch:MomentumVector",
    "GetVelocity": "Fetch:Velocity",
    "GetPhysicalVolume": "Fetch:PhysicalVolume",
    "GetNextVolume": "Fetch:NextVolume",
    "GetMaterial": "Fetch:Material",
    "GetNextMaterial": "Fetch:NextMaterial",
    "GetTouchable": "Fetch:Touchable",
    "GetNextTouchable": "Fetch:NextTouchable",
    "GetOriginTouchable": "Fetch:OriginTouchable",
    "GetProcessDefinedStep": "Fetch:ProcessDefinedStep",
    "GetVertexPosition": "Fetch:VertexPosition",
    "GetVertexMomentumDirection": "Fetch:VertexMomentumDirection",
    "GetVertexKineticEnergy": "Fetch:VertexKineticEnergy",
    "GetLogicalVolumeAtVertex": "Fetch:LogicalVolumeAtVertex",
    "IsFirstStepInVolume": "Predicate:IsFirstStepInVolume",
    "IsLastStepInVolume": "Predicate:IsLastStepInVolume",
    "GetNumberOfSecondariesInCurrentStep": "Fetch:SecondaryCountInStep",
    "GetSecondaryInCurrentStep": "Fetch:SecondaryVectorInStep",
    "GetSecondary": "Fetch:CumulativeSecondaryVector",
    "UserSteppingAction": "Hook:UserSteppingAction",
    "PreUserTrackingAction": "Hook:PreUserTrackingAction",
    "PostUserTrackingAction": "Hook:PostUserTrackingAction",

    # Partition 2: Sampling & Primary Source APIs
    "G4RandomDirection": "Sample:IsotropicDirection",
    "G4UniformRand": "Sample:UniformStochastic",
    "SetParticleMomentumDirection": "Sample:DirectionAssignment",
    "SetParticleMomentum": "Sample:MomentumAssignment",
    "SetParticleEnergy": "Sample:EnergyAssignment",
    "SetParticlePosition": "Sample:PositionAssignment",
    "SetParticleTime": "Sample:TimeAssignment",
    "SetParticleWeight": "Sample:WeightAssignment",
    "SetParticleCharge": "Define:ChargeAssignment",
    "SetParticlePolarization": "Sample:PolarizationAssignment",
    "SetNumberOfParticles": "Define:NumberOfParticles",
    "GeneratePrimaryVertex": "Sample:VertexGeneration",
    "GeneratePrimaries": "Hook:GeneratePrimaries",
    "GetIonTable": "Fetch:IonTable",
    "GetIon": "Define:IonDefinition",
    "FindParticle": "Fetch:ParticleLookup",
    "SetPosDisType": "Define:SPSPosType",
    "SetPosDisShape": "Define:SPSPosShape",
    "SetCentreCoords": "Define:SPSCentreCoords",
    "SetAngDistType": "Define:SPSAngType",
    "SetEnergyDisType": "Define:SPSEneType",
    "SetMonoEnergy": "Define:SPSMonoEnergy",

    # Partition 5: Scoring, Tallies, Sensitive Detectors & Analysis APIs
    "FillH1": "Accumulate:HistogramFill",
    "CreateH1": "Define:Histogram1D",
    "CreateNtuple": "Define:Ntuple",
    "FillNtupleDColumn": "Accumulate:NtupleFill",
    "FillNtupleIColumn": "Accumulate:NtupleFill",
    "AddNtupleRow": "Accumulate:NtupleRow",
    "OpenFile": "IO:OpenFile",
    "CloseFile": "IO:CloseFile",
    "AddEdep": "Accumulate:EnergyDeposit",
    "ProcessHits": "Hook:ProcessHits",
    "EndOfEventAction": "Hook:EndOfEventAction",
    "BeginOfEventAction": "Hook:BeginOfEventAction",
    "EndOfRunAction": "Hook:EndOfRunAction",
    "BeginOfRunAction": "Hook:BeginOfRunAction",
    "RegisterPrimitive": "Define:RegisterPrimitiveScorer",
    "G4PSEnergyDeposit": "Define:ScorerEnergyDeposit",
    "G4PSDoseDeposit": "Define:ScorerDoseDeposit",
    "G4PSCellFlux": "Define:ScorerCellFlux",
    "GetNumberOfEvent": "Fetch:NumberOfEvents",

    # Partition 3: Geometry & Material Composition APIs
    "G4Box": "Define:SolidBox",
    "G4Tubs": "Define:SolidTubs",
    "G4Sphere": "Define:SolidSphere",
    "G4SubtractionSolid": "Define:SolidSubtraction",
    "G4LogicalVolume": "Define:LogicalVolume",
    "G4PVPlacement": "Define:GeometryPlacement",
    "G4PVReplica": "Define:GeometryReplica",
    "CheckOverlaps": "Predicate:CheckOverlaps",
    "GetMass": "Fetch:VolumeMass",
    "SetSensitiveDetector": "Define:AssignSensitiveDetector",
    "AddElement": "Define:AddElement",
    "AddMaterial": "Define:AddMaterial",
    "AddIsotope": "Define:AddIsotope",
    "FindOrBuildMaterial": "Define:NistMaterial",
    "FindOrBuildElement": "Define:NistElement",
    "Construct": "Hook:ConstructGeometry",
    "ConstructSDandField": "Hook:ConstructSDandField",

    # Partition 4: Physics Constructor & Production Cut APIs
    "RegisterPhysics": "Define:PhysicsConstructor",
    "ReplacePhysics": "Define:PhysicsReplace",
    "SetDefaultCutValue": "Define:ProductionCut",
    "SetCutsWithDefault": "Define:ProductionCutDefault",
    "SetProductionCut": "Define:ProductionCut",
    "SetProductionCuts": "Define:ProductionCutsVector",
    "SetUserLimits": "Define:AssignUserLimits",
    "SetMaxAllowedStep": "Define:StepLimit",
    "SetUserMinEkine": "Define:EnergyLimit",
    "SetUserMaxTime": "Define:TimeLimit",
    "G4EmStandardPhysics": "Define:PhysicsEM",
    "G4EmStandardPhysics_option4": "Define:PhysicsEMOption4",
    "G4RadioactiveDecayPhysics": "Define:PhysicsRadioactiveDecay",
    "G4DecayPhysics": "Define:PhysicsDecay",
    "G4HadronPhysicsQGSP_BIC_HP": "Define:PhysicsHadronHP",
    "G4HadronPhysicsFTFP_BERT": "Define:PhysicsHadron",
    "G4HadronElasticPhysics": "Define:PhysicsHadronElastic",
    "G4HadronElasticPhysicsHP": "Define:PhysicsHadronElastic",
    "G4PhotonuclearPhysics": "Define:PhysicsPhotonuclear",
    "G4EmExtraPhysics": "Define:PhysicsEmExtra",
    "SetBirksConstant": "State:SetBirksConstant",
    "AddConstProperty": "Define:MaterialProperty",
    "AddProperty": "Define:MaterialProperty",

    # Complementary Subsystems: Run Driving, Stacking, Extended Solids, Fields
    "CreateRunManager": "Define:RunManager",
    "BeamOn": "Execute:BeamOn",
    "Initialize": "Execute:RunManagerInitialize",
    "SetUserInitialization": "Define:SetUserInitialization",
    "SetUserAction": "Define:SetUserAction",
    "Build": "Hook:ActionInitBuild",
    "BuildForMaster": "Hook:ActionInitBuildForMaster",
    "ClassifyNewTrack": "Hook:ClassifyNewTrack",
    "G4Cons": "Define:SolidCons",
    "G4Polycone": "Define:SolidPolycone",
    "G4UnionSolid": "Define:SolidUnion",
    "G4IntersectionSolid": "Define:SolidIntersection",
    "G4UniformMagField": "Define:MagneticField",
    "G4ChordFinder": "Define:ChordFinder",
    "SetDeltaChord": "State:SetDeltaChord",
    "G4OpticalPhysics": "Define:PhysicsOptical",
    "G4OpticalSurface": "Define:OpticalSurface",
    "G4LogicalSkinSurface": "Define:OpticalSkinSurface",
    "G4LogicalBorderSurface": "Define:OpticalBorderSurface",

    # Partition 6: OpenMC Python APIs
    "openmc.Source": "Sample:SourceDefinition",
    "openmc.IndependentSource": "Sample:SourceDefinition",
    "openmc.stats.Isotropic": "Sample:IsotropicDirection",
    "openmc.stats.Monodirectional": "Sample:DirectionAssignment",
    "openmc.stats.Point": "Sample:PositionAssignment",
    "openmc.stats.Discrete": "Sample:EnergyAssignment",
    "openmc.Tally": "Define:TallyDefinition",
    "openmc.Material": "Define:MaterialDefinition",
    "openmc.WeightWindows": "Define:WeightWindows",
    "openmc.run": "Execute:RunExecution",
    "Source": "Sample:SourceDefinition",
    "IndependentSource": "Sample:SourceDefinition",
    "Isotropic": "Sample:IsotropicDirection",
    "Monodirectional": "Sample:DirectionAssignment",
    "Point": "Sample:PositionAssignment",
    "Discrete": "Sample:EnergyAssignment",
    "Tally": "Define:TallyDefinition",
    "Material": "Define:MaterialDefinition",
    "WeightWindows": "Define:WeightWindows",
    "set_density": "Define:DensityAssignment",
    "add_element": "Define:ElementAssignment"
}


class APIExtractor:
    """Filters AST nodes into high-interest Monte Carlo candidates."""

    def __init__(self):
        self.known_apis = MC_CORE_APIS

    def extract_candidates(self, ast_dict: Dict[str, Any]) -> Dict[str, List[Any]]:
        calls: List[CallExpr] = ast_dict.get("calls", [])
        assigns: List[AssignExpr] = ast_dict.get("assignments", [])
        functions: List[FunctionDecl] = ast_dict.get("functions", [])

        tally_assigns = []
        for a in assigns:
            target_lower = a.target.lower()
            expr_lower = a.expression.lower()
            is_compound = a.operator in {"+=", "-=", "++", "--"}
            is_openmc_prop = (
                any(k in target_lower for k in ["angle", "scores", "filters", "space", "energy", "tally", "material"])
                or any(k in expr_lower for k in ["openmc", "monodirectional", "isotropic", "point", "discrete", "tally", "weightwindows"])
            )

            if not is_compound and not is_openmc_prop:
                continue

            is_stepping = "stepping" in a.enclosing_function.lower() or "step" in a.enclosing_function.lower()
            is_tally_name = any(k in target_lower for k in ["edep", "flux", "count", "yield", "tally", "dose", "trans", "pass", "escape", "hit", "angle", "scores", "filters", "space", "energy"])
            if is_stepping or is_tally_name or is_openmc_prop:
                tally_assigns.append(a)

        matched_calls = []
        for call in calls:
            # Skip calls that are already captured by an OpenMC property assignment on the same line
            if any(a.span.line_start == call.span.line_start and a.span.file == call.span.file for a in tally_assigns):
                continue
            if (call.callee in self.known_apis
                or (call.caller and any(k in call.caller.lower() for k in ["gun", "step", "track", "openmc", "source", "tally"]))):
                matched_calls.append(call)

        return {
            "candidate_calls": matched_calls,
            "candidate_accumulations": tally_assigns
        }
