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
  double t = 30.0 * 86400; // 30 days
  double lambda_p = std::log(2.0) / (8.02 * 86400); // I-131
  double lambda_biol = std::log(2.0) / (1.5 * 86400); // 修复：呼吸道生物清除
  double lambda_eff = lambda_p + lambda_biol; // 有效衰变常数
  double A_rem = 1e6 * std::exp(-lambda_eff * t);
  std::cout << "T3-10 Retained Activity = " << A_rem << " Bq" << std::endl;
  return 0;
}
