// =========================================================================
// 项目说明: HPGe 高纯锗伽马谱仪 Cs-137 放射性点源全能峰效率标定仿真
// 观测量: 662 keV 全能峰探测效率 (Peak Efficiency)
// 辐射源: Cs-137 放射性点源 (4pi 球面度空间各向同性)
// =========================================================================

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "QGSP_BERT.hh"

#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

int main(int argc, char** argv) {
    G4RunManager* runManager = new G4RunManager();
    runManager->SetUserInitialization(new DetectorConstruction());
    runManager->SetUserInitialization(new QGSP_BERT());

    runManager->SetUserAction(new PrimaryGeneratorAction());
    RunAction* runAction = new RunAction();
    runManager->SetUserAction(runAction);
    EventAction* eventAction = new EventAction(runAction);
    runManager->SetUserAction(eventAction);
    runManager->SetUserAction(new SteppingAction(eventAction));

    runManager->Initialize();
    runManager->BeamOn(10000);

    delete runManager;
    return 0;
}
