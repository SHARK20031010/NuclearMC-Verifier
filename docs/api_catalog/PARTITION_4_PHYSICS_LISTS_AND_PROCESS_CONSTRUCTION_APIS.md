# Partition 4: Physics Lists & Process Construction APIs

> **Partition Scope**: Modular physics lists, physics constructors, EM/hadron/decay packages, production cuts, and user limits.

> **Header Base Path**: `/home/shark/geant4/11.2.2-install/include/Geant4`

---

## Header: `G4VUserPhysicsList.hh` (Target: `G4VUserPhysicsList`)

**Kind**: C++ Class (`G4VUserPhysicsList`) | **Total Extracted Public Methods**: 46

| C++ Method Signature | Comment / Description |
|---|---|
| `void initialize()` | - |
| `G4VUserPhysicsList()` | - |
| `virtual ~G4VUserPhysicsList()` | - |
| `G4VUserPhysicsList(const G4VUserPhysicsList&)` | - |
| `G4VUserPhysicsList& operator=(const G4VUserPhysicsList&)` | - |
| `virtual void ConstructParticle() = 0` | Each particle type will be instantiated. This method is invoked by the RunManger. |
| `void Construct()` | By calling the "Construct" method, process manager and processes are created. |
| `virtual void ConstructProcess() = 0` | Each physics process will be instantiated and registered to the process manager of each particle type. Invoked in the Construct() method. |
| `virtual void SetCuts()` | Sets a cut value for all particle types in the particle table. |
| `void SetDefaultCutValue(G4double newCutValue)` | Set/get the default cut value. Calling SetDefaultCutValue() causes re-calcuration of cut values and physics tables just before the next event loop. |
| `G4double GetDefaultCutValue() const` | - |
| `void BuildPhysicsTable()` | Invoke BuildPhysicsTable for all processes for all particles. In case of "Retrieve" flag is ON, PhysicsTable will be retrieved from files. |
| `void PreparePhysicsTable(G4ParticleDefinition*)` | Prepare the PhysicsTable for specified particle type. |
| `void BuildPhysicsTable(G4ParticleDefinition*)` | Build the PhysicsTable for specified particle type. |
| `G4bool StorePhysicsTable(const G4String& directory = ".")` | Store PhysicsTable together with both material and cut value information in files under the specified directory. Returns "true" if files are successfully created. |
| `G4bool IsPhysicsTableRetrieved() const` | Return true if "Retrieve" flag is ON. (i.e. PhysicsTable will be retrieved from files). |
| `G4bool IsStoredInAscii() const` | - |
| `const G4String& GetPhysicsTableDirectory() const` | Get directory path for physics table files. |
| `void SetPhysicsTableRetrieved(const G4String& directory = "")` | Set "Retrieve" flag. Directory path can be set together. Null string (default) means directory is not changed from the current value. |
| `void SetStoredInAscii()` | - |
| `void ResetPhysicsTableRetrieved()` | Reset "Retrieve" flag. |
| `void ResetStoredInAscii()` | - |
| `void DumpList() const` | Print out the List of registered particles types. |
| `void DumpCutValuesTable(G4int flag = 1)` | Request to print out information of cut values. Printing will be performed when all tables are made. |
| `void DumpCutValuesTableIfRequested()` | Triggers the print-out requested by the above method. This method must be invoked by RunManager at the proper moment. |
| `void SetVerboseLevel(G4int value)` | Set/get control flag for output message 0: Silent 1: Warning message 2: More |
| `G4int GetVerboseLevel() const` | - |
| `void UseCoupledTransportation(G4bool vl = true)` | - |
| `void SetCutsWithDefault()` | Invokes default SetCuts() method. Note: cut values will not be overwritten. Use of default SetCuts() method is recommended. |
| `void SetCutValue(G4double aCut, const G4String& pname)` | Sets a cut value for a particle type for the default region. |
| `G4double GetCutValue(const G4String& pname) const` | Gets a cut value for a particle type for the default region. |
| `void SetCutValue(G4double aCut, const G4String& pname, const G4String& rname)` | Sets a cut value for a particle type for a region. |
| `void SetParticleCuts(G4double cut, G4ParticleDefinition* particle, G4Region* region = nullptr)` | Invoke SetCuts for specified particle for a region. If the pointer to the region is NULL, the default region is used In case of "Retrieve" flag is ON, cut values will be retrieved from files. |
| `void SetParticleCuts(G4double cut, const G4String& particleName, G4Region* region = nullptr)` | - |
| `void SetCutsForRegion(G4double aCut, const G4String& rname)` | Invoke SetCuts() for all particles in a region. |
| `void SetApplyCuts(G4bool value, const G4String& name)` | Gets/sets the flag for ApplyCuts(). |
| `G4bool GetApplyCuts(const G4String& name) const` | - |
| `void RemoveProcessManager()` | Remove and delete ProcessManagers for all particles in the Particle Table. |
| `void RemoveTrackingManager()` | Remove and delete TrackingManagers for all particles in the Particle Table. |
| `void AddProcessManager(G4ParticleDefinition* newParticle, G4ProcessManager* newManager = nullptr)` | Add process manager for particles created on-the-fly. |
| `void CheckParticleList()` | Check consistencies of list of particles. |
| `void DisableCheckParticleList()` | - |
| `inline G4int GetInstanceID() const` | - |
| `static const G4VUPLManager& GetSubInstanceManager()` | - |
| `virtual void InitializeWorker()` | Used by Worker threads on the shared instance of physics-list to initialise workers. Derived class re-implementing this method must also call this base class method. |
| `virtual void TerminateWorker()` | Destroy thread-local data. Note that derived classes implementing this method should still call this base class one. |


## Header: `G4VModularPhysicsList.hh` (Target: `G4VModularPhysicsList`)

**Kind**: C++ Class (`G4VModularPhysicsList`) | **Total Extracted Public Methods**: 18

| C++ Method Signature | Comment / Description |
|---|---|
| `void initialize()` | - |
| `G4VModularPhysicsList()` | - |
| `~G4VModularPhysicsList() override` | - |
| `void ConstructParticle() override` | This method will be invoked in the Construct() method. Each particle type will be instantiated. |
| `void ConstructProcess() override` | This method will be invoked in the Construct() method. Each physics process will be instantiated and registered to the process manager of each particle type. |
| `void RegisterPhysics(G4VPhysicsConstructor*)` | Register Physics Constructor. |
| `const G4VPhysicsConstructor* GetPhysics(G4int index) const` | - |
| `const G4VPhysicsConstructor* GetPhysics(const G4String& name) const` | - |
| `const G4VPhysicsConstructor* GetPhysicsWithType(G4int physics_type) const` | - |
| `void ReplacePhysics(G4VPhysicsConstructor*)` | Replace the Physics Constructor. The existing physics constructor with same physics_type as one of the given physics constructor is replaced (existing physics will be deleted). If a corresponding physics constructor is NOT found, the given physics constructor is just added. |
| `void RemovePhysics(G4VPhysicsConstructor*)` | Remove the Physics Constructor from the list. |
| `void RemovePhysics(G4int type)` | - |
| `void RemovePhysics(const G4String& name)` | - |
| `inline G4int GetInstanceID() const` | - |
| `static const G4VMPLManager& GetSubInstanceManager()` | - |
| `void TerminateWorker() override` | - |
| `void SetVerboseLevel(G4int value)` | Set/get control flag for output message 0: Silent 1: Warning message 2: More given verbose level is set to all physics constructors. |
| `G4int GetVerboseLevel() const` | - |


## Header: `G4VPhysicsConstructor.hh` (Target: `G4VPhysicsConstructor`)

**Kind**: C++ Class (`G4VPhysicsConstructor`) | **Total Extracted Public Methods**: 15

| C++ Method Signature | Comment / Description |
|---|---|
| `void initialize()` | - |
| `G4VPhysicsConstructor(const G4String& = "")` | - |
| `G4VPhysicsConstructor(const G4String& name, G4int physics_type)` | - |
| `virtual ~G4VPhysicsConstructor()` | - |
| `virtual void ConstructParticle() = 0` | This method will be invoked in the Construct() method. Each particle type will be instantiated. |
| `virtual void ConstructProcess() = 0` | This method will be invoked in the Construct() method. Each physics process will be instantiated and registered to the process manager of each particle type. |
| `inline void SetPhysicsName(const G4String& = "")` | - |
| `inline const G4String& GetPhysicsName() const` | - |
| `inline void SetPhysicsType(G4int)` | - |
| `inline G4int GetPhysicsType() const` | - |
| `inline G4int GetInstanceID() const` | - |
| `static const G4VPCManager& GetSubInstanceManager()` | - |
| `virtual void TerminateWorker()` | Method called by kernel to destroy thread-local data, equivalent to destructor in sequential mode. Derived classes implementing this method, must also call this base class method. |
| `inline void SetVerboseLevel(G4int value)` | Set/get control flag for output message 0: Silent 1: Warning message 2: More verbose level is set equal to physics list when registered. |
| `inline G4int GetVerboseLevel() const` | - |


## Header: `G4EmStandardPhysics.hh` (Target: `G4EmStandardPhysics`)

**Kind**: C++ Class (`G4EmStandardPhysics`) | **Total Extracted Public Methods**: 4

| C++ Method Signature | Comment / Description |
|---|---|
| `explicit G4EmStandardPhysics(G4int ver=1, const G4String& name="")` | - |
| `~G4EmStandardPhysics() override` | - |
| `void ConstructParticle() override` | - |
| `void ConstructProcess() override` | - |


## Header: `G4EmStandardPhysics_option4.hh` (Target: `G4EmStandardPhysics_option4`)

**Kind**: C++ Class (`G4EmStandardPhysics_option4`) | **Total Extracted Public Methods**: 4

| C++ Method Signature | Comment / Description |
|---|---|
| `explicit G4EmStandardPhysics_option4(G4int ver=1, const G4String& name="")` | - |
| `~G4EmStandardPhysics_option4() override` | - |
| `void ConstructParticle() override` | - |
| `void ConstructProcess() override` | - |


## Header: `G4DecayPhysics.hh` (Target: `G4DecayPhysics`)

**Kind**: C++ Class (`G4DecayPhysics`) | **Total Extracted Public Methods**: 5

| C++ Method Signature | Comment / Description |
|---|---|
| `G4DecayPhysics(G4int ver = 1)` | - |
| `G4DecayPhysics(const G4String& name, G4int ver = 1)` | - |
| `~G4DecayPhysics() override` | - |
| `void ConstructParticle() override` | This method will be invoked in the Construct() method. each particle type will be instantiated |
| `void ConstructProcess() override` | This method will be invoked in the Construct() method. each physics process will be instantiated and registered to the process manager of each particle type |


## Header: `G4RadioactiveDecayPhysics.hh` (Target: `G4RadioactiveDecayPhysics`)

**Kind**: C++ Class (`G4RadioactiveDecayPhysics`) | **Total Extracted Public Methods**: 5

| C++ Method Signature | Comment / Description |
|---|---|
| `G4RadioactiveDecayPhysics(G4int verbose =1)` | - |
| `G4RadioactiveDecayPhysics(const G4String& name, G4int verbose =1)` | - |
| `~G4RadioactiveDecayPhysics() override` | - |
| `void ConstructParticle() override` | This method is dummy for physics |
| `void ConstructProcess() override` | This method will be invoked in the Construct() method. each physics process will be instantiated and registered to the process manager of each particle type |


## Header: `G4HadronPhysicsQGSP_BIC_HP.hh` (Target: `G4HadronPhysicsQGSP_BIC_HP`)

**Kind**: C++ Class (`G4HadronPhysicsQGSP_BIC_HP`) | **Total Extracted Public Methods**: 5

| C++ Method Signature | Comment / Description |
|---|---|
| `G4HadronPhysicsQGSP_BIC_HP( G4int verbose = 1 )` | - |
| `G4HadronPhysicsQGSP_BIC_HP( const G4String& name, G4bool quasiElastic = true )` | - |
| `virtual ~G4HadronPhysicsQGSP_BIC_HP()` | - |
| `G4HadronPhysicsQGSP_BIC_HP(G4HadronPhysicsQGSP_BIC_HP &) = delete` | copy constructor and hide assignment operator |
| `G4HadronPhysicsQGSP_BIC_HP & operator = (const G4HadronPhysicsQGSP_BIC_HP &right) = delete` | - |


## Header: `G4HadronPhysicsFTFP_BERT.hh` (Target: `G4HadronPhysicsFTFP_BERT`)

**Kind**: C++ Class (`G4HadronPhysicsFTFP_BERT`) | **Total Extracted Public Methods**: 7

| C++ Method Signature | Comment / Description |
|---|---|
| `G4HadronPhysicsFTFP_BERT(G4int verbose =1)` | - |
| `G4HadronPhysicsFTFP_BERT(const G4String& name, G4bool quasiElastic=false)` | - |
| `virtual ~G4HadronPhysicsFTFP_BERT()` | - |
| `void ConstructParticle() override` | - |
| `void ConstructProcess() override` | - |
| `G4HadronPhysicsFTFP_BERT(G4HadronPhysicsFTFP_BERT &) = delete` | copy constructor and hide assignment operator |
| `G4HadronPhysicsFTFP_BERT & operator = (const G4HadronPhysicsFTFP_BERT &right) = delete` | - |


## Header: `G4ProductionCuts.hh` (Target: `G4ProductionCuts`)

**Kind**: C++ Class (`G4ProductionCuts`) | **Total Extracted Public Methods**: 18

| C++ Method Signature | Comment / Description |
|---|---|
| `G4ProductionCuts()` | - |
| `G4ProductionCuts(const G4ProductionCuts& right)` | Constructor |
| `virtual ~G4ProductionCuts()` | Copy constructor |
| `G4ProductionCuts& operator=(const G4ProductionCuts& right)` | Destructor |
| `G4bool operator==(const G4ProductionCuts& right) const` | Assignment operator |
| `G4bool operator!=(const G4ProductionCuts& right) const` | - |
| `void SetProductionCut(G4double cut, G4int index)` | Equality operators |
| `void SetProductionCut(G4double cut, G4ParticleDefinition* ptcl)` | - |
| `void SetProductionCut(G4double cut, const G4String& pName)` | - |
| `void SetProductionCut(G4double cut)` | Set the production cut in range for a specific particle from and index, G4ParticleDefinition* or particle name. If the particle is not a photon, e-, e+ or proton, the function has no effect. |
| `G4double GetProductionCut(G4int index) const` | Set the production cut in range for photons, e-, e+ and protons. |
| `G4double GetProductionCut(const G4String& name) const` | Get the production cut in range with an index to particle type |
| `void SetProductionCuts(std::vector<G4double>&)` | Get the production cut in range with a name of particle type |
| `const std::vector<G4double>& GetProductionCuts() const` | Set the vector of production cuts in range for all particles |
| `G4bool IsModified() const` | Get the vector of production cuts in range for all particles |
| `void PhysicsTableUpdated()` | Return true if any cut value has been modified after last calculation of Physics Table |
| `static G4int GetIndex(const G4String& name)` | Inform end of calculation of Physics Table to production cut |
| `static G4int GetIndex(const G4ParticleDefinition* ptcl)` | - |


## Header: `G4UserLimits.hh` (Target: `G4UserLimits`)

**Kind**: C++ Class (`G4UserLimits`) | **Total Extracted Public Methods**: 15

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UserLimits(G4double ustepMax = DBL_MAX, G4double utrakMax = DBL_MAX, G4double utimeMax = DBL_MAX, G4double uekinMin = 0., G4double urangMin = 0.)` | - |
| `G4UserLimits(const G4String& type, G4double ustepMax = DBL_MAX, G4double utrakMax = DBL_MAX, G4double utimeMax = DBL_MAX, G4double uekinMin = 0., G4double urangMin = 0.)` | - |
| `virtual ~G4UserLimits()` | - |
| `virtual G4double GetMaxAllowedStep(const G4Track&)` | - |
| `virtual G4double GetUserMaxTrackLength(const G4Track&)` | If a logical volume has a G4UserLimits object, the Step length can be limited as shorter than MaxAllowedStep in the volume |
| `virtual G4double GetUserMaxTime(const G4Track&)` | - |
| `virtual G4double GetUserMinEkine(const G4Track&)` | - |
| `virtual G4double GetUserMinRange(const G4Track&)` | - |
| `virtual void SetMaxAllowedStep(G4double ustepMax)` | - |
| `virtual void SetUserMaxTrackLength(G4double utrakMax)` | - |
| `virtual void SetUserMaxTime(G4double utimeMax)` | - |
| `virtual void SetUserMinEkine(G4double uekinMin)` | - |
| `virtual void SetUserMinRange(G4double urangMin)` | - |
| `const G4String& GetType() const` | - |
| `void SetType(const G4String& type)` | - |

