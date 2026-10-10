#include "DetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

G4VPhysicalVolume* DetectorConstruction::Construct() {
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
    G4Material* ge = nist->FindOrBuildMaterial("G4_Ge");

    // 世界体
    G4Box* solidWorld = new G4Box("World", 1.0*m, 1.0*m, 1.0*m);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World", 0, false, 0);

    // HPGe 同轴晶体
    G4Tubs* solidCrystal = new G4Tubs("HPGeCrystal", 0.0*cm, 3.5*cm, 3.5*cm, 0.0, 360.0*deg);
    G4LogicalVolume* logicCrystal = new G4LogicalVolume(solidCrystal, ge, "HPGeCrystalLogical");
    new G4PVPlacement(0, G4ThreeVector(0, 0, 10.0*cm), logicCrystal, "HPGePhys", logicWorld, false, 0);

    return physWorld;
}
