#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4Step.hh"

SteppingAction::SteppingAction(EventAction* eventAction) : fEventAction(eventAction) {}

void SteppingAction::UserSteppingAction(const G4Step* aStep) {
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep > 0.) {
        fEventAction->AddEdep(edep);
    }
}
