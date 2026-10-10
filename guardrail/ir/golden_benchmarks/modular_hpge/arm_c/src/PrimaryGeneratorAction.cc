#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction() {
    particleGun = new G4ParticleGun(1);
    G4ParticleDefinition* gamma = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
    particleGun->SetParticleDefinition(gamma);
    particleGun->SetParticleEnergy(661.7 * keV);
    particleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete particleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {
    // 黄金基准: 4pi 球面度空间各向同性立体角正确抽样 (dOmega = sin(theta) dtheta dphi)
    G4double cosTheta = 2.0 * G4UniformRand() - 1.0;
    G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
    G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
    G4ThreeVector dir(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
    particleGun->SetParticleMomentumDirection(dir);
    particleGun->GeneratePrimaryVertex(anEvent);
}
