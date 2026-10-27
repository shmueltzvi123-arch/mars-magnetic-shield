#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4Step.hh"
#include "G4RunManager.hh"

SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction)
{}

SteppingAction::~SteppingAction()
{}
void SteppingAction::UserSteppingAction(const G4Step* step)
{
    auto preVolume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    auto postVolume = step->GetPostStepPoint()->GetTouchableHandle()->GetVolume();

    if (!preVolume || !postVolume) return;

    G4String preName = preVolume->GetName();
    G4String postName = postVolume->GetName();

    // --- Kill track if it enters one of the forbidden coil volumes ---
    if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) {
        if (postName == "ShieldCoil_PV" || postName == "CancelCoil_PV") {
            step->GetTrack()->SetTrackStatus(fStopAndKill);
            return;
        }
    }

    // --- Detect entry into the habitat ---
    if (postName == "Habitat_PV") {
        if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) {
            G4double kineticEnergy = step->GetPostStepPoint()->GetKineticEnergy();
            fEventAction->RegisterHit(kineticEnergy);
        }
    }
}
