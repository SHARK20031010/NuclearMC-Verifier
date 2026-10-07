#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

int main(int argc, char** argv) {
  double t = 24.0 * 3600; // 24 hours
  double lambda1 = std::log(2.0) / (65.94 * 3600);
  double lambda2 = std::log(2.0) / (6.01 * 3600);
  double N1_0 = 1e10;
    double A2 = lambda2 * N1_0 * std::exp(-lambda1 * t);
  std::cout << "T3-5 Tc-99m Activity = " << A2 << " Bq" << std::endl;
  return 0;
}
