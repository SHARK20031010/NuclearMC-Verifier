# Partition 3: Geometry & Material Composition APIs

> **Partition Scope**: Solids, logical volumes, physical placements, materials, elements, isotopes, and NIST manager.

> **Header Base Path**: `/home/shark/geant4/11.2.2-install/include/Geant4`

---

## Header: `G4VUserDetectorConstruction.hh` (Target: `G4VUserDetectorConstruction`)

**Kind**: C++ Class (`G4VUserDetectorConstruction`) | **Total Extracted Public Methods**: 11

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VUserDetectorConstruction() = default` | - |
| `virtual ~G4VUserDetectorConstruction() = default` | - |
| `virtual G4VPhysicalVolume* Construct() = 0` | - |
| `virtual void ConstructSDandField()` | This method is used in multi-threaded applications to build per-worker non-shared objects: SensitiveDetectors and Field managers. |
| `virtual void CloneSD()` | - |
| `virtual void CloneF()` | - |
| `void RegisterParallelWorld(G4VUserParallelWorld*)` | - |
| `G4int ConstructParallelGeometries()` | - |
| `void ConstructParallelSD()` | - |
| `G4int GetNumberOfParallelWorld() const` | - |
| `G4VUserParallelWorld* GetParallelWorld(G4int i) const` | - |


## Header: `G4LogicalVolume.hh` (Target: `G4LogicalVolume`)

**Kind**: C++ Class (`G4LogicalVolume`) | **Total Extracted Public Methods**: 65

| C++ Method Signature | Comment / Description |
|---|---|
| `G4LogicalVolume(G4VSolid* pSolid, G4Material* pMaterial, const G4String& name, G4FieldManager* pFieldMgr = nullptr, G4VSensitiveDetector* pSDetector = nullptr, G4UserLimits* pULimits = nullptr, G4bool optimise = true)` | - |
| `virtual ~G4LogicalVolume()` | Constructor. The solid and material pointer must be non null. The parameters for field, detector and user limits are optional. The volume also enters itself into the logical volume Store. Optimisation of the geometry (voxelisation) for the volume hierarchy is applied by default. For parameterised volumes in the hierarchy, optimisation is -always- applied. |
| `G4LogicalVolume(const G4LogicalVolume&) = delete` | Destructor. Removes the logical volume from the logical volume Store. This class is NOT meant to act as base class, except for exceptional circumstances of extended types used in the kernel. |
| `G4LogicalVolume& operator=(const G4LogicalVolume&) = delete` | - |
| `inline const G4String& GetName() const` | Copy-constructor and assignment operator not allowed. |
| `void SetName(const G4String& pName)` | - |
| `inline std::size_t GetNoDaughters() const` | Returns and sets the name of the logical volume. |
| `inline G4VPhysicalVolume* GetDaughter(const std::size_t i) const` | Returns the number of daughters (0 to n). |
| `void AddDaughter(G4VPhysicalVolume* p)` | Returns the ith daughter. Note numbering starts from 0, and no bounds checking is performed. |
| `inline G4bool IsDaughter(const G4VPhysicalVolume* p) const` | Adds the volume p as a daughter of the current logical volume. |
| `G4bool IsAncestor(const G4VPhysicalVolume* p) const` | Returns true if the volume p is a daughter of the current logical volume. |
| `void RemoveDaughter(const G4VPhysicalVolume* p)` | Returns true if the volume p is part of the hierarchy of volumes established by the current logical volume. Scans recursively the volume tree. |
| `void ClearDaughters()` | Removes the volume p from the List of daughter of the current logical volume. |
| `G4int TotalVolumeEntities() const` | Clears the list of daughters. Used by the phys-volume store when the geometry tree is cleared, since modified at run-time. |
| `inline EVolume CharacteriseDaughters() const` | Returns the total number of physical volumes (replicated or placed) in the tree represented by the current logical volume. |
| `inline EVolume DeduceDaughtersType() const` | Characterise the daughters of this logical volume. |
| `G4VSolid* GetSolid() const` | Used by CharacteriseDaughters(). |
| `void SetSolid(G4VSolid* pSolid)` | - |
| `G4Material* GetMaterial() const` | Gets and sets the current solid. |
| `void SetMaterial(G4Material* pMaterial)` | - |
| `void UpdateMaterial(G4Material* pMaterial)` | Gets and sets the current material. |
| `G4double GetMass(G4bool forced = false, G4bool propagate = true, G4Material* parMaterial = nullptr)` | Sets material and corresponding MaterialCutsCouple. This method is invoked by G4Navigator while it is navigating through material parameterization. |
| `void ResetMass()` | Returns the mass of the logical volume tree computed from the estimated geometrical volume of each solid and material associated to the logical volume and (by default) to its daughters. NOTE: the computation may require a considerable amount of time, depending from the complexity of the geometry tree. The returned value is cached and can be used for successive calls (default), unless recomputation is forced by providing 'true' for the boolean argument in input. Computation should be forced if the geometry setup has changed after the previous call. By setting the 'propagate' boolean flag to 'false' the method returns the mass of the present logical volume only (subtracted for the volume occupied by the daughter volumes). An optional argument to specify a material is also provided. |
| `G4FieldManager* GetFieldManager() const` | Ensure that cached value of Mass is invalidated - due to change in state, e.g. change of size of Solid, change of type of solid, or the addition/deletion of a daughter volume. |
| `void SetFieldManager(G4FieldManager* pFieldMgr, G4bool forceToAllDaughters)` | Gets current FieldManager. |
| `G4VSensitiveDetector* GetSensitiveDetector() const` | Sets FieldManager and propagates it: i) only to daughters with G4FieldManager = nullptr if forceToAllDaughters=false ii) to all daughters if forceToAllDaughters=true |
| `void SetSensitiveDetector(G4VSensitiveDetector* pSDetector)` | Gets current SensitiveDetector. |
| `inline G4UserLimits* GetUserLimits() const` | Sets SensitiveDetector (can be nullptr). |
| `inline void SetUserLimits(G4UserLimits *pULimits)` | - |
| `inline G4SmartVoxelHeader* GetVoxelHeader() const` | Gets and sets current UserLimits. |
| `inline void SetVoxelHeader(G4SmartVoxelHeader *pVoxel)` | - |
| `inline G4double GetSmartless() const` | Gets and sets current VoxelHeader. |
| `inline void SetSmartless(G4double s)` | - |
| `inline G4bool IsToOptimise() const` | Gets and sets user defined optimisation quality. |
| `inline void SetOptimisation(G4bool optim)` | Replies if geometry optimisation (voxelisation) is to be applied for this volume hierarchy. |
| `inline G4bool IsRootRegion() const` | Specifies if to apply or not geometry optimisation to this volume hierarchy. Note that for parameterised volumes in the hierarchy, optimisation is always applied. |
| `inline void SetRegionRootFlag(G4bool rreg)` | Replies if the logical volume represents a root region or not. |
| `inline G4bool IsRegion() const` | Sets/unsets the volume as a root region for cuts. |
| `inline void SetRegion(G4Region* reg)` | Replies if the logical volume is part of a cuts region or not. |
| `inline G4Region* GetRegion() const` | Sets/unsets the volume as cuts region. |
| `inline void PropagateRegion()` | Return the region to which the volume belongs, if any. |
| `const G4MaterialCutsCouple* GetMaterialCutsCouple() const` | Propagates region pointer to daughters. |
| `void SetMaterialCutsCouple(G4MaterialCutsCouple* cuts)` | - |
| `G4bool operator == (const G4LogicalVolume& lv) const` | Accessors for production cuts. |
| `const G4VisAttributes* GetVisAttributes () const` | Equality defined by address only. Returns true if objects are at same address, else false. |
| `void SetVisAttributes (const G4VisAttributes* pVA)` | - |
| `void SetVisAttributes (const G4VisAttributes& VA)` | - |
| `inline G4FastSimulationManager* GetFastSimulationManager () const` | Gets and sets visualization attributes. Arguments are converted to shared_ptr. |
| `inline void SetBiasWeight (G4double w)` | Gets current FastSimulationManager pointer if exists, otherwise null. |
| `inline G4double GetBiasWeight() const` | - |
| `G4LogicalVolume(__void__&)` | Sets and gets bias weight. |
| `virtual G4bool IsExtended() const` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `inline G4FieldManager* GetMasterFieldManager() const` | Return true if it is not a base-class object. |
| `inline G4VSensitiveDetector* GetMasterSensitiveDetector() const` | Gets current FieldManager for the master thread. |
| `inline G4VSolid* GetMasterSolid() const` | Gets current SensitiveDetector for the master thread. |
| `inline G4int GetInstanceID() const` | Gets current Solid for the master thread. |
| `static const G4LVManager& GetSubInstanceManager()` | Returns the instance ID. |
| `static void Clean()` | Returns the private data instance manager. |
| `inline void Lock()` | Clear memory allocated by sub-instance manager. |
| `void InitialiseWorker(G4LogicalVolume* ptrMasterObject, G4VSolid* pSolid, G4VSensitiveDetector* pSDetector)` | Set lock identifier for final deletion of entity. |
| `void TerminateWorker(G4LogicalVolume* ptrMasterObject)` | This method is similar to the constructor. It is used by each worker thread to achieve the partial effect as that of the master thread. |
| `void AssignFieldManager(G4FieldManager* fldMgr)` | This method is similar to the destructor. It is used by each worker thread to achieve the partial effect as that of the master thread. |
| `static G4VSolid* GetSolid(G4LVData& instLVdata) ; // const` | Set the FieldManager - only at this level (do not push down hierarchy) |
| `static void SetSolid(G4LVData& instLVdata, G4VSolid* pSolid)` | - |
| `G4bool ChangeDaughtersType(EVolume atype)` | Optimised Methods - passing thread instance of worker data |


## Header: `G4VPhysicalVolume.hh` (Target: `G4VPhysicalVolume`)

**Kind**: C++ Class (`G4VPhysicalVolume`) | **Total Extracted Public Methods**: 38

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VPhysicalVolume(G4RotationMatrix* pRot, const G4ThreeVector& tlate, const G4String& pName, G4LogicalVolume* pLogical, G4VPhysicalVolume* pMother)` | - |
| `virtual ~G4VPhysicalVolume()` | Initialise volume, positioned in a frame which is rotated by *pRot, relative to the coordinate system of the mother volume pMother. The center of the object is then placed at tlate in the new coordinates. If pRot=0 the volume is unrotated with respect to its mother. The physical volume is added to the mother's logical volume.  Must be called by all subclasses. pMother must point to a valid parent volume, except in the case of the world/top volume, when it =0.  The constructor also registers volume with physical volume Store. Note that the Store may be removed or dynamically built in future because of memory constraints. |
| `G4VPhysicalVolume(const G4VPhysicalVolume&) = delete` | Destructor, will be subclassed. Removes volume from volume Store. |
| `G4VPhysicalVolume& operator=(const G4VPhysicalVolume&) = delete` | - |
| `inline G4bool operator == (const G4VPhysicalVolume& p) const` | No copy constructor and assignment operator. |
| `G4RotationMatrix* GetObjectRotation() const; // Obsolete` | Equality defined by equal addresses only. Access functions  The following are accessor functions that make a distinction between whether the rotation/translation is being made for the frame or the object/volume that is being placed. (They are the inverse of each other). |
| `G4RotationMatrix GetObjectRotationValue() const; // Replacement` | - |
| `G4ThreeVector GetObjectTranslation() const` | - |
| `const G4RotationMatrix* GetFrameRotation() const` | Return the rotation/translation of the Object relative to the mother. |
| `G4ThreeVector GetFrameTranslation() const` | - |
| `const G4ThreeVector GetTranslation() const` | Return the rotation/translation of the Frame used to position this volume in its mother volume (opposite of object rot/trans). Older access functions, that do not distinguish between frame/object! |
| `const G4RotationMatrix* GetRotation() const` | - |
| `void SetTranslation(const G4ThreeVector& v)` | Old access functions, that do not distinguish between frame/object! They return the translation/rotation of the volume. Set functions |
| `G4RotationMatrix* GetRotation()` | - |
| `void SetRotation(G4RotationMatrix*)` | - |
| `inline G4LogicalVolume* GetLogicalVolume() const` | NOT INTENDED FOR GENERAL USE. Non constant versions of above. Used to change transformation for replication/parameterisation mechanism. |
| `inline void SetLogicalVolume(G4LogicalVolume* pLogical)` | Return the associated logical volume. |
| `inline G4LogicalVolume* GetMotherLogical() const` | Set the logical volume. Must not be called when geometry closed. |
| `inline void SetMotherLogical(G4LogicalVolume* pMother)` | Return the current mother logical volume pointer. |
| `inline const G4String& GetName() const` | Set the mother logical volume. Must not be called when geometry closed. |
| `void SetName(const G4String& pName)` | Return the volume's name. |
| `virtual G4int GetMultiplicity() const` | Set the volume's name. |
| `virtual EVolume VolumeType() const = 0` | Returns number of object entities (1 for normal placements, n for replicas or parameterised). Functions required of subclasses |
| `virtual G4bool IsMany() const = 0` | Characterise the type of volume - normal/replicated/parameterised. |
| `virtual G4int GetCopyNo() const = 0` | Return true if the volume is MANY (not implemented yet). |
| `virtual void SetCopyNo(G4int CopyNo) = 0` | Return the volumes copy number. |
| `virtual G4bool IsReplicated() const = 0` | Set the volumes copy number. |
| `virtual G4bool IsParameterised() const = 0` | Return true if replicated (single object instance represents many real volumes), else false. |
| `virtual G4VPVParameterisation* GetParameterisation() const = 0` | Return true if parameterised (single object instance represents many real parameterised volumes), else false. |
| `virtual void GetReplicationData(EAxis& axis, G4int& nReplicas, G4double& width, G4double& offset, G4bool& consuming) const = 0` | Return replicas parameterisation object (able to compute dimensions and transformations of replicas), or NULL if not applicable. |
| `virtual G4bool IsRegularStructure() const = 0` | Return replication information. No-op for no replicated volumes. |
| `virtual G4int GetRegularStructureId() const = 0` | Returns true if the underlying volume structure is regular. |
| `virtual G4bool CheckOverlaps(G4int res=1000, G4double tol=0., G4bool verbose=true, G4int errMax=1)` | Returns non-zero code in case the underlying volume structure is regular, voxel-like.  Value is id for structure type. If non-zero the volume is a candidate for specialised navigation such as 'nearest neighbour' directly on volumes. |
| `G4VPhysicalVolume(__void__&)` | Verifies if the placed volume is overlapping with existing daughters or with the mother volume. Provides default resolution for the number of points to be generated and verified. Concrete implementation is done and required only for placed and parameterised volumes. Returns true if the volume is overlapping. |
| `inline G4int GetInstanceID() const` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `static const G4PVManager& GetSubInstanceManager()` | Returns the instance ID. |
| `static void Clean()` | Returns the private data instance manager. |
| `inline EVolume DeduceVolumeType() const` | Clear memory allocated by sub-instance manager. |


## Header: `G4PVPlacement.hh` (Target: `G4PVPlacement`)

**Kind**: C++ Class (`G4PVPlacement`) | **Total Extracted Public Methods**: 19

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PVPlacement(G4RotationMatrix* pRot, const G4ThreeVector& tlate, G4LogicalVolume* pCurrentLogical, const G4String& pName, G4LogicalVolume* pMotherLogical, G4bool pMany, G4int pCopyNo, G4bool pSurfChk = false)` | - |
| `G4PVPlacement(const G4Transform3D& Transform3D, G4LogicalVolume* pCurrentLogical, const G4String& pName, G4LogicalVolume* pMotherLogical, G4bool pMany, G4int pCopyNo, G4bool pSurfChk = false)` | Initialise a single volume, positioned in a frame which is rotated by *pRot and traslated by tlate, relative to the coordinate system of the mother volume pMotherLogical. If pRot=nullptr the volume is unrotated with respect to its mother. The physical volume is added to the mother's logical volume. Arguments particular to G4PVPlacement: pMany Currently NOT used. For future use to identify if the volume is meant to be considered an overlapping structure, or not. pCopyNo should be set to 0 for the first volume of a given type. pSurfChk if true activates check for overlaps with existing volumes. This is a very natural way of defining a physical volume, and is especially useful when creating subdetectors: the mother volumes are not placed until a later stage of the assembly program. |
| `G4PVPlacement(G4RotationMatrix* pRot, const G4ThreeVector& tlate, const G4String& pName, G4LogicalVolume* pLogical, G4VPhysicalVolume* pMother, G4bool pMany, G4int pCopyNo, G4bool pSurfChk = false)` | Additional constructor, which expects a G4Transform3D that represents the direct rotation and translation of the solid (NOT of the frame). The G4Transform3D argument should be constructed by: i) First rotating it to align the solid to the system of reference of its mother volume *pMotherLogical, and ii) Then placing the solid at the location Transform3D.getTranslation(), with respect to the origin of the system of coordinates of the mother volume. [ This is useful for the people who prefer to think in terms of moving objects in a given reference frame. ] All other arguments are the same as for the previous constructor. |
| `G4PVPlacement(const G4Transform3D& Transform3D, const G4String& pName, G4LogicalVolume* pLogical, G4VPhysicalVolume* pMother, G4bool pMany, G4int pCopyNo, G4bool pSurfChk = false)` | A simple variation of the 1st constructor, only specifying the mother volume as a pointer to its physical volume instead of its logical volume. The effect is exactly the same. |
| `~G4PVPlacement() override` | Utilises both variations above (from 2nd and 3rd constructor). The effect is the same as for the 2nd constructor. |
| `inline G4int GetCopyNo() const override` | Default destructor. |
| `void SetCopyNo(G4int CopyNo) override` | - |
| `G4bool CheckOverlaps(G4int res = 1000, G4double tol = 0., G4bool verbose = true, G4int maxErr = 1) override` | Gets and sets the copy number of the volume. |
| `G4PVPlacement(__void__&)` | Verifies if the placed volume is overlapping with existing daughters or with the mother volume. Provides default resolution for the number of points to be generated and verified. A tolerance for the precision of the overlap check can be specified, by default it is set to maximum precision. Reports a maximum of overlaps errors according to parameter in input. Returns true if the volume is overlapping. |
| `G4PVPlacement(const G4PVPlacement&) = delete` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4PVPlacement& operator=(const G4PVPlacement&) = delete` | - |
| `G4bool IsMany() const override` | Copy constructor and assignment operator not allowed. |
| `G4bool IsReplicated() const override` | - |
| `G4bool IsParameterised() const override` | - |
| `G4VPVParameterisation* GetParameterisation() const override` | - |
| `void GetReplicationData(EAxis& axis, G4int& nReplicas, G4double& width, G4double& offset, G4bool& consuming) const override` | - |
| `G4bool IsRegularStructure() const override` | - |
| `G4int GetRegularStructureId() const override` | - |
| `EVolume VolumeType() const override` | - |


## Header: `G4PVReplica.hh` (Target: `G4PVReplica`)

**Kind**: C++ Class (`G4PVReplica`) | **Total Extracted Public Methods**: 23

| C++ Method Signature | Comment / Description |
|---|---|
| `void initialize()` | - |
| `G4PVReplica(const G4String& pName, G4LogicalVolume* pLogical, G4LogicalVolume* pMother, const EAxis pAxis, const G4int nReplicas, const G4double width, const G4double offset = 0.)` | - |
| `G4PVReplica(const G4String& pName, G4LogicalVolume* pLogical, G4VPhysicalVolume* pMother, const EAxis pAxis, const G4int nReplicas, const G4double width, const G4double offset = 0.)` | - |
| `G4PVReplica(__void__&)` | - |
| `G4PVReplica(const G4PVReplica&) = delete` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4PVReplica& operator=(const G4PVReplica&) = delete` | - |
| `~G4PVReplica() override` | Copy constructor and assignment operator not allowed |
| `EVolume VolumeType() const override` | - |
| `G4bool IsMany() const override` | - |
| `G4bool IsReplicated() const override` | - |
| `G4int GetCopyNo() const override` | - |
| `void SetCopyNo(G4int CopyNo) override` | - |
| `G4bool IsParameterised() const override` | - |
| `G4VPVParameterisation* GetParameterisation() const override` | - |
| `G4int GetMultiplicity() const override` | - |
| `void GetReplicationData(EAxis& axis, G4int& nReplicas, G4double& width, G4double& offset, G4bool& consuming) const override` | - |
| `virtual void SetRegularStructureId( G4int code )` | - |
| `G4bool IsRegularStructure() const override` | This method must set a unique code for each type of regular structure. - It must be called only during detector construction. - It can also be used to prepare any corresponding special navigation 'conditions'. |
| `G4int GetRegularStructureId() const override` | - |
| `inline G4int GetInstanceID() const` | Accessors for specialised geometries Methods for handling of MT instances |
| `static const G4PVRManager& GetSubInstanceManager()` | Returns the instance ID. |
| `void InitialiseWorker(G4PVReplica* pMasterObject)` | Returns the private data instance manager. |
| `void TerminateWorker(G4PVReplica* pMasterObject)` | This method is similar to the constructor. It is used by each worker thread to achieve the partial effect as that of the master thread. |


## Header: `G4Box.hh` (Target: `G4Box`)

**Kind**: C++ Class (`G4Box`) | **Total Extracted Public Methods**: 29

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Box(const G4String& pName, G4double pX, G4double pY, G4double pZ)` | - |
| `~G4Box() override` | Construct a box with name, and half lengths pX,pY,pZ |
| `void ComputeDimensions(G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep) override` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | - |
| `G4bool CalculateExtent(const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pMin, G4double& pMax) const override` | - |
| `inline G4double GetXHalfLength() const` | Accessors and modifiers |
| `inline G4double GetYHalfLength() const` | - |
| `inline G4double GetZHalfLength() const` | - |
| `void SetXHalfLength(G4double dx)` | - |
| `void SetYHalfLength(G4double dy)` | - |
| `void SetZHalfLength(G4double dz)` | - |
| `inline G4double GetCubicVolume() override` | Methods for solid |
| `inline G4double GetSurfaceArea() override` | - |
| `EInside Inside(const G4ThreeVector& p) const override` | - |
| `G4ThreeVector SurfaceNormal( const G4ThreeVector& p) const override` | - |
| `G4double DistanceToIn(const G4ThreeVector& p, const G4ThreeVector& v) const override` | - |
| `G4double DistanceToIn(const G4ThreeVector& p) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p, const G4ThreeVector& v, const G4bool calcNorm = false, G4bool* validNorm = nullptr, G4ThreeVector* n = nullptr) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p) const override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4ThreeVector GetPointOnSurface() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `std::ostream& StreamInfo(std::ostream& os) const override` | - |
| `void DescribeYourselfTo (G4VGraphicsScene& scene) const override` | Utilities for visualization |
| `G4VisExtent GetExtent () const override` | - |
| `G4Polyhedron* CreatePolyhedron () const override` | - |
| `G4Box(__void__&)` | - |
| `G4Box(const G4Box& rhs)` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4Box& operator=(const G4Box& rhs)` | - |


## Header: `G4Tubs.hh` (Target: `G4Tubs`)

**Kind**: C++ Class (`G4Tubs`) | **Total Extracted Public Methods**: 36

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Tubs( const G4String& pName, G4double pRMin, G4double pRMax, G4double pDz, G4double pSPhi, G4double pDPhi )` | - |
| `~G4Tubs() override` | Constructs a tubs with the given name and dimensions |
| `inline G4double GetInnerRadius () const` | Destructor Accessors |
| `inline G4double GetOuterRadius () const` | - |
| `inline G4double GetZHalfLength () const` | - |
| `inline G4double GetStartPhiAngle () const` | - |
| `inline G4double GetDeltaPhiAngle () const` | - |
| `inline G4double GetSinStartPhi () const` | - |
| `inline G4double GetCosStartPhi () const` | - |
| `inline G4double GetSinEndPhi () const` | - |
| `inline G4double GetCosEndPhi () const` | - |
| `inline void SetInnerRadius (G4double newRMin)` | Modifiers |
| `inline void SetOuterRadius (G4double newRMax)` | - |
| `inline void SetZHalfLength (G4double newDz)` | - |
| `inline void SetStartPhiAngle (G4double newSPhi, G4bool trig=true)` | - |
| `inline void SetDeltaPhiAngle (G4double newDPhi)` | - |
| `inline G4double GetCubicVolume() override` | Methods for solid |
| `inline G4double GetSurfaceArea() override` | - |
| `void ComputeDimensions( G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep ) override` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | - |
| `G4bool CalculateExtent(const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pmin, G4double& pmax) const override` | - |
| `EInside Inside( const G4ThreeVector& p ) const override` | - |
| `G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const override` | - |
| `G4double DistanceToIn(const G4ThreeVector& p, const G4ThreeVector& v) const override` | - |
| `G4double DistanceToIn(const G4ThreeVector& p) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p, const G4ThreeVector& v, const G4bool calcNorm = false, G4bool* validNorm = nullptr, G4ThreeVector* n = nullptr) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p) const override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4ThreeVector GetPointOnSurface() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `std::ostream& StreamInfo( std::ostream& os ) const override` | - |
| `void DescribeYourselfTo (G4VGraphicsScene& scene) const override` | Visualisation functions |
| `G4Polyhedron* CreatePolyhedron () const override` | - |
| `G4Tubs(__void__&)` | - |
| `G4Tubs(const G4Tubs& rhs)` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4Tubs& operator=(const G4Tubs& rhs)` | - |


## Header: `G4Sphere.hh` (Target: `G4Sphere`)

**Kind**: C++ Class (`G4Sphere`) | **Total Extracted Public Methods**: 43

| C++ Method Signature | Comment / Description |
|---|---|
| `G4Sphere(const G4String& pName, G4double pRmin, G4double pRmax, G4double pSPhi, G4double pDPhi, G4double pSTheta, G4double pDTheta)` | - |
| `~G4Sphere() override` | Constructs a sphere or sphere shell section with the given name and dimensions |
| `inline G4double GetInnerRadius () const` | Destructor Accessors |
| `inline G4double GetOuterRadius () const` | - |
| `inline G4double GetStartPhiAngle () const` | - |
| `inline G4double GetDeltaPhiAngle () const` | - |
| `inline G4double GetStartThetaAngle() const` | - |
| `inline G4double GetDeltaThetaAngle() const` | - |
| `inline G4double GetSinStartPhi () const` | - |
| `inline G4double GetCosStartPhi () const` | - |
| `inline G4double GetSinEndPhi () const` | - |
| `inline G4double GetCosEndPhi () const` | - |
| `inline G4double GetSinStartTheta () const` | - |
| `inline G4double GetCosStartTheta () const` | - |
| `inline G4double GetSinEndTheta () const` | - |
| `inline G4double GetCosEndTheta () const` | - |
| `inline void SetInnerRadius (G4double newRMin)` | Modifiers |
| `inline void SetOuterRadius (G4double newRmax)` | - |
| `inline void SetStartPhiAngle (G4double newSphi, G4bool trig = true)` | - |
| `inline void SetDeltaPhiAngle (G4double newDphi)` | - |
| `inline void SetStartThetaAngle(G4double newSTheta)` | - |
| `inline void SetDeltaThetaAngle(G4double newDTheta)` | - |
| `G4double GetCubicVolume() override` | Methods for solid |
| `G4double GetSurfaceArea() override` | - |
| `void ComputeDimensions( G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep) override` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | - |
| `G4bool CalculateExtent(const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pmin, G4double& pmax) const override` | - |
| `EInside Inside(const G4ThreeVector& p) const override` | - |
| `G4ThreeVector SurfaceNormal( const G4ThreeVector& p) const override` | - |
| `G4double DistanceToIn(const G4ThreeVector& p, const G4ThreeVector& v) const override` | - |
| `G4double DistanceToIn(const G4ThreeVector& p) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p, const G4ThreeVector& v, const G4bool calcNorm = false, G4bool* validNorm = nullptr, G4ThreeVector* n = nullptr) const override` | - |
| `G4double DistanceToOut(const G4ThreeVector& p) const override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4ThreeVector GetPointOnSurface() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `std::ostream& StreamInfo(std::ostream& os) const override` | - |
| `G4VisExtent GetExtent () const override` | Visualisation functions |
| `void DescribeYourselfTo(G4VGraphicsScene& scene) const override` | - |
| `G4Polyhedron* CreatePolyhedron() const override` | - |
| `G4Sphere(__void__&)` | - |
| `G4Sphere(const G4Sphere& rhs)` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4Sphere& operator=(const G4Sphere& rhs)` | - |


## Header: `G4SubtractionSolid.hh` (Target: `G4SubtractionSolid`)

**Kind**: C++ Class (`G4SubtractionSolid`) | **Total Extracted Public Methods**: 21

| C++ Method Signature | Comment / Description |
|---|---|
| `G4SubtractionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB )` | - |
| `G4SubtractionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB , G4RotationMatrix* rotMatrix, const G4ThreeVector& transVector )` | - |
| `G4SubtractionSolid( const G4String& pName, G4VSolid* pSolidA , G4VSolid* pSolidB , const G4Transform3D& transform )` | - |
| `~G4SubtractionSolid() override` | - |
| `G4GeometryType GetEntityType() const override` | - |
| `G4VSolid* Clone() const override` | - |
| `G4SubtractionSolid(__void__&)` | - |
| `G4SubtractionSolid(const G4SubtractionSolid& rhs)` | Fake default constructor for usage restricted to direct object persistency for clients requiring preallocation of memory for persistifiable objects. |
| `G4SubtractionSolid& operator=(const G4SubtractionSolid& rhs)` | - |
| `void BoundingLimits(G4ThreeVector& pMin, G4ThreeVector& pMax) const override` | Copy constructor and assignment operator. |
| `G4bool CalculateExtent( const EAxis pAxis, const G4VoxelLimits& pVoxelLimit, const G4AffineTransform& pTransform, G4double& pMin, G4double& pMax) const override` | - |
| `EInside Inside( const G4ThreeVector& p ) const override` | - |
| `G4ThreeVector SurfaceNormal( const G4ThreeVector& p ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p, const G4ThreeVector& v ) const override` | - |
| `G4double DistanceToIn( const G4ThreeVector& p) const override` | - |
| `G4double DistanceToOut( const G4ThreeVector& p, const G4ThreeVector& v, const G4bool calcNorm = false, G4bool* validNorm = nullptr, G4ThreeVector* n = nullptr ) const override` | - |
| `G4double DistanceToOut( const G4ThreeVector& p ) const override` | - |
| `void ComputeDimensions( G4VPVParameterisation* p, const G4int n, const G4VPhysicalVolume* pRep ) override` | - |
| `void DescribeYourselfTo ( G4VGraphicsScene& scene ) const override` | - |
| `G4Polyhedron* CreatePolyhedron () const override` | - |
| `G4double GetCubicVolume() final` | - |


## Header: `G4Material.hh` (Target: `G4Material`)

**Kind**: C++ Class (`G4Material`) | **Total Extracted Public Methods**: 55

| C++ Method Signature | Comment / Description |
|---|---|
| `public: // with description G4Material(const G4String& name, // its name G4double z, // atomic number G4double a, // mass of mole G4double density, // density G4State state = kStateUndefined, // solid,gas G4double temp = NTP_Temperature, // temperature G4double pressure = CLHEP::STP_Pressure); // pressure` | Constructor to create a material from single element |
| `G4Material(const G4String& name, // its name G4double density, // density G4int nComponents, // nbOfComponents G4State state = kStateUndefined, // solid,gas G4double temp = NTP_Temperature, // temperature G4double pressure = CLHEP::STP_Pressure); // pressure` | Constructor to create a material from a combination of elements and/or materials subsequently added via AddElement and/or AddMaterial |
| `G4Material(const G4String& name, // its name G4double density, // density const G4Material* baseMaterial, // base material G4State state = kStateUndefined, // solid,gas G4double temp = NTP_Temperature, // temperature G4double pressure = CLHEP::STP_Pressure); // pressure` | Constructor to create a material from the base material |
| `virtual ~G4Material()` | - |
| `void SetChemicalFormula(const G4String& chF)` | These methods allow customisation of corrections to ionisation computations. Free electron density above zero means that the material is a conductor. Computation of density effect correction of fly may be more accurate but require extra computations. |
| `void SetFreeElectronDensity(G4double val)` | - |
| `void ComputeDensityEffectOnFly(G4bool val)` | - |
| `G4Material(const G4Material&) = delete` | - |
| `const G4Material& operator=(const G4Material&) = delete` | - |
| `void AddElementByNumberOfAtoms(const G4Element* elm, G4int nAtoms)` | Add an element, giving number of atoms |
| `inline void AddElement(G4Element* elm, G4int nAtoms)` | - |
| `void AddElementByMassFraction(const G4Element* elm, G4double fraction)` | Add an element or material, giving fraction of mass |
| `inline void AddElement(G4Element* elm, G4double frac)` | - |
| `void AddMaterial(G4Material* material, G4double fraction)` | - |
| `inline const G4String& GetName() const` | retrieval methods |
| `inline const G4String& GetChemicalFormula() const` | - |
| `inline G4double GetFreeElectronDensity() const` | - |
| `inline G4double GetDensity() const` | - |
| `inline G4State GetState() const` | - |
| `inline G4double GetTemperature() const` | - |
| `inline G4double GetPressure() const` | - |
| `inline std::size_t GetNumberOfElements() const` | number of elements constituing this material: |
| `inline const G4ElementVector* GetElementVector() const` | vector of pointers to elements constituing this material: |
| `inline const G4double* GetFractionVector() const` | vector of fractional mass of each element: |
| `inline const G4int* GetAtomsVector() const` | vector of atom count of each element: |
| `inline const G4Element* GetElement(G4int iel) const` | return a pointer to an element, given its index in the material: |
| `inline const G4double* GetVecNbOfAtomsPerVolume() const` | vector of nb of atoms per volume of each element in this material: |
| `inline G4double GetTotNbOfAtomsPerVolume() const` | total number of atoms per volume: |
| `inline G4double GetTotNbOfElectPerVolume() const` | total number of electrons per volume: |
| `inline const G4double* GetAtomicNumDensityVector() const` | obsolete names (5-10-98) see the 2 functions above |
| `inline G4double GetElectronDensity() const` | - |
| `inline G4double GetRadlen() const` | Radiation length: |
| `inline G4double GetNuclearInterLength() const` | Nuclear interaction length |
| `inline G4IonisParamMat* GetIonisation() const` | ionisation parameters: |
| `inline G4SandiaTable* GetSandiaTable() const` | Sandia table: |
| `inline const G4Material* GetBaseMaterial() const` | Base material: |
| `inline const std::map<G4Material*, G4double>& GetMatComponents() const` | material components: |
| `inline G4double GetMassOfMolecule() const` | for chemical compound |
| `G4double GetZ() const` | meaningful only for single material: |
| `G4double GetA() const` | - |
| `void SetMaterialPropertiesTable(G4MaterialPropertiesTable* anMPT)` | the MaterialPropertiesTable (if any) attached to this material: |
| `inline G4MaterialPropertiesTable* GetMaterialPropertiesTable() const` | - |
| `} inline std::size_t GetIndex() const` | the index of this material in the Table: |
| `static G4MaterialTable* GetMaterialTable()` | the static Table of Materials: |
| `static std::size_t GetNumberOfMaterials()` | - |
| `static G4Material* GetMaterial(const G4String& name, G4bool warning = true)` | return  pointer to a material, given its name: |
| `static G4Material* GetMaterial(G4double z, G4double a, G4double dens)` | return  pointer to a simple material, given its propeties: |
| `static G4Material* GetMaterial(std::size_t nComp, G4double dens)` | return  pointer to a composit material, given its propeties: |
| `friend std::ostream& operator<<(std::ostream&, const G4Material*)` | printing methods |
| `friend std::ostream& operator<<(std::ostream&, const G4Material&)` | - |
| `friend std::ostream& operator<<(std::ostream&, const G4MaterialTable&)` | - |
| `inline void SetName(const G4String& name)` | - |
| `virtual G4bool IsExtended() const` | - |
| `G4bool operator==(const G4Material&) const = delete` | operators |
| `G4bool operator!=(const G4Material&) const = delete` | - |


## Header: `G4Element.hh` (Target: `G4Element`)

**Kind**: C++ Class (`G4Element`) | **Total Extracted Public Methods**: 36

| C++ Method Signature | Comment / Description |
|---|---|
| `public: // with description G4Element(const G4String& name, // its name const G4String& symbol, // its symbol G4double Zeff, // atomic number G4double Aeff); // mass of mole` | Constructor to Build an element directly; no reference to isotopes |
| `G4Element(const G4String& name, // its name const G4String& symbol, // its symbol G4int nbIsotopes); // nb of isotopes` | Constructor to Build an element from isotopes via AddIsotope |
| `virtual ~G4Element()` | - |
| `G4Element(G4Element&) = delete` | - |
| `const G4Element& operator=(const G4Element&) = delete` | - |
| `void AddIsotope(G4Isotope* isotope, // isotope G4double RelativeAbundance); // fraction of nb of` | Add an isotope to the element |
| `inline const G4String& GetName() const` | atomes per volume Retrieval methods |
| `inline const G4String& GetSymbol() const` | - |
| `inline G4double GetZ() const` | Atomic number |
| `inline G4int GetZasInt() const` | - |
| `inline G4double GetN() const` | Atomic weight in atomic units |
| `inline G4double GetAtomicMassAmu() const` | - |
| `inline G4double GetA() const` | Mass of a mole in Geant4 units for atoms with atomic shell |
| `inline G4bool GetNaturalAbundanceFlag() const` | - |
| `inline void SetNaturalAbundanceFlag(G4bool)` | - |
| `inline G4int GetNbOfAtomicShells() const` | the number of atomic shells in this element: |
| `G4double GetAtomicShell(G4int index) const` | the binding energy of the shell, ground shell index=0 |
| `G4int GetNbOfShellElectrons(G4int index) const` | the number of electrons at the shell, ground shell index=0 |
| `inline size_t GetNumberOfIsotopes() const` | number of isotopes constituing this element: |
| `inline G4IsotopeVector* GetIsotopeVector() const` | vector of pointers to isotopes constituing this element: |
| `inline G4double* GetRelativeAbundanceVector() const` | vector of relative abundance of each isotope: |
| `inline const G4Isotope* GetIsotope(G4int iso) const` | - |
| `static G4ElementTable* GetElementTable()` | the (static) Table of Elements: |
| `static size_t GetNumberOfElements()` | - |
| `inline size_t GetIndex() const` | the index of this element in the Table: |
| `static G4Element* GetElement(const G4String& name, G4bool warning = true)` | return pointer to an element, given its name: |
| `inline G4double GetfCoulomb() const` | Coulomb correction factor: |
| `inline G4double GetfRadTsai() const` | Tsai formula for the radiation length: |
| `inline G4IonisParamElm* GetIonisation() const` | pointer to ionisation parameters: |
| `friend std::ostream& operator<<(std::ostream&, const G4Element*)` | printing methods |
| `friend std::ostream& operator<<(std::ostream&, const G4Element&)` | - |
| `friend std::ostream& operator<<(std::ostream&, const G4ElementTable&)` | - |
| `friend std::ostream& operator<<(std::ostream&, const G4ElementVector&)` | - |
| `inline void SetName(const G4String& name)` | - |
| `G4bool operator==(const G4Element&) const = delete` | - |
| `G4bool operator!=(const G4Element&) const = delete` | - |


## Header: `G4Isotope.hh` (Target: `G4Isotope`)

**Kind**: C++ Class (`G4Isotope`) | **Total Extracted Public Methods**: 19

| C++ Method Signature | Comment / Description |
|---|---|
| `public: // with description G4Isotope(const G4String& name, // its name G4int z, // atomic number G4int n, // number of nucleons G4double a = 0., // mass of mole G4int mlevel = 0); // isomer level` | Make an isotope |
| `~G4Isotope()` | - |
| `G4Isotope(const G4Isotope&) = delete` | - |
| `G4Isotope& operator=(const G4Isotope&) = delete` | - |
| `const G4String& GetName() const` | Retrieval methods |
| `G4int GetZ() const` | Atomic number |
| `G4int GetN() const` | Number of nucleous |
| `G4double GetA() const` | Atomic mass of mole in Geant4 units with electron shell |
| `G4int Getm() const` | Isomer level |
| `static G4Isotope* GetIsotope(const G4String& name, G4bool warning = false)` | - |
| `static const G4IsotopeTable* GetIsotopeTable()` | - |
| `static size_t GetNumberOfIsotopes()` | - |
| `size_t GetIndex() const` | - |
| `friend std::ostream& operator<<(std::ostream&, const G4Isotope*)` | - |
| `friend std::ostream& operator<<(std::ostream&, const G4Isotope&)` | - |
| `friend std::ostream& operator<<(std::ostream&, const G4IsotopeTable&)` | - |
| `G4bool operator==(const G4Isotope&) const` | - |
| `G4bool operator!=(const G4Isotope&) const` | - |
| `void SetName(const G4String& name)` | - |


## Header: `G4NistManager.hh` (Target: `G4NistManager`)

**Kind**: C++ Class (`G4NistManager`) | **Total Extracted Public Methods**: 46

| C++ Method Signature | Comment / Description |
|---|---|
| `static G4NistManager* Instance()` | - |
| `~G4NistManager()` | - |
| `inline G4Element* GetElement(size_t index) const` | Get G4Element by index |
| `inline G4Element* FindElement(G4int Z) const` | Find or build G4Element by atomic number |
| `inline G4Element* FindOrBuildElement(G4int Z, G4bool isotopes = true)` | - |
| `inline G4Element* FindOrBuildElement(const G4String& symb, G4bool isotopes = true)` | Find or build G4Element by symbol |
| `inline size_t GetNumberOfElements() const` | Get number of elements |
| `inline G4int GetZ(const G4String& symb) const` | Get atomic number by element symbol |
| `inline G4double GetAtomicMassAmu(const G4String& symb) const` | Get atomic weight by element symbol - mean mass in units of amu of an atom with electron shell for the natural isotope composition |
| `inline G4double GetAtomicMassAmu(G4int Z) const` | Get atomic weight in atomic units - mean mass in units of amu of an atom with electron shell for the natural isotope composition |
| `inline G4double GetIsotopeMass(G4int Z, G4int N) const` | Get mass of isotope without electron shell in Geant4 energy units |
| `inline G4double GetAtomicMass(G4int Z, G4int N) const` | Get mass in Geant4 energy units of an atom of a particular isotope with the electron shell |
| `inline G4double GetTotalElectronBindingEnergy(G4int Z) const` | Get total ionisation energy of an atom |
| `inline G4int GetNistFirstIsotopeN(G4int Z) const` | Get N for the first natural isotope |
| `inline G4int GetNumberOfNistIsotopes(G4int Z) const` | Get number of natural isotopes |
| `inline G4double GetIsotopeAbundance(G4int Z, G4int N) const` | Get natural isotope abundance |
| `inline void PrintElement(G4int Z) const` | Print element by Z |
| `void PrintElement(const G4String&) const` | Print element from internal DB by symbol, if "all" - print all elements |
| `void PrintG4Element(const G4String&) const` | Print G4Element by name, if "all" - print all G4Elements |
| `inline const std::vector<G4String>& GetNistElementNames() const` | Access to the vector of Geant4 predefined element names |
| `inline G4double GetMeanIonisationEnergy(G4int Z) const` | Access mean ionisation energy for atoms (Z <= 98) by its index |
| `inline G4double GetNominalDensity(G4int Z) const` | Access nominal density by atomic number for simple materials and by the index for other NIST materials |
| `inline G4Material* GetMaterial(size_t index) const` | Get G4Material by index |
| `inline G4Material* FindMaterial(const G4String& name) const` | Find or build a G4Material by name, from the Geant4 dataBase |
| `inline G4Material* FindOrBuildMaterial( const G4String& name, G4bool isotopes = true, G4bool warning = false)` | - |
| `inline G4Material* FindSimpleMaterial(G4int Z) const` | Find or build a simple material via atomic number |
| `inline G4Material* FindOrBuildSimpleMaterial(G4int Z, G4bool warning = false)` | - |
| `G4Material* BuildMaterialWithNewDensity(const G4String& name, const G4String& basename, G4double density = 0.0, G4double temp = NTP_Temperature, G4double pres = CLHEP::STP_Pressure)` | Build G4Material with user defined name and density on base of a material from Geant4 dataBase |
| `inline G4Material* ConstructNewMaterial(const G4String& name, const std::vector<G4String>& elm, const std::vector<G4int>& nbAtoms, G4double dens, G4bool isotopes = true, G4State state = kStateSolid, G4double temp = NTP_Temperature, G4double pressure = CLHEP::STP_Pressure)` | Construct a G4Material from scratch by atome count temperature and pressure should be consistent with the density |
| `inline G4Material* ConstructNewMaterial(const G4String& name, const std::vector<G4String>& elm, const std::vector<G4double>& weight, G4double dens, G4bool isotopes = true, G4State state = kStateSolid, G4double temp = NTP_Temperature, G4double pressure = CLHEP::STP_Pressure)` | Construct a G4Material from scratch by fraction mass temperature and pressure should be consistent with the density |
| `inline G4Material* ConstructNewGasMaterial(const G4String& name, const G4String& nameNist, G4double temp, G4double pres, G4bool isotopes = true)` | Construct a gas G4Material from scratch by atome count |
| `inline G4Material* ConstructNewIdealGasMaterial(const G4String& name, const std::vector<G4String>& elm, const std::vector<G4int>& nbAtoms, G4bool isotopes = true, G4double temp = NTP_Temperature, G4double pressure = CLHEP::STP_Pressure)` | Construct an ideal gas G4Material from scratch by atom count |
| `void SetDensityEffectCalculatorFlag(const G4String&, G4bool)` | enable/disable density effect calculator by material name |
| `void SetDensityEffectCalculatorFlag(G4Material*, G4bool)` | enable/disable density effect calculator by material pointer |
| `inline size_t GetNumberOfMaterials() const` | Get number of G4Materials |
| `inline G4int GetVerbose() const` | - |
| `void SetVerbose(G4int)` | - |
| `void PrintG4Material(const G4String&) const` | Print G4Material by name |
| `inline void ListMaterials(const G4String&) const` | Print predefined Geant4 materials: "simple" - only pure materials in basic state (Z = 1, ..., 98) "compound" - NIST compounds "hep" - HEP materials and compounds "bio" - bio-medical materials and compounds "all" - all |
| `inline const std::vector<G4String>& GetNistMaterialNames() const` | Access to the list of names of Geant4 predefined materials |
| `inline G4double GetZ13(G4double Z) const` | Fast computation of Z^1/3 |
| `inline G4double GetZ13(G4int Z) const` | - |
| `inline G4double GetA27(G4int Z) const` | Fast computation of A^0.27 for natuaral abundances |
| `inline G4double GetLOGZ(G4int Z) const` | Fast computation of log(A) |
| `inline G4double GetLOGAMU(G4int Z) const` | - |
| `G4ICRU90StoppingData* GetICRU90StoppingData()` | - |


## Header: `G4ThreeVector.hh` (Target: `G4ThreeVector`)

**Kind**: Global Utilities / Functions

| Signature | Description |
|---|---|


## Header: `G4RotationMatrix.hh` (Target: `G4RotationMatrix`)

**Kind**: Global Utilities / Functions

| Signature | Description |
|---|---|

