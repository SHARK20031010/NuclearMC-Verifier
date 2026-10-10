# Partition 1: Stepping, Tracking & Trajectory Lifecycle APIs

> **Partition Scope**: Lifecycle APIs for particle tracking, step-level evaluation, state inspection, and stepping/tracking hooks.

> **Header Base Path**: `/home/shark/geant4/11.2.2-install/include/Geant4`

---

## Header: `G4Step.hh` (Target: `G4Step`)

**Kind**: C++ Class (`G4Step`) | **Total Extracted Public Methods**: 47

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Step()` | - |
| `~G4Step()` | - |
| `G4Step(const G4Step&)` | Constructor/Destructor |
| `G4Step& operator=(const G4Step&)` | - |
| `G4Track* GetTrack() const` | Copy Constructor and assignment operator |
| `void SetTrack(G4Track* value)` | - |
| `G4StepPoint* GetPreStepPoint() const` | Current track |
| `void SetPreStepPoint(G4StepPoint* value)` | - |
| `G4StepPoint* ResetPreStepPoint(G4StepPoint* value=nullptr)` | - |
| `G4StepPoint* GetPostStepPoint() const` | Pre-Step points If Set method is invoked, the previous StepPoint object is deleted. If Reset method is invoked, the previous StepPoint object is not deleted but its pointer is returned. Thus it's the caller's responsibility to properly delete it. |
| `void SetPostStepPoint(G4StepPoint* value)` | - |
| `G4StepPoint* ResetPostStepPoint(G4StepPoint* value=nullptr)` | - |
| `G4double GetStepLength() const` | Post-Step points If Set method is invoked, the previous StepPoint object is deleted. If Reset method is invoked, the previous StepPoint object is not deleted but its pointer is returned. Thus it's the caller's responsibility to properly delete it. |
| `void SetStepLength(G4double value)` | - |
| `G4double GetTotalEnergyDeposit() const` | Before the end of the AlongStepDoIt loop, StepLength keeps the initial value which is determined by the shortest geometrical Step proposed by a physics process. After finishing the AlongStepDoIt, it will be set equal to 'StepLength' in G4Step |
| `void SetTotalEnergyDeposit(G4double value)` | - |
| `G4double GetNonIonizingEnergyDeposit() const` | Total energy deposit |
| `void SetNonIonizingEnergyDeposit(G4double value)` | - |
| `G4SteppingControl GetControlFlag() const` | Total non-ionizing energy deposit |
| `void SetControlFlag(G4SteppingControl StepControlFlag)` | - |
| `void AddTotalEnergyDeposit(G4double value)` | Control flag for stepping |
| `void ResetTotalEnergyDeposit()` | - |
| `void AddNonIonizingEnergyDeposit(G4double value)` | Manipulation of total energy deposit |
| `void ResetNonIonizingEnergyDeposit()` | - |
| `G4bool IsFirstStepInVolume() const` | Manipulation of non-ionizing energy deposit |
| `G4bool IsLastStepInVolume() const` | - |
| `void SetFirstStepFlag()` | - |
| `void ClearFirstStepFlag()` | - |
| `void SetLastStepFlag()` | - |
| `void ClearLastStepFlag()` | - |
| `G4ThreeVector GetDeltaPosition() const` | Get/Set/Clear flag for initial/last step NOTE: flags are not used |
| `G4double GetDeltaTime() const` | - |
| `G4ThreeVector GetDeltaMomentum() const` | Difference of position, time, momentum and energy |
| `G4double GetDeltaEnergy() const` | - |
| `void InitializeStep(G4Track* aValue)` | These methods will be deleted NOTE: use  GetTotalEnergyDeposit() to obtain energy loss in material |
| `void UpdateTrack()` | Initialize contents of G4Step |
| `void CopyPostToPreStepPoint()` | Update track by using G4Step information |
| `G4Polyline* CreatePolyline() const` | Copy PostStepPoint to PreStepPoint |
| `inline void SetPointerToVectorOfAuxiliaryPoints(std::vector<G4ThreeVector>* vec)` | For visualization |
| `inline std::vector<G4ThreeVector>* GetPointerToVectorOfAuxiliaryPoints() const` | - |
| `std::size_t GetNumberOfSecondariesInCurrentStep() const` | Auxiliary points modifiers --- Secondary buckets --- |
| `const std::vector<const G4Track*>* GetSecondaryInCurrentStep() const` | Secondaries in the current step |
| `const G4TrackVector* GetSecondary() const` | - |
| `G4TrackVector* GetfSecondary()` | - |
| `G4TrackVector* NewSecondaryVector()` | - |
| `void DeleteSecondaryVector()` | NOTE: Secondary bucket of the Step contains all secondaries during tracking the current track (i.e. NOT secondaries produced in the current step) all these methods give same object (i.e. G4TrackVector  ) but 2nd one will create bucket in addition |
| `void SetSecondary(G4TrackVector* value)` | Just delete secondary bucket NOTE: G4Track objects inside the bucket are not deleted |


## Header: `G4StepPoint.hh` (Target: `G4StepPoint`)

**Kind**: C++ Class (`G4StepPoint`) | **Total Extracted Public Methods**: 55

| C++ Method Signature | Comment / Description |
|---|---|
| `G4StepPoint() = default` | - |
| `~G4StepPoint()= default` | - |
| `G4StepPoint(const G4StepPoint&) = default` | Constructor/Destructor |
| `G4StepPoint& operator=(const G4StepPoint&)` | - |
| `const G4ThreeVector& GetPosition() const` | Copy Constructor and assignment operator |
| `void SetPosition(const G4ThreeVector& aValue)` | - |
| `void AddPosition(const G4ThreeVector& aValue)` | - |
| `G4double GetLocalTime() const` | Position |
| `void SetLocalTime(const G4double aValue)` | - |
| `void AddLocalTime(const G4double aValue)` | - |
| `G4double GetGlobalTime() const` | Time since the track is created |
| `void SetGlobalTime(const G4double aValue)` | - |
| `void AddGlobalTime(const G4double aValue)` | - |
| `G4double GetProperTime() const` | Time since the event in which the track belongs is created |
| `void SetProperTime(const G4double aValue)` | - |
| `void AddProperTime(const G4double aValue)` | - |
| `const G4ThreeVector& GetMomentumDirection() const` | Proper time of the particle |
| `void SetMomentumDirection(const G4ThreeVector& aValue)` | - |
| `void AddMomentumDirection(const G4ThreeVector& aValue)` | - |
| `G4ThreeVector GetMomentum() const` | Direction of momentum  (should be an unit vector) |
| `G4double GetTotalEnergy() const` | Total momentum of the track |
| `G4double GetKineticEnergy() const` | Total energy of the track |
| `void SetKineticEnergy(const G4double aValue)` | - |
| `void AddKineticEnergy(const G4double aValue)` | - |
| `G4double GetVelocity() const` | Kinetic Energy of the track |
| `void SetVelocity(G4double v)` | - |
| `G4double GetBeta() const` | Velocity |
| `G4double GetGamma() const` | Velocity of the track in unit of c(light velocity) |
| `G4VPhysicalVolume* GetPhysicalVolume() const` | Gamma factor (1/sqrt[1-beta*beta]) of the track |
| `const G4VTouchable* GetTouchable() const` | - |
| `const G4TouchableHandle& GetTouchableHandle() const` | - |
| `void SetTouchableHandle(const G4TouchableHandle& apValue)` | - |
| `G4Material* GetMaterial() const` | - |
| `void SetMaterial(G4Material*)` | - |
| `const G4MaterialCutsCouple* GetMaterialCutsCouple() const` | - |
| `void SetMaterialCutsCouple(const G4MaterialCutsCouple*)` | - |
| `G4VSensitiveDetector* GetSensitiveDetector() const` | - |
| `void SetSensitiveDetector(G4VSensitiveDetector*)` | - |
| `G4double GetSafety() const` | - |
| `void SetSafety(const G4double aValue)` | - |
| `const G4ThreeVector& GetPolarization() const` | - |
| `void SetPolarization(const G4ThreeVector& aValue)` | - |
| `void AddPolarization(const G4ThreeVector& aValue)` | - |
| `G4StepStatus GetStepStatus() const` | - |
| `void SetStepStatus(const G4StepStatus aValue)` | - |
| `const G4VProcess* GetProcessDefinedStep() const` | - |
| `void SetProcessDefinedStep(const G4VProcess* aValue)` | If the pointer is 0, this means the Step is defined by the user defined limit in the current volume |
| `G4double GetMass() const` | - |
| `void SetMass(G4double value)` | - |
| `G4double GetCharge() const` | - |
| `void SetCharge(G4double value)` | - |
| `G4double GetMagneticMoment() const` | - |
| `void SetMagneticMoment(G4double value)` | - |
| `void SetWeight(G4double aValue)` | - |
| `G4double GetWeight() const` | - |


## Header: `G4Track.hh` (Target: `G4Track`)

**Kind**: C++ Class (`G4Track`) | **Total Extracted Public Methods**: 99

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Track()` | - |
| `G4Track(G4DynamicParticle* apValueDynamicParticle, G4double aValueTime, const G4ThreeVector& aValuePosition)` | Default constructor |
| `G4Track(const G4Track&)` | Constructor - aValueTime is a global time |
| `~G4Track()` | Copy Constructor - copies members other than tracking information |
| `inline void* operator new(std::size_t)` | Destructor |
| `inline void operator delete(void* aTrack)` | Override "new" for "G4Allocator". |
| `G4Track& operator=(const G4Track&)` | Override "delete" for "G4Allocator". |
| `inline G4bool operator==(const G4Track&)` | Assignment operator |
| `inline G4bool operator!=(const G4Track&)` | - |
| `void CopyTrackInfo(const G4Track&)` | Equality operators |
| `G4int GetTrackID() const` | Copy information of the track (w/o tracking information) |
| `void SetTrackID(const G4int aValue)` | - |
| `G4int GetParentID() const` | Get/Set functions track ID |
| `void SetParentID(const G4int aValue)` | - |
| `const G4DynamicParticle* GetDynamicParticle() const` | - |
| `const G4ParticleDefinition* GetParticleDefinition() const` | Dynamic particle |
| `G4ParticleDefinition* GetDefinition() const` | Particle definition |
| `const G4ThreeVector& GetPosition() const` | Obsolete, for backwards compatibility... |
| `void SetPosition(const G4ThreeVector& aValue)` | - |
| `G4double GetGlobalTime() const` | Position, time |
| `void SetGlobalTime(const G4double aValue)` | - |
| `G4double GetLocalTime() const` | Time since the event in which the track belongs is created |
| `void SetLocalTime(const G4double aValue)` | - |
| `G4double GetProperTime() const` | Time since the current track is created |
| `void SetProperTime(const G4double aValue)` | - |
| `G4VPhysicalVolume* GetVolume() const` | Proper time of the current track |
| `G4VPhysicalVolume* GetNextVolume() const` | - |
| `G4Material* GetMaterial() const` | Volume, material, touchable |
| `G4Material* GetNextMaterial() const` | - |
| `const G4MaterialCutsCouple* GetMaterialCutsCouple() const` | - |
| `const G4MaterialCutsCouple* GetNextMaterialCutsCouple() const` | - |
| `const G4VTouchable* GetTouchable() const` | - |
| `const G4TouchableHandle& GetTouchableHandle() const` | - |
| `void SetTouchableHandle(const G4TouchableHandle& apValue)` | - |
| `const G4VTouchable* GetNextTouchable() const` | - |
| `const G4TouchableHandle& GetNextTouchableHandle() const` | - |
| `void SetNextTouchableHandle(const G4TouchableHandle& apValue)` | - |
| `const G4VTouchable* GetOriginTouchable() const` | - |
| `const G4TouchableHandle& GetOriginTouchableHandle() const` | - |
| `void SetOriginTouchableHandle(const G4TouchableHandle& apValue)` | - |
| `G4double GetKineticEnergy() const` | - |
| `void SetKineticEnergy(const G4double aValue)` | - |
| `G4double GetTotalEnergy() const` | Energy |
| `const G4ThreeVector& GetMomentumDirection() const` | - |
| `void SetMomentumDirection(const G4ThreeVector& aValue)` | - |
| `G4ThreeVector GetMomentum() const` | Momentum |
| `G4double GetVelocity() const` | - |
| `void SetVelocity(G4double val)` | - |
| `G4double CalculateVelocity() const` | Velocity |
| `G4double CalculateVelocityForOpticalPhoton() const` | - |
| `G4bool UseGivenVelocity() const` | - |
| `void UseGivenVelocity(G4bool val)` | - |
| `const G4ThreeVector& GetPolarization() const` | - |
| `void SetPolarization(const G4ThreeVector& aValue)` | - |
| `G4TrackStatus GetTrackStatus() const` | Polarization |
| `void SetTrackStatus(const G4TrackStatus aTrackStatus)` | - |
| `G4bool IsBelowThreshold() const` | Track status, flags for tracking |
| `void SetBelowThresholdFlag(G4bool value = true)` | - |
| `G4bool IsGoodForTracking() const` | The flag of "BelowThreshold" is set to true If this track energy is below threshold energy in this material is determined by the range cut value |
| `void SetGoodForTrackingFlag(G4bool value = true)` | - |
| `G4double GetTrackLength() const` | The flag of "GoodForTracking" is set by processes if this track should be tracked even if the energy is below threshold |
| `void AddTrackLength(const G4double aValue)` | - |
| `const G4Step* GetStep() const` | Accumulated track length |
| `void SetStep(const G4Step* aValue)` | - |
| `G4int GetCurrentStepNumber() const` | Step information |
| `void IncrementCurrentStepNumber()` | - |
| `G4double GetStepLength() const` | - |
| `void SetStepLength(G4double value)` | - |
| `const G4ThreeVector& GetVertexPosition() const` | Before the end of the AlongStepDoIt() loop, StepLength keeps the initial value which is determined by the shortest geometrical Step proposed by a physics process. After finishing the AlongStepDoIt(), it will be set equal to 'StepLength' in G4Step |
| `void SetVertexPosition(const G4ThreeVector& aValue)` | - |
| `const G4ThreeVector& GetVertexMomentumDirection() const` | Vertex (where this track was created) information |
| `void SetVertexMomentumDirection(const G4ThreeVector& aValue)` | - |
| `G4double GetVertexKineticEnergy() const` | - |
| `void SetVertexKineticEnergy(const G4double aValue)` | - |
| `const G4LogicalVolume* GetLogicalVolumeAtVertex() const` | - |
| `void SetLogicalVolumeAtVertex(const G4LogicalVolume*)` | - |
| `const G4VProcess* GetCreatorProcess() const` | - |
| `void SetCreatorProcess(const G4VProcess* aValue)` | - |
| `inline void SetCreatorModelID(const G4int id)` | - |
| `inline G4int GetCreatorModelID() const` | - |
| `inline G4int GetCreatorModelIndex() const` | - |
| `inline const G4String GetCreatorModelName() const` | - |
| `inline const G4ParticleDefinition* GetParentResonanceDef() const` | Identification of the physics model that created the track: each of the three information (ID, index, name) is unique (the model ID and its name are supposed to be used in Geant4 code, whereas the index is meant for plotting in user code) |
| `inline void SetParentResonanceDef(const G4ParticleDefinition* parent)` | - |
| `inline G4int GetParentResonanceID() const` | - |
| `inline void SetParentResonanceID(const G4int parentID )` | - |
| `inline G4bool HasParentResonance() const` | - |
| `inline G4int GetParentResonancePDGEncoding() const` | - |
| `inline G4String GetParentResonanceName() const` | - |
| `inline G4double GetParentResonanceMass() const` | - |
| `G4double GetWeight() const` | Because short-lived resonances (e.g. omega, phi, rho, delta, etc.) do not have corresponding track objects, if the track is produced by a resonance parent, these methods allow to get/set information regarding this short-lived parent. The ID is a unique (integer) identifier for each resonance (which corresponds to the rounded integer of the mass of the resonance in keV), which allows to know if two (or more) tracks originated from the same parent resonance: this should not be confused with the parent-track-ID (fParentID) which corresponds to its closest ancestor which is not a short-lived resonance (and therefore has a corresponding track object). In the case of a track non originating from a resonance parent, the above "Get" methods return, respectively: nullptr, 0, false, 0, "", 0. |
| `void SetWeight(G4double aValue)` | - |
| `G4VUserTrackInformation* GetUserInformation() const` | Track weight; methods for manipulating a weight for this track |
| `void SetUserInformation(G4VUserTrackInformation* aValue) const` | - |
| `void SetAuxiliaryTrackInformation(G4int id, G4VAuxiliaryTrackInformation* info) const` | User information |
| `G4VAuxiliaryTrackInformation* GetAuxiliaryTrackInformation(G4int id) const` | - |
| `inline std::map<G4int, G4VAuxiliaryTrackInformation*>* GetAuxiliaryTrackInformationMap() const` | - |
| `void RemoveAuxiliaryTrackInformation(G4int id)` | - |
| `void RemoveAuxiliaryTrackInformation(G4String& name)` | - |


## Header: `G4StepStatus.hh` (Target: `G4StepStatus`)

**Kind**: Enumeration (`enum G4StepStatus`)

| Enumerator | Description / Semantic Role |
|---|---|
| `fWorldBoundary` | State flag in tracking / stepping engine |
| `fGeomBoundary` | State flag in tracking / stepping engine |
| `fAtRestDoItProc` | State flag in tracking / stepping engine |
| `fAlongStepDoItProc` | State flag in tracking / stepping engine |
| `fPostStepDoItProc` | State flag in tracking / stepping engine |
| `fUserDefinedLimit` | State flag in tracking / stepping engine |
| `fExclusivelyForcedProc` | State flag in tracking / stepping engine |
| `fUndefined` | State flag in tracking / stepping engine |


## Header: `G4TrackStatus.hh` (Target: `G4TrackStatus`)

**Kind**: Enumeration (`enum G4TrackStatus`)

| Enumerator | Description / Semantic Role |
|---|---|
| `fAlive` | State flag in tracking / stepping engine |
| `fStopButAlive` | State flag in tracking / stepping engine |
| `fStopAndKill` | State flag in tracking / stepping engine |
| `fKillTrackAndSecondaries` | State flag in tracking / stepping engine |
| `fSuspend` | State flag in tracking / stepping engine |
| `fSuspendAndWait` | State flag in tracking / stepping engine |
| `fPostponeToNextEvent` | State flag in tracking / stepping engine |


## Header: `G4UserSteppingAction.hh` (Target: `G4UserSteppingAction`)

**Kind**: C++ Class (`G4UserSteppingAction`) | **Total Extracted Public Methods**: 4

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UserSteppingAction()` | Constructor and destructor |
| `virtual ~G4UserSteppingAction() = default` | - |
| `virtual void SetSteppingManagerPointer(G4SteppingManager* pValue)` | Member functions |
| `virtual void UserSteppingAction(const G4Step*)` | - |


## Header: `G4UserTrackingAction.hh` (Target: `G4UserTrackingAction`)

**Kind**: C++ Class (`G4UserTrackingAction`) | **Total Extracted Public Methods**: 5

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UserTrackingAction()` | Constructor & Destructor |
| `virtual ~G4UserTrackingAction() = default` | - |
| `virtual void SetTrackingManagerPointer(G4TrackingManager* pValue)` | Member functions |
| `virtual void PreUserTrackingAction(const G4Track*)` | - |
| `virtual void PostUserTrackingAction(const G4Track*)` | - |


## Header: `G4VProcess.hh` (Target: `G4VProcess`)

**Kind**: C++ Class (`G4VProcess`) | **Total Extracted Public Methods**: 49

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VProcess(const G4String& aName = "NoName", G4ProcessType aType = fNotDefined)` | - |
| `G4VProcess(const G4VProcess& right)` | Constructor requires the process name and type |
| `virtual ~G4VProcess()` | Copy constructor copies the name but does not copy the physics table (null pointer is assigned instead) |
| `G4VProcess& operator=(const G4VProcess&) = delete` | Destructor |
| `G4bool operator==(const G4VProcess& right) const` | - |
| `G4bool operator!=(const G4VProcess& right) const` | - |
| `virtual G4VParticleChange* PostStepDoIt( const G4Track& track, const G4Step& stepData ) = 0` | Equality operators ////////////////////////// DoIt    ///////////////// ////////////////////////// |
| `virtual G4VParticleChange* AlongStepDoIt( const G4Track& track, const G4Step& stepData ) = 0` | - |
| `virtual G4VParticleChange* AtRestDoIt( const G4Track& track, const G4Step& stepData ) = 0` | - |
| `virtual G4double AlongStepGetPhysicalInteractionLength( const G4Track& track, G4double previousStepSize, G4double currentMinimumStep, G4double& proposedSafety, G4GPILSelection* selection) = 0` | A virtual base class function that has to be overridden by any subclass. The DoIt() method actually performs the physics process and determines either momentum change of the production of secondaries etc. Arguments const G4Track& track: reference to the current G4Track information const G4Step&  stepData: reference to the current G4Step information //////////////////////// GPIL    /////////////// //////////////////////// |
| `virtual G4double AtRestGetPhysicalInteractionLength( const G4Track& track, G4ForceCondition* condition ) = 0` | - |
| `virtual G4double PostStepGetPhysicalInteractionLength( const G4Track& track, G4double previousStepSize, G4ForceCondition* condition ) = 0` | - |
| `inline G4double GetCurrentInteractionLength() const` | Returns the Step-size (actual length) which is allowed by "this" process. (for AtRestGetPhysicalInteractionLength, return value is Step-time) The NumberOfInteractionLengthLeft is recalculated by using previousStepSize and the Step-size is calucalted accoding to the resultant NumberOfInteractionLengthLeft. using NumberOfInteractionLengthLeft, which is recalculated at arguments const G4Track&    track: reference to the current G4Track information G4double*          previousStepSize: the Step-size (actual length) of the previous Step of this track. Negative calue indicates that NumberOfInteractionLengthLeft must be reset. the current physical interaction legth of this process G4ForceCondition* condition: the flag indicates DoIt of this process is forced to be called Forced:    Corresponding DoIt is forced NotForced: Corresponding DoIt is called if the Step size of this Step is determined by this process !! AlongStepDoIt is always called !! G4double& currentMinimumStep: this value is used for transformation of true path length to geometrical path length |
| `inline void SetPILfactor(G4double value)` | Returns currentInteractionLength //////// PIL factor //////// |
| `inline G4double GetPILfactor() const` | - |
| `inline G4double AlongStepGPIL( const G4Track& track, G4double previousStepSize, G4double currentMinimumStep, G4double& proposedSafety, G4GPILSelection* selection )` | Set/Get factor for PhysicsInteractionLength which is passed to G4SteppingManager for both AtRest and PostStep These three GPIL methods are used by Stepping Manager. They invoke virtual GPIL methods listed above. As for AtRest and PostStep the returned value is multipled by thePILfactor |
| `inline G4double AtRestGPIL( const G4Track& track, G4ForceCondition* condition )` | - |
| `inline G4double PostStepGPIL( const G4Track& track, G4double previousStepSize, G4ForceCondition* condition )` | - |
| `virtual G4bool IsApplicable(const G4ParticleDefinition&)` | - |
| `virtual void BuildPhysicsTable(const G4ParticleDefinition&)` | Returns true if this process object is applicable to the particle type. Process will not be registered to a particle if IsApplicable is false |
| `virtual void PreparePhysicsTable(const G4ParticleDefinition&)` | Messaged by the Particle definition (via the Process manager) whenever cross-section tables have to be rebuilt (i.e. if new materials have been defined). It is overloaded by individual processes when they need physics tables |
| `virtual G4bool StorePhysicsTable(const G4ParticleDefinition* , const G4String&, G4bool)` | Messaged by the Particle definition (via the Process manager) whenever cross-section tables have to be prepared for rebuild (i.e. if new materials have been defined). It is overloaded by individual processes when they need physics tables Processes which Build physics tables independent of cuts (for example in their constructors) should preferably use private void BuildThePhysicsTable() and void PreparePhysicsTable(). *Not* another BuildPhysicsTable |
| `virtual G4bool RetrievePhysicsTable(const G4ParticleDefinition* , const G4String&, G4bool)` | Store PhysicsTable in a file. Return false in case of failure at I/O |
| `const G4String& GetPhysicsTableFileName(const G4ParticleDefinition* , const G4String& directory, const G4String& tableName, G4bool ascii = false)` | Retrieve Physics from a file. Return true if the Physics Table can be built by using file. Return false if the process has no functionality or in case of failure. File name should be defined by each process and the file should be placed under the directory specified by the argument |
| `inline const G4String& GetProcessName() const` | This method is utility for Store/RetreivePhysicsTable |
| `inline G4ProcessType GetProcessType() const` | Returns the name of the process |
| `inline void SetProcessType(G4ProcessType)` | Returns the process type |
| `inline G4int GetProcessSubType() const` | Sets the process type |
| `inline void SetProcessSubType(G4int)` | Returns the process sub type |
| `static const G4String& GetProcessTypeName(G4ProcessType)` | Sets the process sub type |
| `virtual const G4VProcess* GetCreatorProcess() const` | Returns the process type name |
| `virtual void StartTracking(G4Track*)` | Returns the process to be used as CreatorProcess for secondaries coming from this process |
| `virtual void EndTracking()` | - |
| `virtual void SetProcessManager(const G4ProcessManager*)` | Inform Start/End of tracking for each track to the physics process |
| `virtual const G4ProcessManager* GetProcessManager()` | A process manager sets its own pointer when the process is registered in the process Manager |
| `virtual void ResetNumberOfInteractionLengthLeft()` | Get the process manager which the process belongs to |
| `inline G4double GetNumberOfInteractionLengthLeft() const` | Reset (determine the value of) NumberOfInteractionLengthLeft |
| `inline G4double GetTotalNumberOfInteractionLengthTraversed() const` | Get NumberOfInteractionLengthLeft |
| `inline G4bool isAtRestDoItIsEnabled() const` | Get NumberOfInteractionLength after ResetNumberOfInteractionLengthLeft() is invoked |
| `inline G4bool isAlongStepDoItIsEnabled() const` | - |
| `inline G4bool isPostStepDoItIsEnabled() const` | - |
| `virtual void DumpInfo() const` | These methods indicate which DoIt is enabled. They are used by G4ProcessManager to check that ordering parameters are properly set |
| `virtual void ProcessDescription(std::ostream& outfile) const` | Dump out process information |
| `inline void SetVerboseLevel(G4int value)` | Write out to html file for automatic documentation |
| `inline G4int GetVerboseLevel() const` | - |
| `virtual void SetMasterProcess(G4VProcess* masterP)` | set/get control flag for output message 0: Silent 1: Warning message 2: More |
| `inline const G4VProcess* GetMasterProcess() const` | Sets the master thread process instance |
| `virtual void BuildWorkerPhysicsTable(const G4ParticleDefinition& part)` | Returns the master thread process instance. Can be used to initialise worker type processes instances from master one (e.g. to share a read-only table) if ( this != GetMasterProcess() ) { /*worker*/ } else { /* master or sequential */ } |
| `virtual void PrepareWorkerPhysicsTable(const G4ParticleDefinition&)` | Messaged by the Particle definition (via the Process manager) in worker threads. See BuildWorkerPhyiscsTable() method. Can be used to share among threads physics tables. Use GetMasterProcess() to get pointer of master process from worker thread. By default this method makes a forward call to BuildPhysicsTable() |

