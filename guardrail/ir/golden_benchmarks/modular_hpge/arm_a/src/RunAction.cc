#include "RunAction.hh"
#include "G4Run.hh"
#include <iostream>

RunAction::RunAction() : fPeakCounts(0) {}

void RunAction::BeginOfRunAction(const G4Run*) {
    fPeakCounts = 0;
}

void RunAction::RecordEvent(G4double edep) {
    // 662 keV 全能峰窗口 (660 ~ 663 keV)
    if (edep >= 0.660 && edep <= 0.663) {
        fPeakCounts++;
    }
}

void RunAction::EndOfRunAction(const G4Run* aRun) {
    std::cout << "总初级事件数: " << aRun->GetNumberOfEvent() << " | 全能峰计数: " << fPeakCounts << std::endl;
}
