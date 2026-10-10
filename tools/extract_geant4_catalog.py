#!/usr/bin/env python3
"""
Exhaustive Geant4 11.2.2 User-Facing API Extractor & Catalog Generator
Parses installed Geant4 C++ headers directly from disk and generates structured partition documentation.
"""

import os
import re
from typing import List, Dict, Any

GEANT4_INCLUDE_DIR = "/home/shark/geant4/11.2.2-install/include/Geant4"

PARTITIONS = {
    "partition_1": {
        "title": "Partition 1: Stepping, Tracking & Trajectory Lifecycle APIs",
        "description": "Lifecycle APIs for particle tracking, step-level evaluation, state inspection, and stepping/tracking hooks.",
        "headers": [
            ("G4Step.hh", "G4Step"),
            ("G4StepPoint.hh", "G4StepPoint"),
            ("G4Track.hh", "G4Track"),
            ("G4StepStatus.hh", "G4StepStatus"),
            ("G4TrackStatus.hh", "G4TrackStatus"),
            ("G4UserSteppingAction.hh", "G4UserSteppingAction"),
            ("G4UserTrackingAction.hh", "G4UserTrackingAction"),
            ("G4VProcess.hh", "G4VProcess")
        ]
    },
    "partition_2": {
        "title": "Partition 2: Primary Generator & Phase Space Source APIs",
        "description": "Particle guns, General Particle Source (GPS), phase-space distributions (energy, angular, spatial), particle definitions and random generators.",
        "headers": [
            ("G4VUserPrimaryGeneratorAction.hh", "G4VUserPrimaryGeneratorAction"),
            ("G4ParticleGun.hh", "G4ParticleGun"),
            ("G4GeneralParticleSource.hh", "G4GeneralParticleSource"),
            ("G4SingleParticleSource.hh", "G4SingleParticleSource"),
            ("G4SPSPosDistribution.hh", "G4SPSPosDistribution"),
            ("G4SPSAngDistribution.hh", "G4SPSAngDistribution"),
            ("G4SPSEneDistribution.hh", "G4SPSEneDistribution"),
            ("G4ParticleTable.hh", "G4ParticleTable"),
            ("G4ParticleDefinition.hh", "G4ParticleDefinition"),
            ("G4IonTable.hh", "G4IonTable"),
            ("G4PrimaryVertex.hh", "G4PrimaryVertex"),
            ("G4PrimaryParticle.hh", "G4PrimaryParticle"),
            ("G4RandomDirection.hh", "G4RandomDirection")
        ]
    },
    "partition_3": {
        "title": "Partition 3: Geometry & Material Composition APIs",
        "description": "Solids, logical volumes, physical placements, materials, elements, isotopes, and NIST manager.",
        "headers": [
            ("G4VUserDetectorConstruction.hh", "G4VUserDetectorConstruction"),
            ("G4LogicalVolume.hh", "G4LogicalVolume"),
            ("G4VPhysicalVolume.hh", "G4VPhysicalVolume"),
            ("G4PVPlacement.hh", "G4PVPlacement"),
            ("G4PVReplica.hh", "G4PVReplica"),
            ("G4Box.hh", "G4Box"),
            ("G4Tubs.hh", "G4Tubs"),
            ("G4Sphere.hh", "G4Sphere"),
            ("G4SubtractionSolid.hh", "G4SubtractionSolid"),
            ("G4Material.hh", "G4Material"),
            ("G4Element.hh", "G4Element"),
            ("G4Isotope.hh", "G4Isotope"),
            ("G4NistManager.hh", "G4NistManager"),
            ("G4ThreeVector.hh", "G4ThreeVector"),
            ("G4RotationMatrix.hh", "G4RotationMatrix")
        ]
    },
    "partition_4": {
        "title": "Partition 4: Physics Lists & Process Construction APIs",
        "description": "Modular physics lists, physics constructors, EM/hadron/decay packages, production cuts, and user limits.",
        "headers": [
            ("G4VUserPhysicsList.hh", "G4VUserPhysicsList"),
            ("G4VModularPhysicsList.hh", "G4VModularPhysicsList"),
            ("G4VPhysicsConstructor.hh", "G4VPhysicsConstructor"),
            ("G4EmStandardPhysics.hh", "G4EmStandardPhysics"),
            ("G4EmStandardPhysics_option4.hh", "G4EmStandardPhysics_option4"),
            ("G4DecayPhysics.hh", "G4DecayPhysics"),
            ("G4RadioactiveDecayPhysics.hh", "G4RadioactiveDecayPhysics"),
            ("G4HadronPhysicsQGSP_BIC_HP.hh", "G4HadronPhysicsQGSP_BIC_HP"),
            ("G4HadronPhysicsFTFP_BERT.hh", "G4HadronPhysicsFTFP_BERT"),
            ("G4ProductionCuts.hh", "G4ProductionCuts"),
            ("G4UserLimits.hh", "G4UserLimits")
        ]
    },
    "partition_5": {
        "title": "Partition 5: Scoring, Tallies, Sensitive Detectors & Analysis APIs",
        "description": "Sensitive detectors, multi-functional detectors, primitive scorers, filters, SD manager, and G4AnalysisManager.",
        "headers": [
            ("G4VSensitiveDetector.hh", "G4VSensitiveDetector"),
            ("G4MultiFunctionalDetector.hh", "G4MultiFunctionalDetector"),
            ("G4VPrimitiveScorer.hh", "G4VPrimitiveScorer"),
            ("G4PSEnergyDeposit.hh", "G4PSEnergyDeposit"),
            ("G4PSDoseDeposit.hh", "G4PSDoseDeposit"),
            ("G4PSCellFlux.hh", "G4PSCellFlux"),
            ("G4PSPassageCellFlux.hh", "G4PSPassageCellFlux"),
            ("G4PSTrackLength.hh", "G4PSTrackLength"),
            ("G4PSNofSecondary.hh", "G4PSNofSecondary"),
            ("G4SDManager.hh", "G4SDManager"),
            ("G4SDParticleFilter.hh", "G4SDParticleFilter"),
            ("G4SDKineticEnergyFilter.hh", "G4SDKineticEnergyFilter"),
            ("G4VAnalysisManager.hh", "G4VAnalysisManager"),
            ("G4AnalysisManager.hh", "G4AnalysisManager"),
            ("G4UserRunAction.hh", "G4UserRunAction"),
            ("G4UserEventAction.hh", "G4UserEventAction"),
            ("G4Run.hh", "G4Run"),
            ("G4Event.hh", "G4Event")
        ]
    }
}


def parse_header_methods(filepath: str, class_or_enum: str) -> Dict[str, Any]:
    if not os.path.exists(filepath):
        return {"type": "missing", "name": class_or_enum, "items": []}

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Check if it is an enum
    enum_match = re.search(r"enum(?:\s+class)?\s+" + re.escape(class_or_enum) + r"\s*\{([^}]+)\}", content)
    if enum_match:
        raw_items = enum_match.group(1).split(",")
        enum_values = []
        for it in raw_items:
            it_clean = re.sub(r"//.*", "", it).strip()
            if it_clean:
                enum_values.append(it_clean.split("=")[0].strip())
        return {"type": "enum", "name": class_or_enum, "items": enum_values}

    # Find class declaration
    cls_idx = content.find(f"class {class_or_enum}")
    if cls_idx == -1:
        cls_idx = content.find(f"struct {class_or_enum}")
    if cls_idx == -1:
        # Check global functions (e.g. G4RandomDirection)
        funcs = []
        for line in content.split("\n"):
            line_s = line.strip()
            if class_or_enum in line_s and "(" in line_s and ")" in line_s and not line_s.startswith("//"):
                funcs.append({"sig": line_s.rstrip(";"), "doc": ""})
        return {"type": "functions", "name": class_or_enum, "items": funcs}

    sub = content[cls_idx:]
    pub_idx = sub.find("public:")
    if pub_idx == -1:
        # structs are public by default
        if sub.startswith("struct"):
            pub_idx = sub.find("{")
        else:
            return {"type": "class", "name": class_or_enum, "items": []}

    # Stop at next protected or private or class end
    prot_idx = sub.find("protected:", pub_idx)
    priv_idx = sub.find("private:", pub_idx)
    ends = [x for x in [prot_idx, priv_idx] if x != -1]
    end_idx = min(ends) if ends else len(sub)

    public_block = sub[pub_idx:end_idx]

    lines = public_block.split("\n")
    methods = []
    curr = ""
    comment = ""
    for line in lines:
        s = line.strip()
        if s.startswith("//"):
            comment += " " + s[2:].strip()
            continue
        if not s or s.startswith("#") or s == "public:":
            continue
        curr += " " + s
        if ";" in curr or "{" in curr:
            curr = curr.split("{")[0].strip()
            if curr.endswith(";"):
                curr = curr[:-1].strip()
            if "(" in curr and ")" in curr:
                clean_sig = re.sub(r"\s+", " ", curr).strip()
                # filter out macros / typedefs
                if not clean_sig.startswith("typedef") and not clean_sig.startswith("using"):
                    methods.append({"sig": clean_sig, "doc": comment.strip()})
            curr = ""
            comment = ""

    return {"type": "class", "name": class_or_enum, "items": methods}


def generate_partition_markdown(part_id: str, part_data: Dict[str, Any], output_path: str):
    md = [f"# {part_data['title']}\n"]
    md.append(f"> **Partition Scope**: {part_data['description']}\n")
    md.append(f"> **Header Base Path**: `{GEANT4_INCLUDE_DIR}`\n")
    md.append("---\n")

    for header_name, target_name in part_data["headers"]:
        header_path = os.path.join(GEANT4_INCLUDE_DIR, header_name)
        data = parse_header_methods(header_path, target_name)

        md.append(f"## Header: `{header_name}` (Target: `{target_name}`)\n")
        if data["type"] == "enum":
            md.append(f"**Kind**: Enumeration (`enum {target_name}`)\n")
            md.append("| Enumerator | Description / Semantic Role |")
            md.append("|---|---|")
            for val in data["items"]:
                md.append(f"| `{val}` | State flag in tracking / stepping engine |")
            md.append("\n")
        elif data["type"] == "class":
            md.append(f"**Kind**: C++ Class (`{target_name}`) | **Total Extracted Public Methods**: {len(data['items'])}\n")
            md.append("| C++ Method Signature | Comment / Description |")
            md.append("|---|---|")
            for m in data["items"]:
                doc = m["doc"].replace("|", "\\|") if m["doc"] else "-"
                sig = m["sig"].replace("|", "\\|")
                md.append(f"| `{sig}` | {doc} |")
            md.append("\n")
        elif data["type"] == "functions":
            md.append(f"**Kind**: Global Utilities / Functions\n")
            md.append("| Signature | Description |")
            md.append("|---|---|")
            for m in data["items"]:
                md.append(f"| `{m['sig']}` | Global helper function |")
            md.append("\n")
        else:
            md.append(f"*Header `{header_name}` not found or empty.*\n")

    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(md))


def main():
    docs_dir = "/home/shark/胡思乱想/05_物理语义树形式化核验_PST/docs/api_catalog"
    os.makedirs(docs_dir, exist_ok=True)

    summary_md = ["# Geant4 11.2.2 User-Facing API Complete Catalog Index\n"]
    summary_md.append("This directory contains the ground-truth extracted C++ user-facing API catalogs for Geant4 11.2.2, systematically partitioned into 5 orthogonal physical domains.\n")
    summary_md.append("| Partition | Domain Description | Catalog Doc | Classes / Enums Inspected |")
    summary_md.append("|---|---|---|---|")

    for p_id, p_info in PARTITIONS.items():
        fname = f"{p_id.upper()}_{p_info['title'].split(':')[1].strip().replace(' ', '_').replace(',', '').replace('&', 'AND').upper()}.md"
        out_file = os.path.join(docs_dir, fname)
        generate_partition_markdown(p_id, p_info, out_file)

        targets = ", ".join([f"`{t}`" for _, t in p_info["headers"]])
        summary_md.append(f"| `{p_id}` | {p_info['title']} | [{fname}]({fname}) | {targets} |")
        print(f"Generated {fname} ({len(p_info['headers'])} headers)")

    summary_file = os.path.join(docs_dir, "INDEX.md")
    with open(summary_file, "w", encoding="utf-8") as f:
        f.write("\n".join(summary_md))
    print("Catalog generation complete!")


if __name__ == "__main__":
    main()
