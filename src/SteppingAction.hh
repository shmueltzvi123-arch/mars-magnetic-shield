#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class EventAction;

/// SteppingAction class
///
/// Checks every step of the particle simulation.
/// Detects when a particle crosses the boundary into the Habitat volume.

class SteppingAction : public G4UserSteppingAction
{
  public:
    SteppingAction(EventAction* eventAction);
    virtual ~SteppingAction();

    virtual void UserSteppingAction(const G4Step*);

  private:
    EventAction* fEventAction; // Pointer to report hits
};

#endif