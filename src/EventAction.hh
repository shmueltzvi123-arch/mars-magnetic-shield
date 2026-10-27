#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class RunAction;

/// EventAction class
///
/// Handles data per single particle launch (Event).
/// Used to act as a flag manager to prevent double counting of hits.

class EventAction : public G4UserEventAction
{
  public:
    EventAction(RunAction* runAction);
    virtual ~EventAction();

    virtual void BeginOfEventAction(const G4Event* event);
    virtual void EndOfEventAction(const G4Event* event);

    // Method called by SteppingAction when a hit is detected
    void RegisterHit(G4double energy);
    void AddHits(G4int low, G4int med, G4int high) {
        fLowEnergyHits += low;
        fMediumEnergyHits += med;
        fHighEnergyHits += high;
    }
  private:
    RunAction* fRunAction; // Pointer to RunAction to report results
    G4bool     fIsHit;     // Flag: has this event already recorded a hit?
    G4double   fHitEnergy; // Stores the energy of the first hit

    G4int fLowEnergyHits;    // < 100 MeV
    G4int fMediumEnergyHits; // 100-500 MeV
    G4int fHighEnergyHits;   // > 500 MeV
};

#endif