#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

class RunAction;
class G4Event;

class EventAction : public G4UserEventAction {
public:
    EventAction(RunAction* runAction);
    virtual ~EventAction() override = default;

    virtual void BeginOfEventAction(const G4Event* anEvent) override;
    virtual void EndOfEventAction(const G4Event* anEvent) override;

    void AddEdep(G4double edep) { fTotalEdep += edep; }

private:
    RunAction* fRunAction;
    G4double fTotalEdep;
};

#endif
