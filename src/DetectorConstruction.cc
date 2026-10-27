#include "DetectorConstruction.hh"
#include <cstdlib>

// --- Geometry and Material Headers ---
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Torus.hh"

// --- Magnetic Field Headers ---
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4Mag_UsualEqRhs.hh"
#include "G4ClassicalRK4.hh"
#include "G4IntegrationDriver.hh"
#include "G4ChordFinder.hh"
#include "BFieldLookup.hh"
#include "G4Colour.hh"

// --- Constructor & Destructor ---
DetectorConstruction::DetectorConstruction() : G4VUserDetectorConstruction() {}
DetectorConstruction::~DetectorConstruction() = default;

// ----------------------------------------------------
// 1. Physical Geometry Construction
// ----------------------------------------------------
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4NistManager* nist = G4NistManager::Instance();

    // --- Define Materials ---
    G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    G4Material* mars_soil = nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE"); // Approx for soil
    G4Material* mars_air = nist->FindOrBuildMaterial("G4_CARBON_DIOXIDE");   // Approx for atmosphere
    G4Material* habitat_mat = nist->FindOrBuildMaterial("G4_POLYETHYLENE");

    // --- 1. The World Volume ---
    // UPDATED: Increased size to 40m half-width (80m total) to accommodate the 20m coil.
    G4double world_size = 40.0 * m; 
    
    G4Box* solidWorld = new G4Box("World", world_size, world_size, world_size);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, vacuum, "World_LV");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), logicWorld, "World_PV", 0, false, 0, true);

    // --- 2. Martian Ground (Z < 0) ---
    // Covers the bottom half of the world (from Z = -40m to Z = 0)
    G4Box* solidGround = new G4Box("Ground", world_size, world_size, world_size / 2.0);
    G4LogicalVolume* logicGround = new G4LogicalVolume(solidGround, mars_soil, "Ground_LV");
    
    // Placed at Z = -20m relative to World center, so its top face is at Z = 0.
    new G4PVPlacement(0, G4ThreeVector(0, 0, -world_size / 2.0), logicGround, "Ground_PV", logicWorld, false, 0, true);

    // --- 3. Martian Atmosphere (Z > 0) ---
    // Covers the top half of the world (from Z = 0 to Z = +40m)
    G4Box* solidAir = new G4Box("Air", world_size, world_size, world_size / 2.0);
    G4LogicalVolume* logicAir = new G4LogicalVolume(solidAir, mars_air, "Air_LV");

    // Placed at Z = +20m relative to World center, so its bottom face is at Z = 0.
    new G4PVPlacement(0, G4ThreeVector(0, 0, +world_size / 2.0), logicAir, "Air_PV", logicWorld, false, 0, true);

    // --- 4. The Habitat (Protected Area) ---
    // Cylindrical shape representing the base. Placed INSIDE the Atmosphere.
    G4double innerRadius = 0.0 * m;
    G4double outerRadius = 9.0 * m; // ~254 sqm area
    G4double halfHeight = 2.5 * m;  // Total height 5m
    G4double startAngle = 0.0 * deg;
    G4double spanningAngle = 360.0 * deg;

    G4Tubs* solidHabitat = new G4Tubs("Habitat", innerRadius, outerRadius, halfHeight, startAngle, spanningAngle);
    G4LogicalVolume* logicHabitat = new G4LogicalVolume(solidHabitat, habitat_mat, "Habitat_LV");

    // --- 5. Forbidden Coil Volumes ---
// These volumes represent the physical "do not enter" region of the coils.
// They do NOT generate the magnetic field themselves; they only prevent tracks
// from passing through the singular coil region.

    G4double shieldCoilMajorRadius = 20.0 * m;
    G4double cancelCoilMajorRadius = 9.0 * m;
    G4double coilTubeRadius = 0.25 * m; // forbidden thickness around ideal loop

    G4Torus* solidShieldCoil = new G4Torus(
        "ShieldCoil",
        0.0 * m,                    // inner tube radius
        coilTubeRadius,             // outer tube radius
        shieldCoilMajorRadius,      // major radius
        0.0 * deg,
        360.0 * deg
    );

    G4LogicalVolume* logicShieldCoil = new G4LogicalVolume(
        solidShieldCoil,
        habitat_mat,                // temporary material choice
        "ShieldCoil_LV"
    );

    new G4PVPlacement(
        0,
        G4ThreeVector(0, 0, 0),
        logicShieldCoil,
        "ShieldCoil_PV",
        logicWorld,
        false,
        0,
        true
    );

    G4Torus* solidCancelCoil = new G4Torus(
        "CancelCoil",
        0.0 * m,
        coilTubeRadius,
        cancelCoilMajorRadius,
        0.0 * deg,
        360.0 * deg
    );

    G4LogicalVolume* logicCancelCoil = new G4LogicalVolume(
        solidCancelCoil,
        habitat_mat,                // temporary material choice
        "CancelCoil_LV"
    );

    new G4PVPlacement(
        0,
        G4ThreeVector(0, 0, 0),
        logicCancelCoil,
        "CancelCoil_PV",
        logicWorld,
        false,
        0,
        true
    );
    // Placement logic:
    // Air center is at Z_world = +20m. Air floor is at Z_world = 0.
    // In Air coordinates, the floor is at Z_local = -20m.
    // To place Habitat on the floor: Z_local = -20m + 2.5m (half height) = -17.5m.
    
    // Note: We need to adjust this calculation based on the new world size (40m)!
    // Air floor relative to Air center is -(world_size/2) = -20m.
    G4double habitat_z_pos = -(world_size / 2.0) + halfHeight; 

    new G4PVPlacement(0, G4ThreeVector(0, 0, habitat_z_pos), logicHabitat, "Habitat_PV", logicAir, false, 0, true);

    // --- Visualization Attributes ---
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
    
    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
//{
//    // Ensure this file exists in the build directory!
//    G4String field_map_file = "magnetic_field_map_optimized.txt";
//
//    // 1. Custom Magnetic Field
//    BFieldLookup* magneticField = new BFieldLookup(field_map_file);
//
//    // 2. Global Field Manager
//    G4FieldManager* fieldManager = G4TransportationManager::GetTransportationManager()->GetFieldManager();
//    fieldManager->SetDetectorField(magneticField);
//
//    // 3. Equation of Motion & Stepper
//    G4Mag_UsualEqRhs* equation = new G4Mag_UsualEqRhs(magneticField);
//    G4ClassicalRK4* stepper = new G4ClassicalRK4(equation);
//
//    // 4. Integration Driver
//    G4double minStep = 0.01 * mm; // High precision for magnetic deflection
//    auto* intDriver = new G4IntegrationDriver<G4ClassicalRK4>(minStep, stepper, stepper->GetNumberOfVariables());
//
//    // 5. Chord Finder
//    G4ChordFinder* chordFinder = new G4ChordFinder(intDriver);
//    chordFinder->SetDeltaChord(3.0 * mm);
//    fieldManager->SetChordFinder(chordFinder);
//}


void DetectorConstruction::ConstructSDandField()
{
    // Read field map path from environment variable
    const char* envFieldMap = std::getenv("MAG_FIELD_MAP");

    if (!envFieldMap) {
        G4cerr << "ERROR: MAG_FIELD_MAP environment variable is not set." << G4endl;
        G4cerr << "Please set MAG_FIELD_MAP to the full path of the field map file." << G4endl;
        return;
    }

    G4String field_map_file = envFieldMap;

    G4cout << "--- Using dynamic field map: " << field_map_file << G4endl;

    // --- 1. Load the Magnetic Field Map ---
    // Ensure "magnetic_field_map_optimized.txt" is present in your build/Release folder
    BFieldLookup* magneticField = new BFieldLookup(field_map_file);

    // --- 2. Global Field Manager Setup ---
    // Access the transportation manager to apply the field globally across the world volume
    G4FieldManager* fieldManager = G4TransportationManager::GetTransportationManager()->GetFieldManager();
    fieldManager->SetDetectorField(magneticField);

    // --- 3. Physics: Equation of Motion & Stepper ---
    // Defines how particles move in the field. ClassicalRK4 (Runge-Kutta) is standard for magnetic fields.
    G4Mag_UsualEqRhs* equation = new G4Mag_UsualEqRhs(magneticField);
    G4ClassicalRK4* stepper = new G4ClassicalRK4(equation);

    // --- 4. High-Precision Integration Driver ---
    // We decrease minStep to force Geant4 to calculate more points along the trajectory.
    // This removes the "scribbled" look and creates smooth curves.
    G4double minStep = 0.001 * mm; // Increased precision (Previous: 0.01 mm)
    auto* intDriver = new G4IntegrationDriver<G4ClassicalRK4>(
        minStep,
        stepper,
        stepper->GetNumberOfVariables()
    );

    // --- 5. Chord Finder (The "Resolution" Tool) ---
    // deltaChord defines the max allowed distance between the true curve and the calculated segment.
    // Setting it to 0.1 mm forces the simulation to draw many more points for a smooth visual result.
    G4ChordFinder* chordFinder = new G4ChordFinder(intDriver);
    G4double deltaChord = 0.5 * mm; // High resolution (Previous: 3.0 mm)
    chordFinder->SetDeltaChord(deltaChord);

    fieldManager->SetChordFinder(chordFinder);

    // --- 6. Global Precision Parameters ---
    // These settings ensure accuracy when particles cross between volumes (e.g., entering the Habitat)
    fieldManager->SetDeltaOneStep(0.05 * mm);      // Accuracy within a single step
    fieldManager->SetDeltaIntersection(0.05 * mm); // Accuracy at volume boundaries
}