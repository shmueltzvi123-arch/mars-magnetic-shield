#include "EventAction.hh"
#include "RunAction.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh" // needed for the MeV unit

EventAction::EventAction(RunAction* runAction)
    : G4UserEventAction(), fRunAction(runAction), fIsHit(false), fHitEnergy(0.),
    fLowEnergyHits(0), fMediumEnergyHits(0), fHighEnergyHits(0)
{
}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event*) {
    fIsHit = false;
    fHitEnergy = 0.;
    fLowEnergyHits = fMediumEnergyHits = fHighEnergyHits = 0;
}

void EventAction::RegisterHit(G4double energy) {
    if (!fIsHit) {
        fIsHit = true;
        fHitEnergy = energy;
        // classify the hit by energy band
        if (energy < 100 * MeV) fLowEnergyHits = 1;
        else if (energy < 500 * MeV) fMediumEnergyHits = 1;
        else fHighEnergyHits = 1;
    }
}

void EventAction::EndOfEventAction(const G4Event*) {
    if (fIsHit) {
        // report this event to the worker RunAction
        fRunAction->AddHits(fLowEnergyHits, fMediumEnergyHits, fHighEnergyHits);
        fRunAction->AddEventResult(fIsHit, fHitEnergy);
    }
}
