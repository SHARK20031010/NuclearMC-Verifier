#ifndef RUN_ACTION_HH
#define RUN_ACTION_HH

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;

class RunAction : public G4UserRunAction {
public:
    RunAction();
    virtual ~RunAction() override = default;

    virtual void BeginOfRunAction(const G4Run* aRun) override;
    virtual void EndOfRunAction(const G4Run* aRun) override;

    void RecordEvent(G4double edep);

private:
    G4int fPeakCounts;
};

#endif
