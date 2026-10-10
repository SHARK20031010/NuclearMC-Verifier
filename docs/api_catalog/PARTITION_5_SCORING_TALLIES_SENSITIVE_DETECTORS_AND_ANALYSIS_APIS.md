# Partition 5: Scoring, Tallies, Sensitive Detectors & Analysis APIs

> **Partition Scope**: Sensitive detectors, multi-functional detectors, primitive scorers, filters, SD manager, and G4AnalysisManager.

> **Header Base Path**: `/home/shark/geant4/11.2.2-install/include/Geant4`

---

## Header: `G4VSensitiveDetector.hh` (Target: `G4VSensitiveDetector`)

**Kind**: C++ Class (`G4VSensitiveDetector`) | **Total Extracted Public Methods**: 31

| C++ Method Signature | Comment / Description |
|---|---|
| `explicit G4VSensitiveDetector(G4String name)` | Constructors. The user's concrete class must use one of these constructors by the constructor initializer of the derived class. The name of the sensitive detector must be unique. |
| `G4VSensitiveDetector(const G4VSensitiveDetector& right)` | - |
| `G4VSensitiveDetector& operator=(const G4VSensitiveDetector& right)` | - |
| `virtual ~G4VSensitiveDetector() = default` | - |
| `G4bool operator==(const G4VSensitiveDetector& right) const` | - |
| `G4bool operator!=(const G4VSensitiveDetector& right) const` | - |
| `virtual void Initialize(G4HCofThisEvent*)` | These two methods are invoked at the begining and at the end of each event. The hits collection(s) created by this sensitive detector must be set to the G4HCofThisEvent object at one of these two methods. |
| `virtual void EndOfEvent(G4HCofThisEvent*)` | - |
| `virtual void clear()` | This method is invoked if the event abortion is occured. Hits collections created but not beibg set to G4HCofThisEvent at the event should be deleted. Collection(s) which have already set to G4HCofThisEvent will be deleted automatically. |
| `virtual void DrawAll()` | - |
| `virtual void PrintAll()` | - |
| `inline G4bool Hit(G4Step* aStep)` | This is the public method invoked by G4SteppingManager for generating hit(s). The actual user's implementation for generating hit(s) must be implemented in GenerateHits() virtual protected method. This method MUST NOT be overriden. |
| `if (! isActive()) return false` | - |
| `if (filter != nullptr)` | - |
| `if (! (filter->Accept(aStep))) return false` | - |
| `} if (ROgeometry != nullptr)` | - |
| `if (! (ROgeometry->CheckROVolume(aStep, ROhis))) return false` | - |
| `} return ProcessHits(aStep, ROhis)` | - |
| `} inline void SetROgeometry(G4VReadOutGeometry* value)` | Register the Readout geometry. |
| `inline void SetFilter(G4VSDFilter* value)` | Register a filter |
| `inline G4int GetNumberOfCollections() const` | - |
| `inline G4String GetCollectionName(G4int id) const` | - |
| `inline void SetVerboseLevel(G4int vl)` | - |
| `inline void Activate(G4bool activeFlag)` | - |
| `inline G4bool isActive() const` | - |
| `inline G4String GetName() const` | - |
| `inline G4String GetPathName() const` | - |
| `inline G4String GetFullPathName() const` | - |
| `inline G4VReadOutGeometry* GetROgeometry() const` | - |
| `inline G4VSDFilter* GetFilter() const` | - |
| `virtual G4VSensitiveDetector* Clone() const` | - |


## Header: `G4MultiFunctionalDetector.hh` (Target: `G4MultiFunctionalDetector`)

**Kind**: C++ Class (`G4MultiFunctionalDetector`) | **Total Extracted Public Methods**: 11

| C++ Method Signature | Comment / Description |
|---|---|
| `G4MultiFunctionalDetector(G4String)` | - |
| `~G4MultiFunctionalDetector() override` | - |
| `G4bool RegisterPrimitive(G4VPrimitiveScorer*)` | - |
| `G4bool RemovePrimitive(G4VPrimitiveScorer*)` | - |
| `inline G4int GetNumberOfPrimitives() const` | - |
| `G4VPrimitiveScorer* GetPrimitive(G4int id) const` | - |
| `void Initialize(G4HCofThisEvent*) override` | - |
| `void EndOfEvent(G4HCofThisEvent*) override` | - |
| `void clear() override` | - |
| `void DrawAll() override` | - |
| `void PrintAll() override` | - |


## Header: `G4VPrimitiveScorer.hh` (Target: `G4VPrimitiveScorer`)

**Kind**: C++ Class (`G4VPrimitiveScorer`) | **Total Extracted Public Methods**: 19

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VPrimitiveScorer(G4String name, G4int depth = 0)` | - |
| `virtual ~G4VPrimitiveScorer() = default` | - |
| `G4int GetCollectionID(G4int)` | This method returns the ID of its hitsCollection. This mehod gives valid value only after it is registered to G4MultiFunctionalDetector and the G4MultiFunctionalDetector is registered to G4SDManager. |
| `virtual void Initialize(G4HCofThisEvent*)` | These five methods are exactly identical to those in G4VSensitiveDetector. These methods are invoked by G4SDManager through G4MultiFunctionalDetector. |
| `virtual void EndOfEvent(G4HCofThisEvent*)` | - |
| `virtual void clear()` | - |
| `virtual void DrawAll()` | - |
| `virtual void PrintAll()` | - |
| `void SetUnit(const G4String& unit)` | - |
| `const G4String& GetUnit() const` | - |
| `G4double GetUnitValue() const` | - |
| `inline void SetMultiFunctionalDetector(G4MultiFunctionalDetector* d)` | Set/Get methods |
| `inline G4MultiFunctionalDetector* GetMultiFunctionalDetector() const` | - |
| `inline G4String GetName() const` | - |
| `inline void SetFilter(G4VSDFilter* f)` | - |
| `inline G4VSDFilter* GetFilter() const` | - |
| `inline void SetVerboseLevel(G4int vl)` | - |
| `inline G4int GetVerboseLevel() const` | - |
| `inline void SetNijk(G4int i, G4int j, G4int k)` | - |


## Header: `G4PSEnergyDeposit.hh` (Target: `G4PSEnergyDeposit`)

**Kind**: C++ Class (`G4PSEnergyDeposit`) | **Total Extracted Public Methods**: 7

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PSEnergyDeposit(G4String name, G4int depth = 0); // default unit` | - |
| `G4PSEnergyDeposit(G4String name, const G4String& unit, G4int depth = 0)` | - |
| `~G4PSEnergyDeposit() override = default` | - |
| `void Initialize(G4HCofThisEvent*) override` | - |
| `void clear() override` | - |
| `void PrintAll() override` | - |
| `virtual void SetUnit(const G4String& unit)` | - |


## Header: `G4PSDoseDeposit.hh` (Target: `G4PSDoseDeposit`)

**Kind**: C++ Class (`G4PSDoseDeposit`) | **Total Extracted Public Methods**: 7

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PSDoseDeposit(G4String name, G4int depth = 0)` | - |
| `G4PSDoseDeposit(G4String name, const G4String& unit, G4int depth = 0)` | - |
| `~G4PSDoseDeposit() override = default` | - |
| `void Initialize(G4HCofThisEvent*) override` | - |
| `void clear() override` | - |
| `void PrintAll() override` | - |
| `virtual void SetUnit(const G4String& unit)` | - |


## Header: `G4PSCellFlux.hh` (Target: `G4PSCellFlux`)

**Kind**: C++ Class (`G4PSCellFlux`) | **Total Extracted Public Methods**: 8

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PSCellFlux(G4String name, G4int depth = 0)` | - |
| `G4PSCellFlux(G4String name, const G4String& unit, G4int depth = 0)` | - |
| `~G4PSCellFlux() override = default` | - |
| `inline void Weighted(G4bool flg = true)` | - |
| `void Initialize(G4HCofThisEvent*) override` | Multiply track weight |
| `void clear() override` | - |
| `void PrintAll() override` | - |
| `virtual void SetUnit(const G4String& unit)` | - |


## Header: `G4PSPassageCellFlux.hh` (Target: `G4PSPassageCellFlux`)

**Kind**: C++ Class (`G4PSPassageCellFlux`) | **Total Extracted Public Methods**: 8

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PSPassageCellFlux(G4String name, G4int depth = 0)` | - |
| `G4PSPassageCellFlux(G4String name, const G4String& unit, G4int depth = 0)` | - |
| `~G4PSPassageCellFlux() override = default` | - |
| `inline void Weighted(G4bool flg = true)` | - |
| `void Initialize(G4HCofThisEvent*) override` | Multiply track weight |
| `void clear() override` | - |
| `void PrintAll() override` | - |
| `virtual void SetUnit(const G4String& unit)` | - |


## Header: `G4PSTrackLength.hh` (Target: `G4PSTrackLength`)

**Kind**: C++ Class (`G4PSTrackLength`) | **Total Extracted Public Methods**: 10

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PSTrackLength(G4String name, G4int depth = 0)` | - |
| `G4PSTrackLength(G4String name, const G4String& unit, G4int depth = 0)` | - |
| `~G4PSTrackLength() override = default` | - |
| `inline void Weighted(G4bool flg = true)` | - |
| `void MultiplyKineticEnergy(G4bool flg = true)` | Multiply track weight |
| `void DivideByVelocity(G4bool flg = true)` | Multiply Kinetic Energy |
| `void Initialize(G4HCofThisEvent*) override` | Divide by velocity |
| `void clear() override` | - |
| `void PrintAll() override` | - |
| `virtual void SetUnit(const G4String& unit)` | - |


## Header: `G4PSNofSecondary.hh` (Target: `G4PSNofSecondary`)

**Kind**: C++ Class (`G4PSNofSecondary`) | **Total Extracted Public Methods**: 8

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PSNofSecondary(G4String name, G4int depth = 0)` | - |
| `~G4PSNofSecondary() override = default` | - |
| `void SetParticle(const G4String& particleName)` | Scoring option |
| `inline void Weighted(G4bool flg = true)` | - |
| `void Initialize(G4HCofThisEvent*) override` | Multiply track weight |
| `void clear() override` | - |
| `void PrintAll() override` | - |
| `virtual void SetUnit(const G4String& unit)` | - |


## Header: `G4SDManager.hh` (Target: `G4SDManager`)

**Kind**: C++ Class (`G4SDManager`) | **Total Extracted Public Methods**: 21

| C++ Method Signature | Comment / Description |
|---|---|
| `static G4SDManager* GetSDMpointer()` | Returns the pointer to the singleton object, creating it if not null |
| `static G4SDManager* GetSDMpointerIfExist()` | Returns current pointer to the singleton object Caller is responsible for checking value against `nullptr` |
| `G4SDManager(const G4SDManager&) = delete` | - |
| `G4SDManager& operator=(const G4SDManager&) = delete` | - |
| `~G4SDManager()` | - |
| `void AddNewDetector(G4VSensitiveDetector* aSD)` | Register sensitive detector instance This method must be invoked when the user constructs their sensitive detector |
| `void Activate(G4String dName, G4bool activeFlag)` | Activate/inactivate the registered sensitive detector. For the inactivated detectors, hits collections will not be stored to the G4HCofThisEvent object. |
| `G4int GetCollectionID(G4String colName)` | Return ID number of sensitive detector with given name |
| `G4int GetCollectionID(G4VHitsCollection* aHC)` | Return ID number of sensitive detector creating given hits collection |
| `G4VSensitiveDetector* FindSensitiveDetector(G4String dName, G4bool warning = true)` | - |
| `G4HCofThisEvent* PrepareNewEvent()` | - |
| `void TerminateCurrentEvent(G4HCofThisEvent* HCE)` | - |
| `void AddNewCollection(G4String SDname, G4String DCname)` | - |
| `inline void SetVerboseLevel(G4int vl)` | - |
| `treeTop->SetVerboseLevel(vl)` | - |
| `} inline G4SDStructure* GetTreeTop() const` | - |
| `inline void ListTree() const` | - |
| `inline G4int GetCollectionCapacity() const` | - |
| `inline G4HCtable* GetHCtable() const` | - |
| `void RegisterSDFilter(G4VSDFilter* filter)` | - |
| `void DeRegisterSDFilter(G4VSDFilter* filter)` | - |


## Header: `G4SDParticleFilter.hh` (Target: `G4SDParticleFilter`)

**Kind**: C++ Class (`G4SDParticleFilter`) | **Total Extracted Public Methods**: 9

| C++ Method Signature | Comment / Description |
|---|---|
| `G4SDParticleFilter(G4String name)` | - |
| `G4SDParticleFilter(G4String name, const G4String& particleName)` | - |
| `G4SDParticleFilter(G4String name, const std::vector<G4String>& particleNames)` | - |
| `G4SDParticleFilter(G4String name, const std::vector<G4ParticleDefinition*>& particleDef)` | - |
| `~G4SDParticleFilter() override = default` | Constructors. Filter name and particle's name. |
| `G4bool Accept(const G4Step*) const override` | - |
| `void add(const G4String& particleName)` | - |
| `void addIon(G4int Z, G4int A)` | set method for acceptable particle name. |
| `void show()` | - |


## Header: `G4SDKineticEnergyFilter.hh` (Target: `G4SDKineticEnergyFilter`)

**Kind**: C++ Class (`G4SDKineticEnergyFilter`) | **Total Extracted Public Methods**: 7

| C++ Method Signature | Comment / Description |
|---|---|
| `G4SDKineticEnergyFilter(G4String name, G4double elow = 0.0, G4double ehigh = DBL_MAX)` | - |
| `~G4SDKineticEnergyFilter() override = default` | Constructor. Filter name and kinetic energy range( elow, ehigh). |
| `G4bool Accept(const G4Step*) const override` | - |
| `void SetKineticEnergy(G4double elow, G4double ehigh)` | - |
| `void SetLowEnergy(G4double elow)` | - |
| `void SetHighEnergy(G4double ehigh)` | - |
| `void show()` | Set methods for kinetic energy range. |


## Header: `G4VAnalysisManager.hh` (Target: `G4VAnalysisManager`)

**Kind**: C++ Class (`G4VAnalysisManager`) | **Total Extracted Public Methods**: 296

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VAnalysisManager() = delete` | - |
| `G4bool OpenFile(const G4String& fileName = "")` | Methods for handling files |
| `G4bool Write()` | - |
| `G4bool CloseFile(G4bool reset = true)` | - |
| `G4bool Reset()` | - |
| `void Clear()` | - |
| `G4bool Merge(tools::histo::hmpi* hmpi)` | - |
| `G4bool Plot()` | - |
| `G4bool IsOpenFile() const` | - |
| `void SetDefaultFileType(const G4String& value)` | Method for handling default file type |
| `G4String GetDefaultFileType() const` | - |
| `G4bool SetFileName(const G4String& fileName)` | Methods for handling files and directories names |
| `G4bool SetHistoDirectoryName(const G4String& dirName)` | - |
| `G4bool SetNtupleDirectoryName(const G4String& dirName)` | - |
| `void SetCompressionLevel(G4int level)` | - |
| `G4String GetFileName() const` | - |
| `G4String GetHistoDirectoryName() const` | - |
| `G4String GetNtupleDirectoryName() const` | - |
| `G4int GetCompressionLevel() const` | - |
| `G4int CreateH1(const G4String& name, const G4String& title, G4int nbins, G4double xmin, G4double xmax, const G4String& unitName = "none", const G4String& fcnName = "none", const G4String& binSchemeName = "linear")` | Methods for handling histograms |
| `G4int CreateH1(const G4String& name, const G4String& title, const std::vector<G4double>& edges, const G4String& unitName = "none", const G4String& fcnName = "none")` | - |
| `G4int CreateH2(const G4String& name, const G4String& title, G4int nxbins, G4double xmin, G4double xmax, G4int nybins, G4double ymin, G4double ymax, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& xbinSchemeName = "linear", const G4String& ybinSchemeName = "linear")` | - |
| `G4int CreateH2(const G4String& name, const G4String& title, const std::vector<G4double>& xedges, const std::vector<G4double>& yedges, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none")` | - |
| `G4int CreateH3(const G4String& name, const G4String& title, G4int nxbins, G4double xmin, G4double xmax, G4int nybins, G4double ymin, G4double ymax, G4int nzbins, G4double zmin, G4double zmax, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none", const G4String& xbinSchemeName = "linear", const G4String& ybinSchemeName = "linear", const G4String& zbinSchemeName = "linear")` | - |
| `G4int CreateH3(const G4String& name, const G4String& title, const std::vector<G4double>& xedges, const std::vector<G4double>& yedges, const std::vector<G4double>& zedges, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none")` | - |
| `G4bool SetH1(G4int id, G4int nbins, G4double xmin, G4double xmax, const G4String& unitName = "none", const G4String& fcnName = "none", const G4String& binSchemeName = "linear")` | - |
| `G4bool SetH1(G4int id, const std::vector<G4double>& edges, const G4String& unitName = "none", const G4String& fcnName = "none")` | - |
| `G4bool SetH2(G4int id, G4int nxbins, G4double xmin, G4double xmax, G4int nybins, G4double ymin, G4double ymax, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& xbinSchemeName = "linear", const G4String& ybinSchemeName = "linear")` | - |
| `G4bool SetH2(G4int id, const std::vector<G4double>& xedges, const std::vector<G4double>& yedges, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none")` | - |
| `G4bool SetH3(G4int id, G4int nxbins, G4double xmin, G4double xmax, G4int nzbins, G4double zmin, G4double zmax, G4int nybins, G4double ymin, G4double ymax, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none", const G4String& xbinSchemeName = "linear", const G4String& ybinSchemeName = "linear", const G4String& zbinSchemeName = "linear")` | - |
| `G4bool SetH3(G4int id, const std::vector<G4double>& xedges, const std::vector<G4double>& yedges, const std::vector<G4double>& zedges, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none")` | - |
| `G4bool ScaleH1(G4int id, G4double factor)` | - |
| `G4bool ScaleH2(G4int id, G4double factor)` | - |
| `G4bool ScaleH3(G4int id, G4double factor)` | - |
| `G4int CreateP1(const G4String& name, const G4String& title, G4int nbins, G4double xmin, G4double xmax, G4double ymin = 0, G4double ymax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& xbinSchemeName = "linear")` | Methods for handling profiles |
| `G4int CreateP1(const G4String& name, const G4String& title, const std::vector<G4double>& edges, G4double ymin = 0, G4double ymax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none")` | - |
| `G4int CreateP2(const G4String& name, const G4String& title, G4int nxbins, G4double xmin, G4double xmax, G4int nybins, G4double ymin, G4double ymax, G4double zmin = 0, G4double zmax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none", const G4String& xbinSchemeName = "linear", const G4String& ybinSchemeName = "linear")` | - |
| `G4int CreateP2(const G4String& name, const G4String& title, const std::vector<G4double>& xedges, const std::vector<G4double>& yedges, G4double zmin = 0, G4double zmax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none")` | - |
| `G4bool SetP1(G4int id, G4int nbins, G4double xmin, G4double xmax, G4double ymin = 0, G4double ymax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& xbinSchemeName = "linear")` | - |
| `G4bool SetP1(G4int id, const std::vector<G4double>& edges, G4double ymin = 0, G4double ymax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none")` | - |
| `G4bool SetP2(G4int id, G4int nxbins, G4double xmin, G4double xmax, G4int nybins, G4double ymin, G4double ymax, G4double zmin = 0, G4double zmax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none", const G4String& xbinSchemeName = "linear", const G4String& ybinSchemeName = "linear")` | - |
| `G4bool SetP2(G4int id, const std::vector<G4double>& xedges, const std::vector<G4double>& yedges, G4double zmin = 0, G4double zmax = 0, const G4String& xunitName = "none", const G4String& yunitName = "none", const G4String& zunitName = "none", const G4String& xfcnName = "none", const G4String& yfcnName = "none", const G4String& zfcnName = "none")` | - |
| `G4bool ScaleP1(G4int id, G4double factor)` | - |
| `G4bool ScaleP2(G4int id, G4double factor)` | - |
| `G4int CreateNtuple(const G4String& name, const G4String& title)` | Methods for handling ntuples |
| `G4int CreateNtupleIColumn(const G4String& name)` | Create columns in the last created ntuple |
| `G4int CreateNtupleFColumn(const G4String& name)` | - |
| `G4int CreateNtupleDColumn(const G4String& name)` | - |
| `G4int CreateNtupleSColumn(const G4String& name)` | - |
| `G4int CreateNtupleIColumn( const G4String& name, std::vector<int>& vector)` | Create columns of vector in the last created ntuple |
| `G4int CreateNtupleFColumn( const G4String& name, std::vector<float>& vector)` | - |
| `G4int CreateNtupleDColumn( const G4String& name, std::vector<double>& vector)` | - |
| `G4int CreateNtupleSColumn( const G4String& name, std::vector<std::string>& vector)` | - |
| `void FinishNtuple()` | - |
| `G4int CreateNtupleIColumn(G4int ntupleId, const G4String& name)` | Create columns in the ntuple with given id |
| `G4int CreateNtupleFColumn(G4int ntupleId, const G4String& name)` | - |
| `G4int CreateNtupleDColumn(G4int ntupleId, const G4String& name)` | - |
| `G4int CreateNtupleSColumn(G4int ntupleId, const G4String& name)` | - |
| `G4int CreateNtupleIColumn(G4int ntupleId, const G4String& name, std::vector<int>& vector)` | Create columns of vector in the ntuple with given id |
| `G4int CreateNtupleFColumn(G4int ntupleId, const G4String& name, std::vector<float>& vector)` | - |
| `G4int CreateNtupleDColumn(G4int ntupleId, const G4String& name, std::vector<double>& vector)` | - |
| `G4int CreateNtupleSColumn(G4int ntupleId, const G4String& name, std::vector<std::string>& vector)` | - |
| `void FinishNtuple(G4int ntupleId)` | - |
| `virtual void SetNtupleMerging(G4bool mergeNtuples, G4int nofReducedNtupleFiles = 0)` | MT/MPI |
| `virtual void SetNtupleRowWise(G4bool rowWise, G4bool rowMode = true)` | - |
| `virtual void SetBasketSize(unsigned int basketSize)` | - |
| `virtual void SetBasketEntries(unsigned int basketEntries)` | - |
| `G4bool SetFirstHistoId(G4int firstId)` | The ids of histograms and ntuples are generated automatically starting from 0; with following functions it is possible to change the first Id to start from other value |
| `G4bool SetFirstH1Id(G4int firstId)` | - |
| `G4bool SetFirstH2Id(G4int firstId)` | - |
| `G4bool SetFirstH3Id(G4int firstId)` | - |
| `G4bool SetFirstProfileId(G4int firstId)` | - |
| `G4bool SetFirstP1Id(G4int firstId)` | - |
| `G4bool SetFirstP2Id(G4int firstId)` | - |
| `G4bool SetFirstNtupleId(G4int firstId)` | - |
| `G4bool SetFirstNtupleColumnId(G4int firstId)` | - |
| `G4bool FillH1(G4int id, G4double value, G4double weight = 1.0)` | Methods to fill histograms |
| `G4bool FillH2(G4int id, G4double xvalue, G4double yvalue, G4double weight = 1.0)` | - |
| `G4bool FillH3(G4int id, G4double xvalue, G4double yvalue, G4double zvalue, G4double weight = 1.0)` | - |
| `G4bool FillP1(G4int id, G4double xvalue, G4double yvalue, G4double weight = 1.0)` | Methods to fill profiles |
| `G4bool FillP2(G4int id, G4double xvalue, G4double yvalue, G4double zvalue, G4double weight = 1.0)` | - |
| `G4bool FillNtupleIColumn(G4int id, G4int value)` | Methods to fill ntuples Methods for ntuple with id = FirstNtupleId |
| `G4bool FillNtupleFColumn(G4int id, G4float value)` | - |
| `G4bool FillNtupleDColumn(G4int id, G4double value)` | - |
| `G4bool FillNtupleSColumn(G4int id, const G4String& value)` | - |
| `G4bool AddNtupleRow()` | - |
| `G4bool FillNtupleIColumn(G4int ntupleId, G4int columnId, G4int value)` | Methods for ntuple with id > FirstNtupleId (when more ntuples exist) |
| `G4bool FillNtupleFColumn(G4int ntupleId, G4int columnId, G4float value)` | - |
| `G4bool FillNtupleDColumn(G4int ntupleId, G4int columnId, G4double value)` | - |
| `G4bool FillNtupleSColumn(G4int ntupleId, G4int id, const G4String& value)` | - |
| `G4bool AddNtupleRow(G4int ntupleId)` | - |
| `void SetActivation(G4bool activation)` | Activation option When this option is enabled, only the histograms/profiles marked as activated are returned, filled or saved on file. No warning is issued when Get or Fill is called on inactive histogram. |
| `G4bool GetActivation() const` | - |
| `G4bool IsActive() const` | Return false if activation is enabled and there is no object activated, return true otherwise |
| `G4bool IsAscii() const` | ASCII option Return false if there is no object selected for ASCII output, return true otherwise |
| `G4bool IsPlotting() const` | Plotting option Return false if there is no object selected for plottng, return true otherwise |
| `G4int GetFirstH1Id() const` | Access methods |
| `G4int GetFirstH2Id() const` | - |
| `G4int GetFirstH3Id() const` | - |
| `G4int GetFirstP1Id() const` | - |
| `G4int GetFirstP2Id() const` | - |
| `G4int GetFirstNtupleId() const` | - |
| `G4int GetFirstNtupleColumnId() const` | - |
| `G4int GetNofH1s(G4bool onlyIfExist = false) const` | - |
| `G4int GetNofH2s(G4bool onlyIfExist = false) const` | - |
| `G4int GetNofH3s(G4bool onlyIfExist = false) const` | - |
| `G4int GetNofP1s(G4bool onlyIfExist = false) const` | - |
| `G4int GetNofP2s(G4bool onlyIfExist = false) const` | - |
| `G4int GetNofNtuples(G4bool onlyIfExist = false) const` | - |
| `G4int GetH1Id(const G4String& name, G4bool warn = true) const` | Access methods via names |
| `G4int GetH2Id(const G4String& name, G4bool warn = true) const` | - |
| `G4int GetH3Id(const G4String& name, G4bool warn = true) const` | - |
| `G4int GetP1Id(const G4String& name, G4bool warn = true) const` | - |
| `G4int GetP2Id(const G4String& name, G4bool warn = true) const` | - |
| `G4bool ListH1(G4bool onlyIfActive = true) const` | List objects |
| `G4bool ListH2(G4bool onlyIfActive = true) const` | - |
| `G4bool ListH3(G4bool onlyIfActive = true) const` | - |
| `G4bool ListP1(G4bool onlyIfActive = true) const` | - |
| `G4bool ListP2(G4bool onlyIfActive = true) const` | - |
| `G4bool ListNtuple(G4bool onlyIfActive = true) const` | - |
| `G4bool List(G4bool onlyIfActive = true) const` | - |
| `void SetH1Activation(G4bool activation)` | Methods to manipulate histogram, profiles & ntuples additional information |
| `void SetH1Activation(G4int id, G4bool activation)` | - |
| `void SetH1Ascii(G4int id, G4bool ascii)` | - |
| `void SetH1Plotting(G4int id, G4bool plotting)` | - |
| `void SetH1FileName(G4int id, const G4String& fileName)` | - |
| `void SetH2Activation(G4bool activation)` | - |
| `void SetH2Activation(G4int id, G4bool activation)` | - |
| `void SetH2Ascii(G4int id, G4bool ascii)` | - |
| `void SetH2Plotting(G4int id, G4bool plotting)` | - |
| `void SetH2FileName(G4int id, const G4String& fileName)` | - |
| `void SetH3Activation(G4bool activation)` | - |
| `void SetH3Activation(G4int id, G4bool activation)` | - |
| `void SetH3Ascii(G4int id, G4bool ascii)` | - |
| `void SetH3Plotting(G4int id, G4bool plotting)` | - |
| `void SetH3FileName(G4int id, const G4String& fileName)` | - |
| `void SetP1Activation(G4bool activation)` | - |
| `void SetP1Activation(G4int id, G4bool activation)` | - |
| `void SetP1Ascii(G4int id, G4bool ascii)` | - |
| `void SetP1Plotting(G4int id, G4bool plotting)` | - |
| `void SetP1FileName(G4int id, const G4String& fileName)` | - |
| `void SetP2Activation(G4bool activation)` | - |
| `void SetP2Activation(G4int id, G4bool activation)` | - |
| `void SetP2Ascii(G4int id, G4bool ascii)` | - |
| `void SetP2Plotting(G4int id, G4bool plotting)` | - |
| `void SetP2FileName(G4int id, const G4String& fileName)` | - |
| `void SetNtupleActivation(G4bool activation)` | - |
| `void SetNtupleActivation(G4int id, G4bool activation)` | - |
| `void SetNtupleFileName(const G4String& fileName)` | - |
| `void SetNtupleFileName(G4int id, const G4String& fileName)` | - |
| `G4int GetH1Nbins(G4int id) const` | Access to histogram & profiles parameters |
| `G4double GetH1Xmin(G4int id) const` | - |
| `G4double GetH1Xmax(G4int id) const` | - |
| `G4double GetH1Width(G4int id) const` | - |
| `G4int GetH2Nxbins(G4int id) const` | - |
| `G4double GetH2Xmin(G4int id) const` | - |
| `G4double GetH2Xmax(G4int id) const` | - |
| `G4double GetH2XWidth(G4int id) const` | - |
| `G4int GetH2Nybins(G4int id) const` | - |
| `G4double GetH2Ymin(G4int id) const` | - |
| `G4double GetH2Ymax(G4int id) const` | - |
| `G4double GetH2YWidth(G4int id) const` | - |
| `G4int GetH3Nxbins(G4int id) const` | - |
| `G4double GetH3Xmin(G4int id) const` | - |
| `G4double GetH3Xmax(G4int id) const` | - |
| `G4double GetH3XWidth(G4int id) const` | - |
| `G4int GetH3Nybins(G4int id) const` | - |
| `G4double GetH3Ymin(G4int id) const` | - |
| `G4double GetH3Ymax(G4int id) const` | - |
| `G4double GetH3YWidth(G4int id) const` | - |
| `G4int GetH3Nzbins(G4int id) const` | - |
| `G4double GetH3Zmin(G4int id) const` | - |
| `G4double GetH3Zmax(G4int id) const` | - |
| `G4double GetH3ZWidth(G4int id) const` | - |
| `G4int GetP1Nbins(G4int id) const` | - |
| `G4double GetP1Xmin(G4int id) const` | - |
| `G4double GetP1Xmax(G4int id) const` | - |
| `G4double GetP1XWidth(G4int id) const` | - |
| `G4double GetP1Ymin(G4int id) const` | - |
| `G4double GetP1Ymax(G4int id) const` | - |
| `G4int GetP2Nxbins(G4int id) const` | - |
| `G4double GetP2Xmin(G4int id) const` | - |
| `G4double GetP2Xmax(G4int id) const` | - |
| `G4double GetP2XWidth(G4int id) const` | - |
| `G4int GetP2Nybins(G4int id) const` | - |
| `G4double GetP2Ymin(G4int id) const` | - |
| `G4double GetP2Ymax(G4int id) const` | - |
| `G4double GetP2YWidth(G4int id) const` | - |
| `G4double GetP2Zmin(G4int id) const` | - |
| `G4double GetP2Zmax(G4int id) const` | - |
| `G4String GetH1Name(G4int id) const` | Access to histogram & profiles additional information |
| `G4double GetH1Unit(G4int id) const` | - |
| `G4bool GetH1Activation(G4int id) const` | - |
| `G4bool GetH1Ascii(G4int id) const` | - |
| `G4bool GetH1Plotting(G4int id) const` | - |
| `G4String GetH1FileName(G4int id) const` | - |
| `G4String GetH2Name(G4int id) const` | - |
| `G4double GetH2XUnit(G4int id) const` | - |
| `G4double GetH2YUnit(G4int id) const` | - |
| `G4bool GetH2Activation(G4int id) const` | - |
| `G4bool GetH2Ascii(G4int id) const` | - |
| `G4bool GetH2Plotting(G4int id) const` | - |
| `G4String GetH3Name(G4int id) const` | - |
| `G4double GetH3XUnit(G4int id) const` | - |
| `G4double GetH3YUnit(G4int id) const` | - |
| `G4double GetH3ZUnit(G4int id) const` | - |
| `G4bool GetH3Activation(G4int id) const` | - |
| `G4bool GetH3Ascii(G4int id) const` | - |
| `G4bool GetH3Plotting(G4int id) const` | - |
| `G4String GetP1Name(G4int id) const` | - |
| `G4double GetP1XUnit(G4int id) const` | - |
| `G4double GetP1YUnit(G4int id) const` | - |
| `G4bool GetP1Activation(G4int id) const` | - |
| `G4bool GetP1Ascii(G4int id) const` | - |
| `G4bool GetP1Plotting(G4int id) const` | - |
| `G4String GetP2Name(G4int id) const` | - |
| `G4double GetP2XUnit(G4int id) const` | - |
| `G4double GetP2YUnit(G4int id) const` | - |
| `G4double GetP2ZUnit(G4int id) const` | - |
| `G4bool GetP2Activation(G4int id) const` | - |
| `G4bool GetP2Ascii(G4int id) const` | - |
| `G4bool GetP2Plotting(G4int id) const` | - |
| `G4bool GetNtupleActivation(G4int id) const` | - |
| `G4String GetNtupleFileName(G4int id) const` | - |
| `G4bool SetH1Title(G4int id, const G4String& title)` | Setters for histogram & profiles attributes for plotting |
| `G4bool SetH1XAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH1YAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH1XAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetH1YAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetH2Title(G4int id, const G4String& title)` | - |
| `G4bool SetH2XAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH2YAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH2ZAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH2XAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetH2YAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetH2ZAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetH3Title(G4int id, const G4String& title)` | - |
| `G4bool SetH3XAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH3YAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH3ZAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetH3XAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetH3YAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetH3ZAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetP1Title(G4int id, const G4String& title)` | - |
| `G4bool SetP1XAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetP1YAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetP1XAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetP1YAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetP2Title(G4int id, const G4String& title)` | - |
| `G4bool SetP2XAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetP2YAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetP2ZAxisTitle(G4int id, const G4String& title)` | - |
| `G4bool SetP2XAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetP2YAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4bool SetP2ZAxisIsLog(G4int id, G4bool isLog)` | - |
| `G4String GetH1Title(G4int id) const` | Access histogram & profiles attributes for plotting |
| `G4String GetH1XAxisTitle(G4int id) const` | - |
| `G4String GetH1YAxisTitle(G4int id) const` | - |
| `G4bool GetH1XAxisIsLog(G4int id) const` | - |
| `G4bool GetH1YAxisIsLog(G4int id) const` | - |
| `G4String GetH2Title(G4int id) const` | - |
| `G4String GetH2XAxisTitle(G4int id) const` | - |
| `G4String GetH2YAxisTitle(G4int id) const` | - |
| `G4String GetH2ZAxisTitle(G4int id) const` | - |
| `G4bool GetH2XAxisIsLog(G4int id) const` | - |
| `G4bool GetH2YAxisIsLog(G4int id) const` | - |
| `G4bool GetH2ZAxisIsLog(G4int id) const` | - |
| `G4String GetH3Title(G4int id) const` | - |
| `G4String GetH3XAxisTitle(G4int id) const` | - |
| `G4String GetH3YAxisTitle(G4int id) const` | - |
| `G4String GetH3ZAxisTitle(G4int id) const` | - |
| `G4bool GetH3XAxisIsLog(G4int id) const` | - |
| `G4bool GetH3YAxisIsLog(G4int id) const` | - |
| `G4bool GetH3ZAxisIsLog(G4int id) const` | - |
| `G4String GetP1Title(G4int id) const` | - |
| `G4String GetP1XAxisTitle(G4int id) const` | - |
| `G4String GetP1YAxisTitle(G4int id) const` | - |
| `G4bool GetP1XAxisIsLog(G4int id) const` | - |
| `G4bool GetP1YAxisIsLog(G4int id) const` | - |
| `G4String GetP2Title(G4int id) const` | - |
| `G4String GetP2XAxisTitle(G4int id) const` | - |
| `G4String GetP2YAxisTitle(G4int id) const` | - |
| `G4String GetP2ZAxisTitle(G4int id) const` | - |
| `G4bool GetP2XAxisIsLog(G4int id) const` | - |
| `G4bool GetP2YAxisIsLog(G4int id) const` | - |
| `G4bool GetP2ZAxisIsLog(G4int id) const` | - |
| `G4bool DeleteH1(G4int id, G4bool keepSetting = false)` | New methods for deleting selected objects |
| `G4bool DeleteH2(G4int id, G4bool keepSetting = false)` | - |
| `G4bool DeleteH3(G4int id, G4bool keepSetting = false)` | - |
| `G4bool DeleteP1(G4int id, G4bool keepSetting = false)` | - |
| `G4bool DeleteP2(G4int id, G4bool keepSetting = false)` | - |
| `G4bool DeleteNtuple(G4int id, G4bool clear = false)` | - |
| `void SetVerboseLevel(G4int verboseLevel)` | Verbosity |
| `G4int GetVerboseLevel() const` | - |
| `G4String GetType() const` | The manager type (starts with an uppercase letter) |
| `G4String GetFileType() const` | The manager file type (starts with a lowercase letter) |


## Header: `G4AnalysisManager.hh` (Target: `G4AnalysisManager`)

**Kind**: Global Utilities / Functions

| Signature | Description |
|---|---|


## Header: `G4UserRunAction.hh` (Target: `G4UserRunAction`)

**Kind**: C++ Class (`G4UserRunAction`) | **Total Extracted Public Methods**: 7

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UserRunAction()` | - |
| `virtual ~G4UserRunAction() = default` | - |
| `virtual G4Run* GenerateRun()` | - |
| `virtual void BeginOfRunAction(const G4Run* /*aRun*/)` | - |
| `virtual void EndOfRunAction(const G4Run* /*aRun*/)` | - |
| `inline virtual void SetMaster(G4bool val = true)` | - |
| `inline G4bool IsMaster() const` | - |


## Header: `G4UserEventAction.hh` (Target: `G4UserEventAction`)

**Kind**: C++ Class (`G4UserEventAction`) | **Total Extracted Public Methods**: 6

| C++ Method Signature | Comment / Description |
|---|---|
| `G4UserEventAction()` | - |
| `virtual ~G4UserEventAction() = default` | - |
| `virtual void SetEventManager(G4EventManager* value)` | - |
| `virtual void BeginOfEventAction(const G4Event* anEvent)` | - |
| `virtual void EndOfEventAction(const G4Event* anEvent)` | - |
| `virtual void MergeSubEvent(G4Event* masterEvent, const G4Event* subEvent)` | Two virtual method the user can override. |


## Header: `G4Run.hh` (Target: `G4Run`)

**Kind**: C++ Class (`G4Run`) | **Total Extracted Public Methods**: 20

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Run()` | - |
| `virtual ~G4Run()` | - |
| `G4Run(const G4Run&) = delete` | - |
| `G4Run& operator=(const G4Run&) = delete` | - |
| `virtual void RecordEvent(const G4Event*)` | Method to be overwritten by the user for recording events in this run. In such a case, it is the user's responsibility to increment numberOfEvent. Also, user's run class object must be instantiated in user's runAction. |
| `virtual void Merge(const G4Run*)` | Method to be overwritten by the user for merging local G4Run object to the global G4Run object. |
| `virtual void MergeSubEvent(G4Event* masterEv, const G4Event* subEv)` | Method to be overwritten by the user for merging sub-event results (e.g. hits) into the master G4Event. [N.B.] Trajectories are merged in the base-class method. So, the user should invoke the base-class method at the end of his/her overwriting method. |
| `void StoreEvent(G4Event* evt)` | Store a G4Event object until this run object is deleted. Given the potential large memory size of G4Event and its data-member objects stored in G4Event, the user must be careful and responsible for not storing too many G4Event objects. This method is invoked by G4RunManager if the user invokes G4EventManager::KeepTheCurrentEvent() or "/event/keepCurrentEvent" UI command while the particular event is in being processed (typically in EndOfEventAction). |
| `inline G4int GetRunID() const` | Returns the run ID. Run ID is set by G4RunManager. |
| `inline G4int GetNumberOfEvent() const` | Returns number of events processed in this run. The number is incremented at the end of each event processing. |
| `inline G4int GetNumberOfEventToBeProcessed() const` | - |
| `inline const G4HCtable* GetHCtable() const` | Returns hits collection. |
| `inline const G4DCtable* GetDCtable() const` | Returns digi collection. |
| `inline const G4String& GetRandomNumberStatus() const` | Returns random number status at the beginning of this run. |
| `inline std::vector<const G4Event*>* GetEventVector() const` | Returns the event vector. |
| `inline void SetRunID(G4int id)` | - |
| `inline void SetNumberOfEventToBeProcessed(G4int n_ev)` | - |
| `inline void SetHCtable(G4HCtable* HCtbl)` | - |
| `inline void SetDCtable(G4DCtable* DCtbl)` | - |
| `inline void SetRandomNumberStatus(G4String& st)` | - |


## Header: `G4Event.hh` (Target: `G4Event`)

**Kind**: C++ Class (`G4Event`) | **Total Extracted Public Methods**: 47

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Event() = default` | - |
| `explicit G4Event(G4int evID)` | - |
| `~G4Event()` | - |
| `G4Event(const G4Event &) = delete` | - |
| `G4Event& operator=(const G4Event &) = delete` | - |
| `inline void *operator new(std::size_t)` | - |
| `inline void operator delete(void* anEvent)` | - |
| `G4bool operator==(const G4Event& right) const` | - |
| `G4bool operator!=(const G4Event& right) const` | - |
| `void Print() const` | - |
| `void Draw() const` | Print the event ID (starts with zero and increments by one) to G4cout. |
| `inline void SetEventID(G4int i)` | Invoke Draw() methods of all stored trajectories, hits, and digits. For hits and digits, Draw() methods of the concrete classes must be implemented. Otherwise nothing will be drawn. |
| `inline void SetHCofThisEvent(G4HCofThisEvent* value)` | - |
| `inline void SetDCofThisEvent(G4DCofThisEvent* value)` | - |
| `inline void SetTrajectoryContainer(G4TrajectoryContainer* value)` | - |
| `inline void SetEventAborted()` | - |
| `inline void SetRandomNumberStatus(G4String& st)` | - |
| `randomNumberStatus = new G4String(st)` | - |
| `} inline void SetRandomNumberStatusForProcessing(G4String& st)` | - |
| `randomNumberStatusForProcessing = new G4String(st)` | - |
| `} inline void KeepTheEvent(G4bool vl=true)` | - |
| `inline G4bool ToBeKept() const` | - |
| `inline void KeepForPostProcessing() const` | - |
| `inline void PostProcessingFinished() const` | - |
| `if (grips<0)` | - |
| `G4Exception("G4Event::Release()", "EVENT91001", FatalException, "Number of grips is negative. This cannot be correct.")` | - |
| `} } inline G4int GetNumberOfGrips() const` | - |
| `inline G4int GetEventID() const` | - |
| `inline void AddPrimaryVertex(G4PrimaryVertex* aPrimaryVertex)` | - |
| `if( thePrimaryVertex == nullptr )` | This method sets a new primary vertex. This method must be invoked exclusively by G4VPrimaryGenerator concrete class. |
| `} inline G4int GetNumberOfPrimaryVertex() const` | - |
| `inline G4PrimaryVertex* GetPrimaryVertex(G4int i=0) const` | Returns number of primary verteces the G4Event object has. |
| `if( i == 0 )` | - |
| `if( i > 0 && i < numberOfPrimaryVertex )` | - |
| `for( G4int j=0; j<i; ++j )` | - |
| `if( primaryVertex == nullptr ) return nullptr` | - |
| `primaryVertex = primaryVertex->GetNext()` | - |
| `} inline G4HCofThisEvent* GetHCofThisEvent() const` | Returns i-th primary vertex of the event. |
| `inline G4DCofThisEvent* GetDCofThisEvent() const` | - |
| `inline G4TrajectoryContainer* GetTrajectoryContainer() const` | - |
| `inline G4bool IsAborted() const` | These three methods return the pointers to the G4HCofThisEvent (hits collections of this event), G4DCofThisEvent (digi collections of this event), and G4TrajectoryContainer (trajectory coonainer), respectively. |
| `inline void SetUserInformation(G4VUserEventInformation* anInfo)` | Return a boolean which indicates the event has been aborted and thus it should not be used for analysis. |
| `inline G4VUserEventInformation* GetUserInformation() const` | - |
| `inline const G4String& GetRandomNumberStatus() const` | Set and Get method of G4VUserEventInformation |
| `if(!validRandomNumberStatus)` | - |
| `} inline const G4String& GetRandomNumberStatusForProcessing() const` | - |
| `if(!validRandomNumberStatusForProcessing)` | - |

