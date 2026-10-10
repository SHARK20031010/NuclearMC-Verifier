#include "EventAction.hh"
#include "RunAction.hh"
#include "G4Event.hh"

EventAction::EventAction(RunAction* runAction) : fRunAction(runAction), fTotalEdep(0.) {}

void EventAction::BeginOfEventAction(const G4Event*) {
    fTotalEdep = 0.;
}

void EventAction::EndOfEventAction(const G4Event*) {
    if (fTotalEdep > 0.) {
        fRunAction->RecordEvent(fTotalEdep);
    }
}
