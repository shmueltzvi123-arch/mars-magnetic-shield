#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh" 
#include "globals.hh"

class G4Event;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    // The function that generates the particle (called by Geant4 for every event)
    void GeneratePrimaries(G4Event* anEvent) override;

private:
    G4ParticleGun* fParticleGun; // The standard Geant4 tool for shooting particles

    // Function to sample energy from the Solar Energetic Particle (SEP) spectrum
    G4double GetRandomSEP_Energy(); // <-- CRITICAL FUNCTION
};

#endif