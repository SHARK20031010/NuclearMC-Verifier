# Complementary Subsystems & Run Management APIs

> **Scope**: Deep-dive inspection of top-level run driving, modern MT action assembly, stacking classification, custom Hits, extended solids, magnetic fields, optical physics, and variance reduction.

> **Header Base Path**: `/home/shark/geant4/11.2.2-install/include/Geant4`

---

## Header: `G4RunManagerFactory.hh` (Target: `G4RunManagerFactory`)

**Kind**: C++ Class (`G4RunManagerFactory`) | **Total Extracted Public Methods**: 14

| C++ Method Signature | Comment / Description |
|---|---|
| `static G4RunManager* CreateRunManager(G4RunManagerType _type = G4RunManagerType::Default, G4VUserTaskQueue* _queue = nullptr, G4bool fail_if_unavail = true, G4int nthreads = 0)` | - |
| `static G4RunManager* CreateRunManager(G4RunManagerType _type, G4bool fail_if_unavail, G4int nthreads = 0, G4VUserTaskQueue* _queue = nullptr)` | provide a version which specifies to fail if unavailable |
| `return CreateRunManager(_type, _queue, fail_if_unavail, nthreads)` | - |
| `} static G4RunManager* CreateRunManager(G4RunManagerType _type, G4int nthreads, G4bool fail_if_unavail = true, G4VUserTaskQueue* _queue = nullptr)` | - |
| `return CreateRunManager(_type, _queue, fail_if_unavail, nthreads)` | - |
| `} template<typename... Args> static G4RunManager* CreateRunManager(std::string type, Args&&... args)` | provide string versions of above |
| `return CreateRunManager(GetType(type), std::forward<Args>(args)...)` | - |
| `} static std::string GetDefault()` | - |
| `static std::string GetName(G4RunManagerType)` | - |
| `static G4RunManagerType GetType(const std::string&)` | - |
| `static std::set<std::string> GetOptions()` | - |
| `static G4RunManager* GetMasterRunManager()` | - |
| `static G4MTRunManager* GetMTMasterRunManager()` | - |
| `static G4RunManagerKernel* GetMasterRunManagerKernel()` | - |


## Header: `G4RunManager.hh` (Target: `G4RunManager`)

**Kind**: C++ Class (`G4RunManager`) | **Total Extracted Public Methods**: 112

| C++ Method Signature | Comment / Description |
|---|---|
| `static G4RunManager* GetRunManager()` | Static method which returns the singleton pointer of G4RunManager or its derived class. Note this returns the per-thread singleton in case of a multi-threaded build. |
| `G4RunManager()` | The constructor and the destructor. The user must construct this class object at the beginning of his/her main() and must delete it at the bottom of the main(). |
| `virtual ~G4RunManager()` | - |
| `G4RunManager(const G4RunManager&) = delete` | Forbidden copy constructor and assignment operator. |
| `G4RunManager& operator=(const G4RunManager&) = delete` | - |
| `virtual void BeamOn(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1)` | This method starts an event loop of "n_event" events. The condition of Geant4 is examined before starting the event loop. This method must be invoked at 'Idle' state. The state will be changed to 'GeomClosed' during the event loop and will go back to 'Idle' when the loop is over or aborted. In case a string "macroFile" which represents the name of a macro file is provided, the macro file will be executed AT THE END of each event processing. In case "n_select" is greater than zero, at the end of the first "n_select" events, the macro file is executed. |
| `virtual void Initialize()` | This method invokes all the necessary initialisation procedures for an event loop. This method must be invoked at the Geant4 'PreInit' state or 'Idle'. The state will be changed to 'Init' during initialization procedures and then changed to 'Idle'. This method invokes two protected methods, InitializeGeometry() and InitializePhysics(). After some event loops, the user can invoke this method once again. It is required if the user changes geometry, physics process, and/or cut-off value. If the user forget the second invocation, the BeamOn() method will invoke this method (Note that this feature is not valid for the first initialization). |
| `virtual void DefineWorldVolume(G4VPhysicalVolume* worldVol, G4bool topologyIsChanged = true)` | This method must be invoked if the geometry setup has been changed between runs. The flag "topologyIsChanged" will specify if the geometry topology is different from the original one used in the previous run; if not, it must be set to false, so that the original optimisation and navigation history are preserved. This method is invoked also at initialisation. |
| `virtual void AbortRun(G4bool softAbort = false)` | This method safely aborts the current event loop even if an event is in progress. This method is available for 'GeomClosed' and 'EventProc' Geant4 states. The application state will be changed to 'Idle', so that another event loop can be processed. If the "softAbort" flag is true, the event loop is aborted after processing the current event, while the current event is aborted if the flag is set to false. |
| `virtual void AbortEvent()` | This method aborts the currently processing event, remaining events in the current event loop will be processed. This method is available only for 'EventProc' application state. |
| `virtual void InitializeGeometry()` | These methods are invoked from the Initialize() method for the initializations of geometry and physics processes. The user's concrete G4VUserDetectorConstruction class will be accessed from the method InitializeGeometry() and the G4VUserPhysicsList class will be accessed from the method InitializePhysics(). |
| `virtual void InitializePhysics()` | - |
| `virtual G4bool ConfirmBeamOnCondition()` | These four methods are invoked from the BeamOn() method and they're invoked in this order. ConfirmBeamOnCondition() method checks if all the necessary initialisations have been done already. If the condition is not satisfied, false is returned and the following three methods will be skipped. The RunInitialization() method initialises a run. e.g., a G4Run class object is constructed in this method. The DoEventLoop() method controls an event loop. Arguments are the same as for the BeamOn() method. Inside the event loop, the two following methods are invoked at the beginning and at the end of each event. The RunTermination() method terminates a run processing. e.g., a G4Run class object is deleted in this method. If the user adopts ODBMS and wants to store the G4Run object, he/she must override this method. |
| `virtual void RunInitialization()` | - |
| `virtual void DoEventLoop(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1)` | - |
| `virtual void RunTermination()` | - |
| `virtual void InitializeEventLoop(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1)` | Granular virtual methods invoked from DoEventLoop(). |
| `virtual void ProcessOneEvent(G4int i_event)` | - |
| `virtual void TerminateOneEvent()` | - |
| `virtual void TerminateEventLoop()` | - |
| `virtual G4Event* GenerateEvent(G4int i_event)` | These two methods are invoked from DoEventLoop() at the beginning and at the end of each event processing. GenerateEvent() constructs a G4Event class object and invoke the user's G4VUserPrimaryGeneratorAction concrete class. If the user is adopting an ODBMS system and event objects have been created and stored in the data-base, he/she must override this method. AnalyzeEvent() stores an event to a data-base if a concrete G4VPersistentManager class is defined. |
| `virtual void AnalyzeEvent(G4Event* anEvent)` | - |
| `void ConfigureProfilers(G4int argc, char** argv)` | Calls the above virtual method. |
| `virtual void SetNumberOfThreads(G4int)` | Dummy methods to dispatch generic inheritance calls from G4RunManager base class. |
| `virtual G4int GetNumberOfThreads() const` | - |
| `void DumpRegion(const G4String& rname) const` | Dump information of a region. |
| `void DumpRegion(G4Region* region = nullptr) const` | Dump information of a region. If the pointer is NULL, all regions are shown. |
| `void GeometryHasBeenModified(G4bool prop = true)` | This method must be invoked (or equivalent UI command can be used) in case the user changes his/her detector geometry after Initialize() method has been invoked. Then, at the beginning of the next BeamOn(), all necessary geometry optimisations will be made. The parameter "prop" has to be true if this C++ method is directly invoked. |
| `void ReinitializeGeometry(G4bool destroyFirst = false, G4bool prop = true)` | This method must be invoked (or equivalent UI command can be used) in case the user needs his/her detector construction has to be re-invoked. Geometry optimisations will be also done. If the first parameter "destroyFirst" is true, G4SolidStore, G4LogicalVolumeStore and G4PhysicalVolumeStore are cleaned up, and thus all solids, logical volumes and physical volumes previously defined are deleted. The second parameter "prop" has to be true if this C++ method is directly invoked. |
| `inline void PhysicsHasBeenModified()` | This method must be invoked (or equivalent UI command can be used) in case the user changes his/her physics process(es), e.g. (in)activate some processes. Once this method is invoked, regardless of cuts are changed or not, BuildPhysicsTable() of a PhysicsList is invoked for refreshing all physics tables. |
| `inline void CutOffHasBeenModified()` | - |
| `} void ReOptimizeMotherOf(G4VPhysicalVolume*)` | This method may be used if the orientation and/or size of a particular physical volume has been modified while the rest of the geometries in the world has not been changed. This avoids the full re-optimisation of the entire geometry tree which is forced if GeometryHasBeenModified() method is invoked. |
| `void ReOptimize(G4LogicalVolume*)` | Same as above, but the mother logical volume is specified instead. |
| `inline void SetGeometryToBeOptimized(G4bool vl)` | - |
| `if (geometryToBeOptimized != vl)` | - |
| `kernel->GeometryHasBeenModified()` | - |
| `kernel->SetGeometryToBeOptimized(vl)` | - |
| `} } inline G4bool GetGeometryToBeOptimized()` | - |
| `void GeometryDirectlyUpdated(G4bool val = true)` | - |
| `static G4bool IfGeometryHasBeenDestroyed()` | This is used only by workers thread to reset RNG engines from files that are event specific. Not implemented for sequential since run seed defines event seeds. |
| `virtual void ConstructScoringWorlds()` | - |
| `virtual void rndmSaveThisRun()` | - |
| `virtual void rndmSaveThisEvent()` | - |
| `virtual void RestoreRandomNumberStatus(const G4String& fileN)` | - |
| `virtual void RestoreRndmEachEvent(G4bool)` | - |
| `} virtual void SetUserInitialization(G4VUserDetectorConstruction* userInit)` | Set user-actions and user-initialization to the kernel. Store respective user initialization and action classes. In MT mode, actions are shared among all threads, and should be set in the master thread, while user-actions are thread-private and each      ` thread has private instances. Master thread does not have user-actions except for the (optional) run-action. User should instantiate the user-actions in the action-initialization and use that class' setters to set user-actions and *not* directly the methods provided here. Multiple Run, Event, Tracking and Stepping actions are allowed, the multiple instances will be appended to the current configuration. Multiple Stacking and PrimaryGeneration are not allowed. |
| `virtual void SetUserInitialization(G4VUserPhysicsList* userInit)` | - |
| `virtual void SetUserInitialization(G4VUserActionInitialization* userInit)` | - |
| `virtual void SetUserInitialization(G4UserWorkerInitialization* userInit)` | - |
| `virtual void SetUserInitialization(G4UserWorkerThreadInitialization* userInit)` | - |
| `virtual void SetUserAction(G4UserRunAction* userAction)` | - |
| `virtual void SetUserAction(G4VUserPrimaryGeneratorAction* userAction)` | - |
| `virtual void SetUserAction(G4UserEventAction* userAction)` | - |
| `virtual void SetUserAction(G4UserStackingAction* userAction)` | - |
| `virtual void SetUserAction(G4UserTrackingAction* userAction)` | - |
| `virtual void SetUserAction(G4UserSteppingAction* userAction)` | - |
| `inline const G4VUserDetectorConstruction* GetUserDetectorConstruction() const` | Methods returning respective user initialization and action classes. |
| `} inline const G4VUserPhysicsList* GetUserPhysicsList() const` | - |
| `inline const G4VUserActionInitialization* GetUserActionInitialization() const` | - |
| `} inline G4VUserActionInitialization* GetNonConstUserActionInitialization() const` | - |
| `} inline const G4UserWorkerInitialization* GetUserWorkerInitialization() const` | - |
| `} inline const G4UserWorkerThreadInitialization* GetUserWorkerThreadInitialization() const` | - |
| `} inline const G4UserRunAction* GetUserRunAction() const` | - |
| `inline const G4VUserPrimaryGeneratorAction* GetUserPrimaryGeneratorAction() const` | - |
| `} inline const G4UserEventAction* GetUserEventAction() const` | - |
| `inline const G4UserStackingAction* GetUserStackingAction() const` | - |
| `inline const G4UserTrackingAction* GetUserTrackingAction() const` | - |
| `inline const G4UserSteppingAction* GetUserSteppingAction() const` | - |
| `inline void SetNumberOfAdditionalWaitingStacks(G4int iAdd)` | Set the number of additional (optional) waiting stacks. This method must be invoked at 'PreInit', 'Init' or 'Idle' states. Once the user sets the number of additional waiting stacks, he/she can use the corresponding ENUM in G4ClassificationOfNewTrack. |
| `eventManager->GetStackManager()->SetNumberOfAdditionalWaitingStacks(iAdd)` | - |
| `} inline const G4String& GetVersionString() const` | - |
| `inline void SetPrimaryTransformer(G4PrimaryTransformer* pt)` | - |
| `kernel->SetPrimaryTransformer(pt)` | - |
| `} inline void StoreRandomNumberStatusToG4Event(G4int vl)` | if vl = 1 : status before primary particle generation is stored if vl = 2 : status before event processing (after primary particle generation) is stored if vl = 3 : both are stored if vl = 0 : none is stored (default). |
| `eventManager->StoreRandomNumberStatusToG4Event(vl)` | - |
| `} inline G4int GetFlagRandomNumberStatusToG4Event() const` | - |
| `} inline void SetRandomNumberStore(G4bool flag)` | - |
| `inline G4bool GetRandomNumberStore() const` | - |
| `inline void SetRandomNumberStoreDir(const G4String& dir)` | - |
| `if (dirStr.back() != '/') dirStr += "/"` | - |
| `std::replace(dirStr.begin(), dirStr.end(), '/', '\\')` | - |
| `G4int sysret = system(shellCmd)` | - |
| `if (sysret != 0)` | - |
| `G4Exception("GrRunManager::SetRandomNumberStoreDir", "Run0071", JustWarning, errmsg)` | - |
| `} } inline const G4String& GetRandomNumberStoreDir() const` | - |
| `inline const G4String& GetRandomNumberStatusForThisRun() const` | - |
| `} inline const G4String& GetRandomNumberStatusForThisEvent() const` | - |
| `if (storeRandomNumberStatusToG4Event == 0 \|\| storeRandomNumberStatusToG4Event == 2)` | - |
| `G4Exception("GrRunManager::SetRandomNumberStoreDir", "Run0072", JustWarning, "Random number status is not available for this event.")` | - |
| `} inline void SetRandomNumberStorePerEvent(G4bool flag)` | - |
| `inline G4bool GetRandomNumberStorePerEvent() const` | - |
| `inline void SetVerboseLevel(G4int vl)` | - |
| `kernel->SetVerboseLevel(vl)` | - |
| `} inline G4int GetVerboseLevel() const` | - |
| `inline G4int GetPrintProgress()` | - |
| `inline void SetPrintProgress(G4int i)` | - |
| `inline void SetNumberOfEventsToBeStored(G4int val)` | Sets the number of events to be kept after processing. That is, "val" previous events can be used with the most recent event for digitizing pileup. "val"+1 previous event is deleted. This method must be invoked before starting the event loop. |
| `inline const G4Run* GetCurrentRun() const` | Returns the pointer to the current run. This method is available for 'GeomClosed' and 'EventProc' application states. |
| `inline G4Run* GetNonConstCurrentRun() const` | - |
| `inline const G4Event* GetCurrentEvent() const` | Returns the pointer to the current event. This method is available for 'EventProc' application state. |
| `inline const G4Event* GetPreviousEvent(G4int i) const` | Returns the pointer to the "i" previous event. This method is available for 'EventProc' application state. In case the event loop has not yet reached the requested event, null will be returned. To use this method, SetNumberOfEventsToBeStored() method mentioned above must be invoked previously to the event loop. |
| `if (i >= 1 && i <= n_perviousEventsToBeStored)` | - |
| `auto itr = previousEvents->cbegin()` | - |
| `for (G4int j = 1; j < i; ++j)` | - |
| `} inline void SetRunIDCounter(G4int i)` | Set the run number counter. Initially, the counter is initialized to zero and incremented by one for every BeamOn(). |
| `inline G4int GetNumberOfParallelWorld() const` | - |
| `inline void SetNumberOfEventsToBeProcessed(G4int val)` | - |
| `inline G4int GetNumberOfEventsToBeProcessed() const` | - |
| `inline G4int GetNumberOfSelectEvents() const` | - |
| `inline const G4String& GetSelectMacro() const` | - |
| `inline void SetDCtable(G4DCtable* DCtbl)` | - |
| `inline RMType GetRunManagerType() const` | - |


## Header: `G4MTRunManager.hh` (Target: `G4MTRunManager`)

**Kind**: C++ Class (`G4MTRunManager`) | **Total Extracted Public Methods**: 49

| C++ Method Signature | Comment / Description |
|---|---|
| `G4MTRunManager()` | - |
| `~G4MTRunManager() override` | - |
| `void SetNumberOfThreads(G4int n) override` | - |
| `G4int GetNumberOfThreads() const override` | - |
| `void SetPinAffinity(G4int n = 1)` | - |
| `inline G4int GetPinAffinity() const` | - |
| `void Initialize() override` | Inherited methods to re-implement for MT case |
| `void InitializeEventLoop(G4int n_event, const char* macroFile = nullptr, G4int n_select = -1) override` | - |
| `virtual void InitializeThreadPool()` | - |
| `void TerminateOneEvent() override` | The following do not do anything for this runmanager |
| `void ProcessOneEvent(G4int i_event) override` | - |
| `void ConstructScoringWorlds() override` | - |
| `void RunTermination() override` | - |
| `virtual G4bool SetUpAnEvent(G4Event*, G4long& s1, G4long& s2, G4long& s3, G4bool reseedRequired = true)` | The following method should be invoked by G4WorkerRunManager for each event. False is returned if no more event to be processed. Note: G4Event object must be instantiated by a worker thread. In case no more events remain to be processed, that worker thread must delete that G4Event object. If a worker runs with its own random number sequence, the Boolean flag 'reseedRequired' should be set to false. This is *NOT* allowed for the first event. |
| `virtual G4int SetUpNEvents(G4Event*, G4SeedsQueue* seedsQueue, G4bool reseedRequired = true)` | Same as above method, but seeds are set only once over "eventModulo" events. The return value shows the number of events the caller Worker has to process (between 1 and eventModulo depending on number of events yet to be processed). G4Event object has the event ID of the first event of this bunch. If zero is returned no more events need to be processed, and worker thread must delete that G4Event. Called by Initialize() method. |
| `std::vector<G4String> GetCommandStack()` | This method is invoked just before spawning the threads to collect from UI manager the list of commands that threads will execute. |
| `virtual size_t GetNumberActiveThreads() const` | Returns number of currently active threads. This number may be different from the number of threads currently in running state, e.g. the number returned by: G4Threading::GetNumberOfActiveWorkerThreads() method. |
| `static G4ThreadId GetMasterThreadId()` | - |
| `virtual void ThisWorkerReady()` | Worker threads barrier: this method should be called by each worker when ready to start thread event-loop. This method will return only when all workers are ready. |
| `virtual void ThisWorkerEndEventLoop()` | Worker threads barrier: this method should be called by each worker when worker event loop is terminated. |
| `static G4ScoringManager* GetMasterScoringManager()` | - |
| `static masterWorlds_t& GetMasterWorlds()` | - |
| `static void addWorld(G4int counter, G4VPhysicalVolume* w)` | - |
| `inline const CLHEP::HepRandomEngine* getMasterRandomEngine() const` | - |
| `static G4MTRunManager* GetMasterRunManager()` | Returns the singleton instance of the run manager common to all threads implementing the master behavior |
| `static G4RunManagerKernel* GetMasterRunManagerKernel()` | Returns the singleton instance of the run manager kernel common to all threads |
| `static G4MTRunManagerKernel* GetMTMasterRunManagerKernel()` | - |
| `void SetUserInitialization(G4VUserPhysicsList* userPL) override` | - |
| `void SetUserInitialization(G4VUserDetectorConstruction* userDC) override` | - |
| `void SetUserInitialization(G4UserWorkerInitialization* userInit) override` | - |
| `void SetUserInitialization(G4UserWorkerThreadInitialization* userInit) override` | - |
| `void SetUserInitialization(G4VUserActionInitialization* userInit) override` | - |
| `void SetUserAction(G4UserRunAction* userAction) override` | - |
| `void SetUserAction(G4VUserPrimaryGeneratorAction* userAction) override` | - |
| `void SetUserAction(G4UserEventAction* userAction) override` | - |
| `void SetUserAction(G4UserStackingAction* userAction) override` | - |
| `void SetUserAction(G4UserTrackingAction* userAction) override` | - |
| `void SetUserAction(G4UserSteppingAction* userAction) override` | - |
| `void MergeScores(const G4ScoringManager* localScoringManager)` | To be invoked solely from G4WorkerRunManager to merge the results |
| `void MergeRun(const G4Run* localRun)` | - |
| `virtual void RequestWorkersProcessCommandsStack()` | Called to force workers to request and process the UI commands stack This will block untill all workers have processed UI commands |
| `virtual void ThisWorkerProcessCommandsStackDone()` | Called by workers to signal to master it has completed processing of UI commands |
| `virtual WorkerActionRequest ThisWorkerWaitForNextAction()` | Worker thread barrier: this method should be used by workers' run manager to wait, after an event loop for the next action to be performed (for example execute a new run). This returns the action to be performed. |
| `inline void SetEventModulo(G4int i = 1)` | - |
| `inline G4int GetEventModulo() const` | - |
| `void AbortRun(G4bool softAbort = false) override` | - |
| `void AbortEvent() override` | - |
| `static G4int SeedOncePerCommunication()` | - |
| `static void SetSeedOncePerCommunication(G4int val)` | - |


## Header: `G4VUserActionInitialization.hh` (Target: `G4VUserActionInitialization`)

**Kind**: C++ Class (`G4VUserActionInitialization`) | **Total Extracted Public Methods**: 5

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VUserActionInitialization() = default` | - |
| `virtual ~G4VUserActionInitialization() = default` | - |
| `virtual void Build() const = 0` | Virtual method to be implemented by the user to instantiate user action class objects. |
| `virtual void BuildForMaster() const` | Virtual method to be implemented by the user to instantiate user run action class object to be used by G4MTRunManager. This method is not invoked in the sequential mode. The user should not use this method to instantiate user action classes except for user run action. |
| `virtual G4VSteppingVerbose* InitializeSteppingVerbose() const` | Virtual method to be implemented by the user if having a concrete SteppingVerbose class to be used by the worker thread. In this case one should instantiate a SteppingVerbose in the concrete implementation of this method and return its pointer. If this method is not implemented, the default G4SteppingVerbose will be used. Please note that this method affects only for the worker thread. |


## Header: `G4UserStackingAction.hh` (Target: `G4UserStackingAction`)

**Kind**: C++ Class (`G4UserStackingAction`) | **Total Extracted Public Methods**: 6

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UserStackingAction()` | - |
| `virtual ~G4UserStackingAction() = default` | - |
| `inline void SetStackManager(G4StackManager* value)` | - |
| `virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* aTrack)` | --------------------------------------------------------------- vitual methods to be implemented by user --------------------------------------------------------------- |
| `virtual void NewStage()` | Reply G4ClassificationOfNewTrack determined by the newly coming G4Track.  enum G4ClassificationOfNewTrack { fUrgent,    // put into the urgent stack fWaiting,   // put into the waiting stack fPostpone,  // postpone to the next event fKill       // kill without stacking }  The parent_ID of the track indicates the origin of it:  G4int parent_ID = aTrack->get_parentID();  parent_ID = 0 : primary particle > 0 : secondary particle < 0 : postponed from the previous event |
| `virtual void PrepareNewEvent()` | This method is called by G4StackManager when the urgentStack becomes empty and contents in the waitingStack are transferred to the urgentStack. Note that this method is not called at the begining of each event, but "PrepareNewEvent()" is called.  In case re-classification of the stacked tracks is needed, use the following method to request to G4StackManager:  stackManager->ReClassify();  All of the stacked tracks in the waitingStack will be re-classified by "ClassifyNewTrack()" method. To abort current event, use the following method:  stackManager->clear();  Note that this way is valid and safe only for the case it is called from this user class. The more global way of event abortion is:  G4UImanager * UImanager = G4UImanager::GetUIpointer(); UImanager->ApplyCommand("/event/abort"); |


## Header: `G4ClassificationOfNewTrack.hh` (Target: `G4ClassificationOfNewTrack`)

**Kind**: Enumeration (`enum G4ClassificationOfNewTrack`)

| Enumerator | Description / Semantic Role |
|---|---|
| `fUrgent` | State flag / Stack status |
| `fWaiting` | State flag / Stack status |
| `fPostpone` | State flag / Stack status |
| `fKill` | State flag / Stack status |
| `fWaiting_1` | State flag / Stack status |
| `fWaiting_2` | State flag / Stack status |
| `fWaiting_3` | State flag / Stack status |
| `fWaiting_4` | State flag / Stack status |
| `fWaiting_5` | State flag / Stack status |
| `fWaiting_6` | State flag / Stack status |
| `fWaiting_7` | State flag / Stack status |
| `fWaiting_8` | State flag / Stack status |
| `fWaiting_9` | State flag / Stack status |
| `fWaiting_10` | State flag / Stack status |
| `fSubEvent_1` | State flag / Stack status |
| `fSubEvent_2` | State flag / Stack status |
| `fSubEvent_3` | State flag / Stack status |
| `fSubEvent_4` | State flag / Stack status |
| `fSubEvent_5` | State flag / Stack status |
| `fSubEvent_6` | State flag / Stack status |
| `fSubEvent_7` | State flag / Stack status |
| `fSubEvent_8` | State flag / Stack status |
| `fSubEvent_9` | State flag / Stack status |
| `fSubEvent_A` | State flag / Stack status |
| `fSubEvent_B` | State flag / Stack status |
| `fSubEvent_C` | State flag / Stack status |
| `fSubEvent_D` | State flag / Stack status |
| `fSubEvent_E` | State flag / Stack status |
| `fSubEvent_F` | State flag / Stack status |


## Header: `G4VHit.hh` (Target: `G4VHit`)

**Kind**: C++ Class (`G4VHit`) | **Total Extracted Public Methods**: 7

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VHit() = default` | - |
| `virtual ~G4VHit() = default` | - |
| `G4bool operator==(const G4VHit&) const` | - |
| `virtual void Draw()` | - |
| `virtual void Print()` | - |
| `virtual const std::map<G4String, G4AttDef>* GetAttDefs() const` | Returns pointer to map of attribute definitions for this hit Caller is responsible for checking validity of returned pointer. See `G4Trajectory` for a concrete implementation. |
| `virtual std::vector<G4AttValue>* CreateAttValues() const` | Returns pointer to list of attributes for this hit Each attribute must correspond to a definition returned by `GetAffDefs`. Caller is responsible for checking validity of returned pointer. See `G4Trajectory` for a concrete implementation, and `G4VTrajectory::ShowTrajectory` for an example of its use. |


## Header: `G4HCofThisEvent.hh` (Target: `G4HCofThisEvent`)

**Kind**: C++ Class (`G4HCofThisEvent`) | **Total Extracted Public Methods**: 13

| C++ Method Signature | Comment / Description |
|---|---|
| `G4HCofThisEvent()` | - |
| `explicit G4HCofThisEvent(G4int cap)` | - |
| `~G4HCofThisEvent()` | - |
| `G4HCofThisEvent(const G4HCofThisEvent&)` | - |
| `G4HCofThisEvent& operator=(const G4HCofThisEvent&)` | - |
| `inline void* operator new(size_t)` | - |
| `inline void operator delete(void* anHCoTE)` | - |
| `void AddHitsCollection(G4int HCID, G4VHitsCollection* aHC)` | - |
| `inline G4VHitsCollection* GetHC(G4int i)` | Returns a pointer to a hits collection. Null will be returned if the particular collection is not stored at the current event. The integer argument is ID number which is assigned by G4SDManager and the number can be obtained by G4SDManager::GetHitsCollectionID() method. |
| `inline G4int GetNumberOfCollections()` | Return number of hits collections which are stored in this class object. |
| `for (const G4VHitsCollection* h : *HC)` | - |
| `if (h != nullptr) n++` | - |
| `} inline size_t GetCapacity()` | - |


## Header: `G4Cons.hh` (Target: `G4Cons`)

**Kind**: C++ Class (`G4Cons`) | **Total Extracted Public Methods**: 40

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Cons(const G4String& pName, G4double pRmin1, G4double pRmax1, G4double pRmin2, G4double pRmax2, G4double pDz, G4double pSPhi, G4double pDPhi)` | - |
| `~G4Cons() override` | Constructs a cone with the given name and dimensions |
| `inline G4double GetInnerRadiusMinusZ() const` | Destructor Accessors |
| `inline G4double GetOuterRadiusMinusZ() const` | - |
| `inline G4double GetInnerRadiusPlusZ() const` | - |
| `inline G4double GetOuterRadiusPlusZ() const` | - |
| `inline G4double GetZHalfLength() const` | - |
| `inline G4double GetStartPhiAngle() const` | - |
| `inline G4double GetDeltaPhiAngle() const` | - |
| `inline G4double GetSinStartPhi() const` | - |
| `inline G4double GetCosStartPhi() const` | - |
| `inline G4double GetSinEndPhi() const` | - |
| `inline G4double GetCosEndPhi() const` | - |
| `inline void SetInnerRadiusMinusZ (G4double Rmin1 )` | Modifiers |
| `inline void SetOuterRadiusMinusZ (G4double Rmax1 )` | - |
| `inline void SetInnerRadiusPlusZ (G4double Rmin2 )` | - |
| `inline void SetOuterRadiusPlusZ (G4double Rmax2 )` | - |
| `inline void SetZHalfLength (G4double newDz )` | - |
| `inline void SetStartPhiAngle (G4double newSPhi, G4bool trig=true)` | - |
| `inline void SetDeltaPhiAngle (G4double newDPhi)` | - |
| `inline G4double GetCubicVolume() override` | Other methods for solid |
| `inline G4double GetSurfaceArea() override` | - |
| `void ComputeDimensions( G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep) override` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | - |
| `G4bool CalculateExtent(const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pMin, G4double& pMax) const override` | - |
| `EInside Inside( const G4ThreeVector& p ) const override` | - |
| `G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const override` | - |
| `G4double DistanceToIn (const G4ThreeVector& p, const G4ThreeVector& v) const override` | - |
| `G4double DistanceToIn (const G4ThreeVector& p) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p, const G4ThreeVector& v, const G4bool calcNorm = false, G4bool* validNorm = nullptr, G4ThreeVector* n = nullptr) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p) const override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4ThreeVector GetPointOnSurface() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `std::ostream& StreamInfo(std::ostream& os) const override` | - |
| `void DescribeYourselfTo( G4VGraphicsScene& scene ) const override` | Visualisation functions |
| `G4Polyhedron* CreatePolyhedron() const override` | - |
| `G4Cons(__void__&)` | - |
| `G4Cons(const G4Cons& rhs)` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4Cons& operator=(const G4Cons& rhs)` | - |


## Header: `G4Polycone.hh` (Target: `G4Polycone`)

**Kind**: C++ Class (`G4Polycone`) | **Total Extracted Public Methods**: 31

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Polycone( const G4String& name, G4double phiStart, // initial phi starting angle G4double phiTotal, // total phi angle G4int numZPlanes, // number of z planes const G4double zPlane[], // position of z planes const G4double rInner[], // tangent distance to inner surface const G4double rOuter[] ); // tangent distance to outer surface` | - |
| `G4Polycone( const G4String& name, G4double phiStart, // initial phi starting angle G4double phiTotal, // total phi angle G4int numRZ, // number corners in r,z space const G4double r[], // r coordinate of these corners const G4double z[] ); // z coordinate of these corners` | - |
| `~G4Polycone() override` | - |
| `EInside Inside( const G4ThreeVector& p ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p, const G4ThreeVector& v ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p ) const override` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | - |
| `G4bool CalculateExtent(const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pmin, G4double& pmax) const override` | - |
| `G4double GetCubicVolume() override` | - |
| `G4double GetSurfaceArea() override` | - |
| `G4ThreeVector GetPointOnSurface() const override` | - |
| `void ComputeDimensions( G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep ) override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `std::ostream& StreamInfo(std::ostream& os) const override` | - |
| `G4Polyhedron* CreatePolyhedron() const override` | - |
| `G4bool Reset()` | - |
| `inline G4double GetStartPhi() const` | Accessors |
| `inline G4double GetEndPhi() const` | - |
| `inline G4double GetSinStartPhi() const` | - |
| `inline G4double GetCosStartPhi() const` | - |
| `inline G4double GetSinEndPhi() const` | - |
| `inline G4double GetCosEndPhi() const` | - |
| `inline G4bool IsOpen() const` | - |
| `inline G4int GetNumRZCorner() const` | - |
| `inline G4PolyconeSideRZ GetCorner(G4int index) const` | - |
| `inline G4PolyconeHistorical* GetOriginalParameters() const` | - |
| `inline void SetOriginalParameters(G4PolyconeHistorical* pars)` | - |
| `G4Polycone(__void__&)` | - |
| `G4Polycone( const G4Polycone& source )` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4Polycone &operator=( const G4Polycone& source )` | - |


## Header: `G4UnionSolid.hh` (Target: `G4UnionSolid`)

**Kind**: C++ Class (`G4UnionSolid`) | **Total Extracted Public Methods**: 21

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UnionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB )` | - |
| `G4UnionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB , G4RotationMatrix* rotMatrix, const G4ThreeVector& transVector )` | - |
| `G4UnionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB , const G4Transform3D& transform )` | - |
| `~G4UnionSolid() override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `G4UnionSolid(__void__&)` | - |
| `G4UnionSolid(const G4UnionSolid& rhs)` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4UnionSolid& operator=(const G4UnionSolid& rhs)` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | Copy constructor and assignment operator. |
| `G4bool CalculateExtent( const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pMin, G4double& pMax ) const override` | - |
| `EInside Inside( const G4ThreeVector& p ) const override` | - |
| `G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p, const G4ThreeVector& v ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p ) const override` | - |
| `G4double DistanceToOut( const G4ThreeVector& p, const G4ThreeVector& v, const G4bool calcNorm = false, G4bool* validNorm = nullptr, G4ThreeVector* n = nullptr ) const override` | - |
| `G4double DistanceToOut( const G4ThreeVector& p ) const override` | - |
| `void ComputeDimensions( G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep ) override` | - |
| `void DescribeYourselfTo ( G4VGraphicsScene& scene ) const override` | - |
| `G4Polyhedron* CreatePolyhedron () const override` | - |
| `G4double GetCubicVolume() final` | - |


## Header: `G4IntersectionSolid.hh` (Target: `G4IntersectionSolid`)

**Kind**: C++ Class (`G4IntersectionSolid`) | **Total Extracted Public Methods**: 20

| C++ Method Signature | Comment / Description |
|---|---|
| `G4IntersectionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB )` | - |
| `G4IntersectionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB, G4RotationMatrix* rotMatrix, const G4ThreeVector& transVector )` | - |
| `G4IntersectionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB, const G4Transform3D& transform )` | - |
| `~G4IntersectionSolid() override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `G4IntersectionSolid(__void__&)` | - |
| `G4IntersectionSolid(const G4IntersectionSolid& rhs)` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4IntersectionSolid& operator=(const G4IntersectionSolid& rhs)` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | Copy constructor and assignment operator. |
| `G4bool CalculateExtent( const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pMin, G4double& pMax) const override` | - |
| `EInside Inside( const G4ThreeVector& p ) const override` | - |
| `G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p, const G4ThreeVector& v ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p) const override` | - |
| `G4double DistanceToOut( const G4ThreeVector& p, const G4ThreeVector& v, const G4bool calcNorm=false, G4bool *validNorm=nullptr, G4ThreeVector *n=nullptr ) const override` | - |
| `G4double DistanceToOut( const G4ThreeVector& p ) const override` | - |
| `void ComputeDimensions( G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep ) override` | - |
| `void DescribeYourselfTo ( G4VGraphicsScene& scene ) const override` | - |
| `G4Polyhedron* CreatePolyhedron () const override` | - |


## Header: `G4FieldManager.hh` (Target: `G4FieldManager`)

**Kind**: C++ Class (`G4FieldManager`) | **Total Extracted Public Methods**: 29

| C++ Method Signature | Comment / Description |
|---|---|
| `public: // with description G4FieldManager(G4Field* detectorField = nullptr, G4ChordFinder* pChordFinder = nullptr, G4bool b = true ); // fieldChangesEnergy is taken from field` | - |
| `G4FieldManager(G4MagneticField* detectorMagneticField)` | General constructor for any field. -> Must be set with field and chordfinder for use. |
| `virtual ~G4FieldManager()` | Creates ChordFinder -> Assumes pure magnetic field (so energy constant) |
| `G4FieldManager(const G4FieldManager&) = delete` | - |
| `G4FieldManager& operator=(const G4FieldManager&) = delete` | - |
| `G4bool SetDetectorField(G4Field* detectorField, G4int failMode = 0)` | - |
| `inline void ProposeDetectorField(G4Field* detectorField)` | Pushes the field to the equation. Failure to push the field (due to absence of a chord finder, driver, stepper or equation) is - '0' = quiet      : Do not complain if chordFinder == 0 (It will still warn for other error.) - '1' = warn       : a warning if anything is missing - '2'/else = FATAL : a fatal error for all other values. Returns success (true) or failure (false) |
| `inline void ChangeDetectorField(G4Field* detectorField)` | Pushes the field to this class only -- no further. Should be used  to initialise this field, only *before* creating the chord finder and its dependent classes. User is then responsible to ensure that: i) an equation, stepper, driver and chord finder are created ii) this field is used by the equation. |
| `inline const G4Field* GetDetectorField() const` | Pushes the field to the equation ( & keeps its address ) Can be used only once the equation, stepper, driver and chord finder have all been created.  Else it is an error. |
| `inline G4bool DoesFieldExist() const` | - |
| `void CreateChordFinder(G4MagneticField* detectorMagField)` | Set, get and check the field object |
| `inline void SetChordFinder(G4ChordFinder* aChordFinder)` | - |
| `inline G4ChordFinder* GetChordFinder()` | - |
| `inline const G4ChordFinder* GetChordFinder() const` | - |
| `virtual void ConfigureForTrack( const G4Track * )` | Create, set or get the associated Chord Finder |
| `public: // with description inline G4double GetDeltaIntersection() const` | Setup the choice of the configurable parameters relying on the current track's energy, particle identity, .. Note: in addition to the values of member variables, a user can use this to change the ChordFinder, the field, ... |
| `inline G4double GetDeltaOneStep() const` | Accuracy for boundary intersection. |
| `inline void SetAccuraciesWithDeltaOneStep(G4double valDeltaOneStep)` | Accuracy for one tracking/physics step. |
| `inline void SetDeltaOneStep(G4double valueD1step)` | Sets both accuracies, maintaining a fixed ratio for accuracies of volume Intersection and Integration (in One Step) |
| `inline void SetDeltaIntersection(G4double valueDintersection)` | Set accuracy for integration of one step.   (only) |
| `inline G4double GetMinimumEpsilonStep() const` | Set accuracy of  intersection of a volume.  (only) |
| `G4bool SetMinimumEpsilonStep( G4double newEpsMin )` | - |
| `inline G4double GetMaximumEpsilonStep() const` | Minimum for Relative accuracy of a Step |
| `G4bool SetMaximumEpsilonStep( G4double newEpsMax )` | - |
| `inline G4bool DoesFieldChangeEnergy() const` | Maximum for Relative accuracy of a Step |
| `inline void SetFieldChangesEnergy(G4bool value)` | - |
| `virtual G4FieldManager* Clone() const` | For electric field this should be true For magnetic field this should be false |
| `static G4double GetMaxAcceptedEpsilon()` | Needed for multi-threading, create a clone of this object |
| `static G4bool SetMaxAcceptedEpsilon(G4double maxEps, G4bool softFail= false)` | - |


## Header: `G4UniformMagField.hh` (Target: `G4UniformMagField`)

**Kind**: C++ Class (`G4UniformMagField`) | **Total Extracted Public Methods**: 9

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UniformMagField(const G4ThreeVector& FieldVector)` | - |
| `G4UniformMagField(G4double vField, G4double vTheta, G4double vPhi)` | A field with value equal to FieldVector. |
| `~G4UniformMagField() override` | - |
| `G4UniformMagField(const G4UniformMagField& p)` | - |
| `G4UniformMagField& operator = (const G4UniformMagField& p)` | - |
| `void GetFieldValue(const G4double yTrack[4], G4double* MagField) const override` | Copy constructor and assignment operator. |
| `void SetFieldValue(const G4ThreeVector& newFieldValue)` | - |
| `G4ThreeVector GetConstantFieldValue() const` | - |
| `G4Field* Clone() const override` | Return the field value |


## Header: `G4OpticalPhysics.hh` (Target: `G4OpticalPhysics`)

**Kind**: C++ Class (`G4OpticalPhysics`) | **Total Extracted Public Methods**: 5

| C++ Method Signature | Comment / Description |
|---|---|
| `G4OpticalPhysics(G4int verbose = 0, const G4String& name = "Optical")` | - |
| `~G4OpticalPhysics() override` | - |
| `void PrintStatistics() const` | - |
| `G4OpticalPhysics(const G4OpticalPhysics& right) = delete` | - |
| `G4OpticalPhysics& operator=(const G4OpticalPhysics& right) = delete` | - |


## Header: `G4OpticalParameters.hh` (Target: `G4OpticalParameters`)

**Kind**: C++ Class (`G4OpticalParameters`) | **Total Extracted Public Methods**: 50

| C++ Method Signature | Comment / Description |
|---|---|
| `static G4OpticalParameters* Instance()` | - |
| `~G4OpticalParameters()` | - |
| `void SetDefaults()` | - |
| `void StreamInfo(std::ostream& os) const` | printing |
| `void Dump() const` | - |
| `friend std::ostream& operator<<(std::ostream& os, const G4OpticalParameters&)` | - |
| `void SetVerboseLevel(G4int)` | - |
| `G4int GetVerboseLevel() const` | - |
| `void SetProcessActivation(const G4String&, G4bool)` | - |
| `G4bool GetProcessActivation(const G4String&) const` | - |
| `void SetCerenkovMaxPhotonsPerStep(G4int)` | Cerenkov |
| `G4int GetCerenkovMaxPhotonsPerStep() const` | - |
| `void SetCerenkovVerboseLevel(G4int)` | - |
| `G4int GetCerenkovVerboseLevel() const` | - |
| `void SetCerenkovMaxBetaChange(G4double)` | - |
| `G4double GetCerenkovMaxBetaChange() const` | - |
| `void SetCerenkovTrackSecondariesFirst(G4bool)` | - |
| `G4bool GetCerenkovTrackSecondariesFirst() const` | - |
| `void SetCerenkovStackPhotons(G4bool)` | - |
| `G4bool GetCerenkovStackPhotons() const` | - |
| `void SetScintByParticleType(G4bool)` | Scintillation |
| `G4bool GetScintByParticleType() const` | - |
| `void SetScintTrackInfo(G4bool)` | - |
| `G4bool GetScintTrackInfo() const` | - |
| `void SetScintTrackSecondariesFirst(G4bool)` | - |
| `G4bool GetScintTrackSecondariesFirst() const` | - |
| `void SetScintFiniteRiseTime(G4bool)` | - |
| `G4bool GetScintFiniteRiseTime() const` | - |
| `void SetScintStackPhotons(G4bool)` | - |
| `G4bool GetScintStackPhotons() const` | - |
| `void SetScintVerboseLevel(G4int)` | - |
| `G4int GetScintVerboseLevel() const` | - |
| `void SetWLSTimeProfile(const G4String&)` | WLS |
| `G4String GetWLSTimeProfile() const` | - |
| `void SetWLSVerboseLevel(G4int)` | - |
| `G4int GetWLSVerboseLevel() const` | - |
| `void SetWLS2TimeProfile(const G4String&)` | WLS2 |
| `G4String GetWLS2TimeProfile() const` | - |
| `void SetWLS2VerboseLevel(G4int)` | - |
| `G4int GetWLS2VerboseLevel() const` | - |
| `void SetBoundaryVerboseLevel(G4int)` | boundary |
| `G4int GetBoundaryVerboseLevel() const` | - |
| `void SetBoundaryInvokeSD(G4bool)` | - |
| `G4bool GetBoundaryInvokeSD() const` | - |
| `void SetAbsorptionVerboseLevel(G4int)` | absorption |
| `G4int GetAbsorptionVerboseLevel() const` | - |
| `void SetRayleighVerboseLevel(G4int)` | rayleigh |
| `G4int GetRayleighVerboseLevel() const` | - |
| `void SetMieVerboseLevel(G4int)` | mie |
| `G4int GetMieVerboseLevel() const` | - |


## Header: `G4OpticalSurface.hh` (Target: `G4OpticalSurface`)

**Kind**: C++ Class (`G4OpticalSurface`) | **Total Extracted Public Methods**: 33

| C++ Method Signature | Comment / Description |
|---|---|
| `G4OpticalSurface(const G4String& name, G4OpticalSurfaceModel model = glisur, G4OpticalSurfaceFinish finish = polished, G4SurfaceType type = dielectric_dielectric, G4double value = 1.0)` | Constructor of an optical surface object. |
| `~G4OpticalSurface() override` | - |
| `G4OpticalSurface(const G4OpticalSurface& right)` | - |
| `G4OpticalSurface& operator=(const G4OpticalSurface& right)` | - |
| `G4bool operator==(const G4OpticalSurface& right) const` | - |
| `G4bool operator!=(const G4OpticalSurface& right) const` | - |
| `void SetType(const G4SurfaceType& type) override` | - |
| `inline G4OpticalSurfaceFinish GetFinish() const` | Returns the optical surface finish. |
| `void SetFinish(const G4OpticalSurfaceFinish)` | Sets the optical surface finish. |
| `inline G4OpticalSurfaceModel GetModel() const` | Returns the optical surface model used. |
| `inline void SetModel(const G4OpticalSurfaceModel model)` | Sets the optical surface model to be followed. |
| `inline G4double GetSigmaAlpha() const` | Returns an unified model surface parameter. |
| `inline void SetSigmaAlpha(const G4double s_a)` | Sets an unified model surface parameter. |
| `G4double GetPolish() const` | Returns the optical surface polish type. |
| `inline void SetPolish(const G4double plsh)` | Sets the optical surface polish type. |
| `inline G4MaterialPropertiesTable* GetMaterialPropertiesTable() const` | Retrieves the pointer of the G4MaterialPropertiesTable attached to optical surface. |
| `} inline void SetMaterialPropertiesTable(G4MaterialPropertiesTable* anMPT)` | Attaches a G4MaterialPropertiesTable to the optical surface. |
| `} void DumpInfo() const` | Prints information about the optical surface. |
| `void ReadDataFile()` | call the correct ReadXXXFile |
| `void ReadCompressedFile(const G4String&, std::istringstream&)` | read a zlib-compressed file |
| `void ReadLUTFile()` | Method to read the Look-Up-Table into array AngularDistribution |
| `inline G4double GetAngularDistributionValue(G4int, G4int, G4int)` | for DAVIS model |
| `inline G4double GetAngularDistributionValueLUT(G4int)` | Returns the AngularDistributionValue |
| `void ReadLUTDAVISFile()` | Method to read the Davis Look-Up-Table into array AngularDistribution |
| `void ReadReflectivityLUTFile()` | Method to read the Look-Up-Table for reflectivity |
| `inline G4double GetReflectivityLUTValue(G4int)` | Returns the reflectivity value from the Davis Look-Up-Table |
| `G4int GetInmax() const` | Returns the number of lines in the Davis Look-Up-Table |
| `G4int GetLUTbins() const` | Returns the number of probability values per incidentangle |
| `G4int GetRefMax() const` | Returns the number of reflectivity values per angle |
| `G4int GetThetaIndexMax() const` | - |
| `G4int GetPhiIndexMax() const` | - |
| `void ReadDichroicFile()` | Method to read the dichroic surface data file into Dichroic |
| `inline G4Physics2DVector* GetDichroicVector()` | - |


## Header: `G4LogicalBorderSurface.hh` (Target: `G4LogicalBorderSurface`)

**Kind**: C++ Class (`G4LogicalBorderSurface`) | **Total Extracted Public Methods**: 17

| C++ Method Signature | Comment / Description |
|---|---|
| `G4LogicalBorderSurface( const G4String& name, G4VPhysicalVolume* vol1, G4VPhysicalVolume* vol2, G4SurfaceProperty* surfaceProperty )` | - |
| `~G4LogicalBorderSurface() override` | - |
| `G4LogicalBorderSurface(const G4LogicalBorderSurface&) = delete` | Constructor and destructor |
| `G4LogicalBorderSurface& operator=(const G4LogicalBorderSurface&) = delete` | - |
| `G4bool operator==( const G4LogicalBorderSurface& right ) const` | Copy constructor and assignment operator not allowed. |
| `G4bool operator!=( const G4LogicalBorderSurface& right ) const` | - |
| `static G4LogicalBorderSurface* GetSurface( const G4VPhysicalVolume* vol1, const G4VPhysicalVolume* vol2 )` | Operators |
| `inline void SetPhysicalVolumes( G4VPhysicalVolume* vol1, G4VPhysicalVolume* vol2 )` | - |
| `inline const G4VPhysicalVolume* GetVolume1() const` | - |
| `inline const G4VPhysicalVolume* GetVolume2() const` | - |
| `inline std::size_t GetIndex() const` | - |
| `inline void SetVolume1( G4VPhysicalVolume* vol1 )` | Generic accessors |
| `inline void SetVolume2( G4VPhysicalVolume* vol2 )` | - |
| `static void CleanSurfaceTable()` | To use with care! |
| `static const G4LogicalBorderSurfaceTable* GetSurfaceTable()` | - |
| `static std::size_t GetNumberOfBorderSurfaces()` | - |
| `static void DumpInfo()` | - |


## Header: `G4GeometrySampler.hh` (Target: `G4GeometrySampler`)

**Kind**: C++ Class (`G4GeometrySampler`) | **Total Extracted Public Methods**: 14

| C++ Method Signature | Comment / Description |
|---|---|
| `public: // with description explicit G4GeometrySampler(G4VPhysicalVolume *worldvolume, const G4String &particlename)` | - |
| `explicit G4GeometrySampler(G4String worldvolumeName, const G4String &particlename)` | - |
| `virtual ~G4GeometrySampler()` | - |
| `virtual void PrepareImportanceSampling(G4VIStore* istore, const G4VImportanceAlgorithm *ialg)` | virtual void PrepareScoring(G4VScorer *Scorer); |
| `virtual void PrepareWeightRoulett(G4double wsurvive, G4double wlimit, G4double isource)` | - |
| `virtual void PrepareWeightWindow(G4VWeightWindowStore *wwstore, G4VWeightWindowAlgorithm *wwAlg, G4PlaceOfAction placeOfAction)` | - |
| `virtual void Configure()` | - |
| `virtual void AddProcess()` | - |
| `virtual void ClearSampling()` | - |
| `virtual G4bool IsConfigured() const` | - |
| `void SetParallel(G4bool paraflag)` | - |
| `void SetWorld(const G4VPhysicalVolume* world)` | - |
| `void SetParticle(const G4String &particlename)` | - |
| `inline G4String GetParticleName()` | - |


## Header: `G4WeightWindowStore.hh` (Target: `G4WeightWindowStore`)

**Kind**: C++ Class (`G4WeightWindowStore`) | **Total Extracted Public Methods**: 12

| C++ Method Signature | Comment / Description |
|---|---|
| `public: // with description static G4WeightWindowStore* GetInstance()` | - |
| `static G4WeightWindowStore* GetInstance(const G4String& ParallelWorldName)` | return ptr to singleton instance of the class |
| `G4double GetLowerWeight(const G4GeometryCell& gCell, G4double partEnergy) const override` | return ptr to singleton instance of the class |
| `G4bool IsKnown(const G4GeometryCell &gCell) const override` | derive a lower weight bound value of a "cell" addressed by a G4GeometryCell and the corresponding energy from the store |
| `void Clear()` | returns true if the gCell is in the store, else false |
| `void SetWorldVolume()` | - |
| `void SetParallelWorldVolume(const G4String& paraName)` | set a pointer to the world volume of the weightwindow geometry |
| `const G4VPhysicalVolume& GetWorldVolume() const override` | set a pointer to parallel world volume of the weightwindow geometry |
| `virtual const G4VPhysicalVolume* GetParallelWorldVolumePointer() const` | return a reference to the world volume of the weightwindow geometry |
| `void AddLowerWeights(const G4GeometryCell& gCell, const std::vector<G4double>& lowerWeights)` | return a pointer to parallel world volume of the weightwindow geometry |
| `void AddUpperEboundLowerWeightPairs(const G4GeometryCell& gCell, const G4UpperEnergyToLowerWeightMap& enWeMap)` | add lower weights. Only if general upper energy bounds have been set |
| `void SetGeneralUpperEnergyBounds(const std::set<G4double, std::less<G4double> >& enBounds)` | set upper energy - lower weight pairs for a cell |

