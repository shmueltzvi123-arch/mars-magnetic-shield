#include "RunAction.hh"

#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4AccumulableManager.hh"
#include "G4SystemOfUnits.hh"


RunAction::RunAction()
    : G4UserRunAction()
{
    // Register all run counters with Geant4's accumulable manager.
    //
    // In multi-threaded mode, Geant4 creates a separate RunAction for
    // each worker thread. Registering these counters allows their values
    // to be merged into the master thread when the run is complete.
    auto accumulableManager = G4AccumulableManager::Instance();

    accumulableManager->Register(fHitCount);
    accumulableManager->Register(fLowEnergyHits);
    accumulableManager->Register(fMediumEnergyHits);
    accumulableManager->Register(fHighEnergyHits);

    // Configure the analysis output.
    //
    // The histogram is created here, but its filling logic will be handled
    // separately after the multi-threaded run counters are verified.
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetDefaultFileType("csv");

    // Create the proton hit-energy histogram and preserve its ID.
    //
    // Each worker thread fills its own local histogram. Geant4 automatically
    // merges the worker histograms when the analysis data is written.
    fHitEnergyHistogramId = analysisManager->CreateH1(
        "HitEnergy",
        "Kinetic Energy of Protons",
        100,
        0.,
        500. * MeV
    );
}


RunAction::~RunAction()
{
}


void RunAction::BeginOfRunAction(const G4Run* run)
{
    // Reset all counters before starting a new run.
    //
    // Reset() restores every registered G4AccValue to its initial value.
    auto accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->Reset();

    // Open the analysis output file for this Geant4 run.
    auto analysisManager = G4AnalysisManager::Instance();

    G4String fileName =
        "MarsShield_Run_" + std::to_string(run->GetRunID());

    analysisManager->OpenFile(fileName);
}


void RunAction::AddHits(
    G4int low,
    G4int medium,
    G4int high
)
{
    // Accumulate the energy classification of habitat hits.
    fLowEnergyHits += low;
    fMediumEnergyHits += medium;
    fHighEnergyHits += high;
}


void RunAction::AddEventResult(
    G4bool hit,
    G4double energy
)
{
    // EventAction reports at most one habitat hit per event.
    // Therefore every accepted call represents one unprotected event.
    if (!hit) {
        return;
    }

    // Count the event for the final shielding-efficiency calculation.
    fHitCount += 1;

    // Record the kinetic energy of the proton at its first entry into
    // the habitat. In MT mode each worker fills its local histogram;
    // Geant4 merges all worker histograms during Write().
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->FillH1(
        fHitEnergyHistogramId,
        energy
    );
}


void RunAction::EndOfRunAction(const G4Run* run)
{
    // Merge all worker-thread counters into the master thread.
    //
    // After this call, the master RunAction contains the combined totals
    // from every worker that participated in the simulation.
    auto accumulableManager = G4AccumulableManager::Instance();
    accumulableManager->Merge();

    // Finish and close the Geant4 analysis output.
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();

    // Worker threads hold only partial statistics.
    // The final simulation result must therefore be printed only by
    // the master thread after all worker values have been merged.
    if (!IsMaster()) {
        return;
    }

    const G4int numberOfEvents = run->GetNumberOfEvent();

    if (numberOfEvents == 0) {
        return;
    }

    // Read the final merged counters from the master accumulables.
    const G4int hitCount = fHitCount.GetValue();
    const G4int lowEnergyHits = fLowEnergyHits.GetValue();
    const G4int mediumEnergyHits = fMediumEnergyHits.GetValue();
    const G4int highEnergyHits = fHighEnergyHits.GetValue();

    // Shielding efficiency is defined as the percentage of launched
    // particles that did NOT reach the habitat.
    const G4double shieldingEfficiency =
        100.0 -
        (100.0 * static_cast<G4double>(hitCount) /
         static_cast<G4double>(numberOfEvents));

    // Print one final combined result for the complete multi-threaded run.
    G4cout << "\n################ RESULT ################" << G4endl;

    G4cout
        << " Total Events Shot:    "
        << numberOfEvents
        << G4endl;

    G4cout
        << " Shielding Efficiency:  "
        << shieldingEfficiency
        << " %"
        << G4endl;

    G4cout
        << " Efficiency Low (<100MeV): "
        << lowEnergyHits
        << " hits"
        << G4endl;

    G4cout
        << " Efficiency Medium (100-500MeV): "
        << mediumEnergyHits
        << " hits"
        << G4endl;

    G4cout
        << " Efficiency High (>500MeV): "
        << highEnergyHits
        << " hits"
        << G4endl;

    G4cout << "########################################\n" << G4endl;
}