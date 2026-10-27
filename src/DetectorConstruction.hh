#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

class G4VPhysicalVolume;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    // Required Geant4 functions:
    // 1. Construct() builds the physical volumes (World and Habitat)
    G4VPhysicalVolume* Construct() override;
    // 2. ConstructSDandField() assigns the magnetic field and detectors
    void ConstructSDandField() override;
};

#endif