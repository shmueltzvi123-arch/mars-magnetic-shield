#ifndef BFIELDLOOKUP_HH
#define BFIELDLOOKUP_HH

#include "G4MagneticField.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"

// Global parameters (MUST match the Python code!)
const G4int N_POINTS = 51;  // 51 nodes per axis -> 51x51x51 grid, 1 m spacing
const G4double L_BOUNDS = 25.0 * m; // grid spans -25 m to +25 m on every axis

class BFieldLookup : public G4MagneticField
{
public:
    BFieldLookup(G4String filename);
    ~BFieldLookup() override = default;

    // The function GEANT4 calls to get the field vector
    void GetFieldValue(const G4double point[4], G4double field[6]) const override;

private:
    // Grid parameters calculated from L_BOUNDS and N_POINTS
    G4double fMin_X, fMax_X, fDelta_X;
    G4double fMin_Y, fMax_Y, fDelta_Y;
    G4double fMin_Z, fMax_Z, fDelta_Z;

    // 4D Array for storing Bx, By, Bz (in Tesla) at each grid point
    G4double fB_Data[N_POINTS][N_POINTS][N_POINTS][3];

    // Helper functions:
    void ReadFieldMap(G4String filename);
    void InterpolateField(G4double x, G4double y, G4double z, G4double& Bx, G4double& By, G4double& Bz) const;
};

#endif
