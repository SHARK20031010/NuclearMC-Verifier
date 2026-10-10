#ifndef STEPPING_ACTION_HH
#define STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class EventAction;
class G4Step;

class SteppingAction : public G4UserSteppingAction {
public:
    SteppingAction(EventAction* eventAction);
    virtual ~SteppingAction() override = default;

    virtual void UserSteppingAction(const G4Step* aStep) override;

private:
    EventAction* fEventAction;
};

#endif
