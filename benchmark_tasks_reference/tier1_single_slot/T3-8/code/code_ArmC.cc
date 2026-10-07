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
  double R = 1e6; // reaction rate
  double lambda = std::log(2.0) / 7.13; // N-16
  double L = 3.0 * m; double v = 5.0 * m/s;
  double tau_residence = L / v; // 修复：停留时间 t=L/v
  double A = R * (1.0 - std::exp(-lambda * tau_residence));
  std::cout << "T3-8 N-16 Activity = " << A << " Bq/cm3" << std::endl;
  return 0;
}
