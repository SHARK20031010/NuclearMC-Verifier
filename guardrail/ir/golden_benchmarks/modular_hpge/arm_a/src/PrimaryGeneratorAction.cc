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
    // 缺陷: Cs-137 放射性点源错误写为单向定轴平行发射 (0, 0, 1)，未实施 4pi 各向同性立体角抽样
    particleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
    particleGun->GeneratePrimaryVertex(anEvent);
}
