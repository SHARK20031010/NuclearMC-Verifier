#!/usr/bin/env python3
"""
Extractor for Complementary Geant4 Subsystems:
1. Top-Level Run Management & Action Initialization
2. Stacking & Priority Management
3. Custom Hits & HitsCollection Readout
4. Extended Solids, EM Field, Optical Physics & Biasing
"""

import os
import re
from typing import List, Dict, Any

GEANT4_INCLUDE_DIR = "/home/shark/geant4/11.2.2-install/include/Geant4"

COMPLEMENTARY_HEADERS = [
    # Subsystem 1: Run Management & Action Initialization
    ("G4RunManagerFactory.hh", "G4RunManagerFactory"),
    ("G4RunManager.hh", "G4RunManager"),
    ("G4MTRunManager.hh", "G4MTRunManager"),
    ("G4VUserActionInitialization.hh", "G4VUserActionInitialization"),

    # Subsystem 2: Stacking & Track Classification
    ("G4UserStackingAction.hh", "G4UserStackingAction"),
    ("G4ClassificationOfNewTrack.hh", "G4ClassificationOfNewTrack"),

    # Subsystem 3: Custom Hit & HitsCollection
    ("G4VHit.hh", "G4VHit"),
    ("G4HCofThisEvent.hh", "G4HCofThisEvent"),

    # Subsystem 4: Extended Geometry Solids
    ("G4Cons.hh", "G4Cons"),
    ("G4Polycone.hh", "G4Polycone"),
    ("G4UnionSolid.hh", "G4UnionSolid"),
    ("G4IntersectionSolid.hh", "G4IntersectionSolid"),

    # Subsystem 5: Electromagnetic Field
    ("G4FieldManager.hh", "G4FieldManager"),
    ("G4UniformMagField.hh", "G4UniformMagField"),

    # Subsystem 6: Optical Physics & Biasing
    ("G4OpticalPhysics.hh", "G4OpticalPhysics"),
    ("G4OpticalParameters.hh", "G4OpticalParameters"),
    ("G4OpticalSurface.hh", "G4OpticalSurface"),
    ("G4LogicalBorderSurface.hh", "G4LogicalBorderSurface"),
    ("G4GeometrySampler.hh", "G4GeometrySampler"),
    ("G4WeightWindowStore.hh", "G4WeightWindowStore")
]


def parse_header_methods(filepath: str, class_or_enum: str) -> Dict[str, Any]:
    if not os.path.exists(filepath):
        return {"type": "missing", "name": class_or_enum, "items": []}

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    enum_match = re.search(r"enum(?:\s+class)?\s+" + re.escape(class_or_enum) + r"\s*\{([^}]+)\}", content)
    if enum_match:
        raw_items = enum_match.group(1).split(",")
        enum_values = []
        for it in raw_items:
            it_clean = re.sub(r"//.*", "", it).strip()
            if it_clean:
                enum_values.append(it_clean.split("=")[0].strip())
        return {"type": "enum", "name": class_or_enum, "items": enum_values}

    cls_idx = content.find(f"class {class_or_enum}")
    if cls_idx == -1:
        cls_idx = content.find(f"struct {class_or_enum}")
    if cls_idx == -1:
        return {"type": "class", "name": class_or_enum, "items": []}

    sub = content[cls_idx:]
    pub_idx = sub.find("public:")
    if pub_idx == -1:
        if sub.startswith("struct"):
            pub_idx = sub.find("{")
        else:
            return {"type": "class", "name": class_or_enum, "items": []}

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
                if not clean_sig.startswith("typedef") and not clean_sig.startswith("using"):
                    methods.append({"sig": clean_sig, "doc": comment.strip()})
            curr = ""
            comment = ""

    return {"type": "class", "name": class_or_enum, "items": methods}


def main():
    out_dir = "/home/shark/胡思乱想/05_物理语义树形式化核验_PST/docs/api_catalog"
    out_file = os.path.join(out_dir, "COMPLEMENTARY_SUBSYSTEMS_AND_RUN_MANAGEMENT.md")

    md = ["# Complementary Subsystems & Run Management APIs\n"]
    md.append("> **Scope**: Deep-dive inspection of top-level run driving, modern MT action assembly, stacking classification, custom Hits, extended solids, magnetic fields, optical physics, and variance reduction.\n")
    md.append(f"> **Header Base Path**: `{GEANT4_INCLUDE_DIR}`\n")
    md.append("---\n")

    for header_name, target_name in COMPLEMENTARY_HEADERS:
        header_path = os.path.join(GEANT4_INCLUDE_DIR, header_name)
        data = parse_header_methods(header_path, target_name)

        md.append(f"## Header: `{header_name}` (Target: `{target_name}`)\n")
        if data["type"] == "enum":
            md.append(f"**Kind**: Enumeration (`enum {target_name}`)\n")
            md.append("| Enumerator | Description / Semantic Role |")
            md.append("|---|---|")
            for val in data["items"]:
                md.append(f"| `{val}` | State flag / Stack status |")
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

    with open(out_file, "w", encoding="utf-8") as f:
        f.write("\n".join(md))
    print(f"Generated {out_file} with {len(COMPLEMENTARY_HEADERS)} complementary headers.")


if __name__ == "__main__":
    main()
