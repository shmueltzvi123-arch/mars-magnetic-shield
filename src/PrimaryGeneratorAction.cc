#include "PrimaryGeneratorAction.hh"
#include "G4Event.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4RandomTools.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction()
{
    fParticleGun = new G4ParticleGun(1); 

    // Define Particle: Proton
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle("proton");
    fParticleGun->SetParticleDefinition(particle);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

// --- Energy Sampling: Power Law for Solar Energetic Particles (SEP) ---
G4double PrimaryGeneratorAction::GetRandomSEP_Energy()
{
    G4double E_min = 1.0 * MeV;
    G4double E_max = 500.0 * MeV;
    G4double Gamma = 1.5; // Spectral index

    G4double R = G4UniformRand();

    // Inverse transform sampling for Power Law
    G4double energy =
        (std::pow(E_min, 1.0 - Gamma) - std::pow(E_max, 1.0 - Gamma)) * R + std::pow(E_max, 1.0 - Gamma);

    energy = std::pow(energy, 1.0 / (1.0 - Gamma));

    return energy;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
    // 1. Set Energy
    G4double E_kin = GetRandomSEP_Energy();
    fParticleGun->SetParticleEnergy(E_kin);

    // 2. Set Direction (Upper Hemisphere Only)
    // We want particles coming from space (downwards)
    G4ThreeVector direction = G4RandomDirection(); 

    // Force Z to be negative (pointing towards the ground)
    if (direction.z() > 0) {
        direction.setZ(-direction.z());
    }
    fParticleGun->SetParticleMomentumDirection(direction);

    // 3. Set Position
    // UPDATED: Start from 35m away to ensure we are outside the 20m outer coil
    G4double R_start = 35.0 * m; 
    
    // Backtrack from the target to find the start position
    G4ThreeVector position = R_start * (-direction);

    // Safety: Ensure we don't start below ground (horizon check)
    if (position.z() < 0.5 * m) position.setZ(0.5 * m);

    fParticleGun->SetParticlePosition(position);

    // 4. Fire
    fParticleGun->GeneratePrimaryVertex(anEvent);
}