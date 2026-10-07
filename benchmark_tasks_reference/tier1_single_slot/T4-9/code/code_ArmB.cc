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
  double tau = 5.0 * us;
  double t_last = -1e9;
  G4long recorded = 0;
  for (int i=0; i<100; ++i) {
    double t = i * 2.0 * us; // 到达时间戳
    double dt = t - t_last;
    if (dt >= tau) { // 修复：死时间判定
      recorded++;
      t_last = t;
    }
  }
  std::cout << "T4-9 Recorded counts = " << recorded << std::endl;
  return 0;
}
