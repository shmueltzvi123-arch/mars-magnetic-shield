#include "ActionInitialization.hh"

#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "PrimaryGeneratorAction.hh"


ActionInitialization::ActionInitialization()
    : G4VUserActionInitialization()
{
}


ActionInitialization::~ActionInitialization()
{
}


void ActionInitialization::BuildForMaster() const
{
    // The master thread only needs the RunAction.
    SetUserAction(new RunAction());
}


void ActionInitialization::Build() const
{
    // Each worker thread receives its own RunAction.
    auto runAction = new RunAction();
    SetUserAction(runAction);

    // EventAction reports results to this worker's RunAction.
    auto eventAction = new EventAction(runAction);
    SetUserAction(eventAction);

    // SteppingAction reports hits to this worker's EventAction.
    SetUserAction(new SteppingAction(eventAction));

    // Each worker also needs its own primary generator.
    SetUserAction(new PrimaryGeneratorAction());
}