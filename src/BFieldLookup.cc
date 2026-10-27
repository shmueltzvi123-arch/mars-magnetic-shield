#include "BFieldLookup.hh"
#include <fstream>
#include <iostream>
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"

// --- Constructor ---
BFieldLookup::BFieldLookup(G4String filename)
{
    G4cout << "--- MAGNETIC SHIELD: Loading Field Map ---" << G4endl;

    // DEFINING BOUNDARIES
    // IMPORTANT: These must match the Python script that generated the map.
    // Updated to 40m to accommodate the 20m outer coil.
    fMin_X = -25000.0 * mm; fMax_X = 25000.0 * mm;
    fMin_Y = -25000.0 * mm; fMax_Y = 25000.0 * mm;
    fMin_Z = -25000.0 * mm; fMax_Z = 25000.0 * mm;

    // Calculate step size based on N_POINTS (defined in header)
    fDelta_X = (fMax_X - fMin_X) / (N_POINTS - 1);
    fDelta_Y = (fMax_Y - fMin_Y) / (N_POINTS - 1);
    fDelta_Z = (fMax_Z - fMin_Z) / (N_POINTS - 1);

    ReadFieldMap(filename);
}

// --- Read Data from File ---
void BFieldLookup::ReadFieldMap(G4String filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        G4cerr << "ERROR: Could not open field map file: " << filename << G4endl;
        return;
    }

    G4String header;
    std::getline(file, header); // Skip header

    G4double x, y, z, Bx, By, Bz;
    G4int count = 0;
    G4int max_index = N_POINTS * N_POINTS * N_POINTS;

    // Assumes file order matches the loop order (Z changing fastest)
    while (file >> x >> y >> z >> Bx >> By >> Bz)
    {
        G4int i = count / (N_POINTS * N_POINTS);
        G4int j = (count / N_POINTS) % N_POINTS;
        G4int k = count % N_POINTS;

        if (count < max_index) {
            fB_Data[i][j][k][0] = Bx * tesla;
            fB_Data[i][j][k][1] = By * tesla;
            fB_Data[i][j][k][2] = Bz * tesla;
        }
        count++;
    }
    file.close();
    G4cout << "--- SUCCESS: Loaded " << count << " points. ---" << G4endl;
    G4cout << "--- EXPECTED: " << max_index << " data rows. ---" << G4endl;

    if (count != max_index) {
        G4cerr << "WARNING: Field map row count mismatch. "
            << "Loaded " << count << ", expected " << max_index << G4endl;
    }
}

// --- Get Field Value (Main Function) ---
void BFieldLookup::GetFieldValue(const G4double point[4], G4double field[6]) const
{
    G4double x = point[0]; 
    G4double y = point[1]; 
    G4double z = point[2];

    // --- GROUND CUTOFF ---
    // If particle is underground (Z < 0), assume field is zero.
    // This saves computation and avoids artifacts below the surface.
    if (z < 0.0) {
       field[0] = field[1] = field[2] = 0.0;
       field[3] = field[4] = field[5] = 0.0;
       return; 
    }

    // Check if point is inside our mapped area
    if (x <= fMin_X || x >= fMax_X || 
        y <= fMin_Y || y >= fMax_Y || 
        z <= fMin_Z || z >= fMax_Z) {
        
        field[0] = field[1] = field[2] = 0.0;
    } else {
        InterpolateField(x, y, z, field[0], field[1], field[2]);
    }
    
    // Electric field is zero
    field[3] = field[4] = field[5] = 0.0;
}

// --- Trilinear Interpolation ---
void BFieldLookup::InterpolateField(G4double x, G4double y, G4double z, G4double& Bx, G4double& By, G4double& Bz) const
{
    // Find grid cell index
    G4int i = (int)((x - fMin_X) / fDelta_X);
    G4int j = (int)((y - fMin_Y) / fDelta_Y);
    G4int k = (int)((z - fMin_Z) / fDelta_Z);

    // Boundary safety check
    if (i >= N_POINTS - 1) i = N_POINTS - 2;
    if (j >= N_POINTS - 1) j = N_POINTS - 2;
    if (k >= N_POINTS - 1) k = N_POINTS - 2;

    // Relative position within the cell
    G4double xd = (x - (fMin_X + i * fDelta_X)) / fDelta_X;
    G4double yd = (y - (fMin_Y + j * fDelta_Y)) / fDelta_Y;
    G4double zd = (z - (fMin_Z + k * fDelta_Z)) / fDelta_Z;

    // Interpolate for each dimension
    for (int dim = 0; dim < 3; dim++) {
        double c00 = fB_Data[i][j][k][dim] * (1-xd) + fB_Data[i+1][j][k][dim] * xd;
        double c01 = fB_Data[i][j][k+1][dim] * (1-xd) + fB_Data[i+1][j][k+1][dim] * xd;
        double c10 = fB_Data[i][j+1][k][dim] * (1-xd) + fB_Data[i+1][j+1][k][dim] * xd;
        double c11 = fB_Data[i][j+1][k+1][dim] * (1-xd) + fB_Data[i+1][j+1][k+1][dim] * xd;
        
        double c0 = c00 * (1-yd) + c10 * yd;
        double c1 = c01 * (1-yd) + c11 * yd;
        
        double res = c0 * (1-zd) + c1 * zd;

        if (dim == 0) Bx = res;
        if (dim == 1) By = res;
        if (dim == 2) Bz = res;
    }
}