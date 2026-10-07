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
  double E0 = 10 * MeV; double sigma = 0.5 * MeV;
  double Ek = E0 + G4RandGauss::shoot(0.0, sigma);
  if (Ek <= 0.0) Ek = 1.0 * eV; // 非负截断 Ek > 0
  std::cout << "T6-8 Sampled non-negative Ek = " << Ek/MeV << " MeV" << std::endl;
  return 0;
}
