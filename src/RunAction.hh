#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "G4AccValue.hh"
#include "globals.hh"

class G4Run;

/// RunAction class
///
/// Collects simulation results over the full Geant4 run.
///
/// In multi-threaded mode, every worker thread owns a separate RunAction
/// instance. G4AccValue is used for all counters so that worker results
/// can be safely merged into the master thread at the end of the run.
class RunAction : public G4UserRunAction
{
public:
    RunAction();
    ~RunAction() override;

    /// Called by Geant4 before event processing begins.
    void BeginOfRunAction(const G4Run* run) override;

    /// Called by Geant4 after all events in the run have finished.
    void EndOfRunAction(const G4Run* run) override;

   /// Records a habitat hit and its kinetic energy.
    ///
    /// Each event is reported at most once by EventAction. The hit counter
    /// contributes to the shielding-efficiency calculation, while the energy
    /// is stored in the proton hit-energy histogram.
    void AddEventResult(G4bool hit, G4double energy);

    /// Adds the energy-category counters reported by EventAction.
    void AddHits(G4int low, G4int medium, G4int high);

private:
    // Multi-thread-safe counters.
    //
    // Each worker updates its own local values. At the end of the run,
    // G4AccumulableManager merges them into the master RunAction.
    G4AccValue<G4int> fHitCount{0};
    G4AccValue<G4int> fLowEnergyHits{0};
    G4AccValue<G4int> fMediumEnergyHits{0};
    G4AccValue<G4int> fHighEnergyHits{0};
    // Geant4 analysis identifier for the proton hit-energy histogram.
    // The ID is returned by CreateH1() and stored here so the code does
    // not depend on a hard-coded histogram index.
    G4int fHitEnergyHistogramId{-1};
};

#endif