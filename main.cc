#include "G4RunManagerFactory.hh"
#include "G4PhysListFactory.hh"
#include "G4UImanager.hh"
#include "globals.hh"

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"


int main(int argc, char** argv)

{
    // Batch-only mode: require a macro file.
    if (argc != 2) {
        G4cerr << "Usage: MarsShield <macro_file.mac>" << G4endl;
        return 1;
    }

    // Multi-threaded Geant4 run manager.
    auto runManager =
        G4RunManagerFactory::CreateRunManager(
            G4RunManagerType::MT
        );

    // Geometry.
    runManager->SetUserInitialization(
        new DetectorConstruction()
    );

    // Physics.
    G4PhysListFactory physListFactory;

    G4VModularPhysicsList* physicsList =
        physListFactory.GetReferencePhysList(
            "QGSP_BERT_HP"
        );

    runManager->SetUserInitialization(
        physicsList
    );

    // User actions.
    // In MT mode these must be registered through
    // G4VUserActionInitialization.
    runManager->SetUserInitialization(
        new ActionInitialization()
    );

    // Initialize Geant4 kernel.
    runManager->Initialize();

    // Execute macro in batch mode.
    G4String command = "/control/execute ";
    G4String fileName = argv[1];

    G4UImanager* uiManager =
        G4UImanager::GetUIpointer();

    uiManager->ApplyCommand(
        command + fileName
    );

    delete runManager;

    return 0;
}