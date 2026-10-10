# Partition 2: Primary Generator & Phase Space Source APIs

> **Partition Scope**: Particle guns, General Particle Source (GPS), phase-space distributions (energy, angular, spatial), particle definitions and random generators.

> **Header Base Path**: `/home/shark/geant4/11.2.2-install/include/Geant4`

---

## Header: `G4VUserPrimaryGeneratorAction.hh` (Target: `G4VUserPrimaryGeneratorAction`)

**Kind**: C++ Class (`G4VUserPrimaryGeneratorAction`) | **Total Extracted Public Methods**: 3

| C++ Method Signature | Comment / Description |
|---|---|
| `G4VUserPrimaryGeneratorAction()` | - |
| `virtual ~G4VUserPrimaryGeneratorAction() = default` | - |
| `virtual void GeneratePrimaries(G4Event* anEvent) = 0` | - |


## Header: `G4ParticleGun.hh` (Target: `G4ParticleGun`)

**Kind**: C++ Class (`G4ParticleGun`) | **Total Extracted Public Methods**: 26

| C++ Method Signature | Comment / Description |
|---|---|
| `G4ParticleGun()` | - |
| `explicit G4ParticleGun(G4int numberofparticles)` | - |
| `explicit G4ParticleGun(G4ParticleDefinition* particleDef, G4int numberofparticles = 1)` | - |
| `~G4ParticleGun() override` | Costructors. "numberofparticles" is the number of particles to be shot at one invokation of GeneratePrimaryVertex() method. All particles are shot with the same physical quantities. |
| `G4ParticleGun(const G4ParticleGun&) = delete` | - |
| `const G4ParticleGun& operator=(const G4ParticleGun&) = delete` | - |
| `G4bool operator==(const G4ParticleGun&) const = delete` | - |
| `G4bool operator!=(const G4ParticleGun&) const = delete` | - |
| `void GeneratePrimaryVertex(G4Event* evt) override` | - |
| `void SetParticleDefinition(G4ParticleDefinition* aParticleDefinition)` | Creates a primary vertex at the given point and put primary particles to it. Followings are the Set methods for the particle properties. SetParticleDefinition() should be called first. By using SetParticleMomentum(), both particle_momentum_direction and particle_energy(Kinetic Energy) are set. |
| `void SetParticleEnergy(G4double aKineticEnergy)` | - |
| `void SetParticleMomentum(G4double aMomentum)` | - |
| `void SetParticleMomentum(G4ParticleMomentum aMomentum)` | - |
| `inline void SetParticleMomentumDirection(G4ParticleMomentum aMomDirection)` | - |
| `inline void SetParticleCharge(G4double aCharge)` | - |
| `inline void SetParticlePolarization(G4ThreeVector aVal)` | - |
| `inline void SetNumberOfParticles(G4int i)` | - |
| `inline void SetParticleWeight(G4double w)` | - |
| `inline G4ParticleDefinition* GetParticleDefinition() const` | - |
| `inline G4ParticleMomentum GetParticleMomentumDirection() const` | - |
| `inline G4double GetParticleEnergy() const` | - |
| `inline G4double GetParticleMomentum() const` | - |
| `inline G4double GetParticleCharge() const` | - |
| `inline G4ThreeVector GetParticlePolarization() const` | - |
| `inline G4int GetNumberOfParticles() const` | - |
| `inline G4double GetParticleWeight() const` | - |


## Header: `G4GeneralParticleSource.hh` (Target: `G4GeneralParticleSource`)

**Kind**: C++ Class (`G4GeneralParticleSource`) | **Total Extracted Public Methods**: 28

| C++ Method Signature | Comment / Description |
|---|---|
| `G4GeneralParticleSource()` | - |
| `~G4GeneralParticleSource() override` | Initialize variables and instantiates the messenger and generator classes |
| `void GeneratePrimaryVertex(G4Event*) override` | Delete messenger and others |
| `inline G4int GetNumberofSource()` | - |
| `void ListSource()` | Return the number of particle gun defined |
| `void SetCurrentSourceto(G4int)` | List the particle guns defined |
| `void SetCurrentSourceIntensity(G4double)` | Set the current gun to the specified one so its definition can be changed |
| `inline G4SingleParticleSource* GetCurrentSource() const` | Change the current particle gun strength |
| `inline G4int GetCurrentSourceIndex() const` | Return the pointer to current particle gun |
| `inline G4double GetCurrentSourceIntensity() const` | Return the index of the current particle gun |
| `void ClearAll()` | Return the strength of the current gun |
| `void AddaSource (G4double)` | Remove all defined particle gun |
| `void DeleteaSource(G4int)` | Add a new particle gun with the specified strength |
| `inline void SetVerbosity(G4int i)` | Delete the specified particle gun |
| `inline void SetMultipleVertex(G4bool av)` | Set the verbosity level. |
| `inline void SetFlatSampling(G4bool av)` | Set if multiple vertex per event. |
| `inline void SetParticleDefinition (G4ParticleDefinition * aPDef)` | Set if flat_sampling is applied in multiple source case |
| `inline G4ParticleDefinition* GetParticleDefinition () const` | - |
| `inline void SetParticleCharge(G4double aCharge)` | Set/Get the particle definition of the primary track |
| `inline void SetParticlePolarization (G4ThreeVector aVal)` | Set the charge state of the primary track |
| `inline G4ThreeVector GetParticlePolarization () const` | - |
| `inline void SetParticleTime(G4double aTime)` | Set/Get polarization state of the primary track |
| `inline G4double GetParticleTime() const` | - |
| `inline void SetNumberOfParticles(G4int i)` | Set/Get the Time. |
| `inline G4int GetNumberOfParticles() const` | - |
| `inline G4ThreeVector GetParticlePosition() const` | Set/Get the number of particles to be generated in the primary track |
| `inline G4ThreeVector GetParticleMomentumDirection() const` | - |
| `inline G4double GetParticleEnergy() const` | - |


## Header: `G4SingleParticleSource.hh` (Target: `G4SingleParticleSource`)

**Kind**: C++ Class (`G4SingleParticleSource`) | **Total Extracted Public Methods**: 20

| C++ Method Signature | Comment / Description |
|---|---|
| `G4SingleParticleSource()` | - |
| `~G4SingleParticleSource() override` | Constructor: initializes variables and instantiates the messenger and navigator classes |
| `void GeneratePrimaryVertex(G4Event *evt) override` | Destructor: deletes messenger and prints out run information |
| `inline G4SPSPosDistribution* GetPosDist() const` | Generate the particles initial parameters |
| `inline G4SPSAngDistribution* GetAngDist() const` | Return a pointer to the position distribution generator |
| `inline G4SPSEneDistribution* GetEneDist() const` | Return a pointer to the angular distribution generator |
| `inline G4SPSRandomGenerator* GetBiasRndm() const` | Return a pointer to the energy distribution generator |
| `void SetVerbosity(G4int)` | Return a pointer to the biased random number generator |
| `void SetParticleDefinition(G4ParticleDefinition* aParticleDefinition)` | Set the verbosity level |
| `inline G4ParticleDefinition* GetParticleDefinition() const` | - |
| `inline void SetParticleCharge(G4double aCharge)` | Get/Set the particle definition of the primary track |
| `inline void SetParticlePolarization(const G4ThreeVector& aVal)` | Set the charge state of the primary track |
| `inline const G4ThreeVector& GetParticlePolarization() const` | - |
| `inline void SetParticleTime(G4double aTime)` | Set/Get the polarization state of the primary track |
| `inline G4double GetParticleTime() const` | - |
| `inline void SetNumberOfParticles(G4int i)` | Set/Get the Time |
| `inline G4int GetNumberOfParticles() const` | - |
| `inline G4ThreeVector GetParticlePosition() const` | Set/get the number of particles to be generated in the primary track |
| `inline G4ThreeVector GetParticleMomentumDirection() const` | - |
| `inline G4double GetParticleEnergy() const` | - |


## Header: `G4SPSPosDistribution.hh` (Target: `G4SPSPosDistribution`)

**Kind**: C++ Class (`G4SPSPosDistribution`) | **Total Extracted Public Methods**: 43

| C++ Method Signature | Comment / Description |
|---|---|
| `G4SPSPosDistribution()` | - |
| `~G4SPSPosDistribution()` | Constructor: initializes data and instantiates the Navigator class |
| `void SetPosDisType(const G4String&)` | Destructor Methods to create source position dist |
| `void SetPosDisShape(const G4String&)` | Allows user to choose Point, Plane, Surface or Volume source position distributions |
| `void SetCentreCoords(const G4ThreeVector&)` | Allows the user to choose the particular shape they wish for the position distribution. Choices are: Square, Circle, Ellipse, Rectangle, Sphere, Ellipsoid, Cylinder, Parallelepiped |
| `void SetPosRot1(const G4ThreeVector&)` | Sets the coordinates of the centre of the position distribution |
| `void SetPosRot2(const G4ThreeVector&)` | Used to specify the coordinate system for the position distribution along with SetPosRot2. Sets the vector x' and need not be a unit vector |
| `void SetHalfX(G4double)` | Used in connection with SetPosRot1. This sets a vector in the plane x'y'. By a series of cross products x', y', z' are generated. Again need not be a unit vector |
| `void SetHalfY(G4double)` | Sets the half length in x |
| `void SetHalfZ(G4double)` | Sets the half length in y |
| `void SetRadius(G4double)` | Sets the half length in z |
| `void SetRadius0(G4double)` | Sets the radius where appropriate for source distribution shapes |
| `void SetBeamSigmaInR(G4double)` | Sets the inner radius where appropriate for source distribution shapes |
| `void SetBeamSigmaInX(G4double)` | Sets the sigma for 1D beam |
| `void SetBeamSigmaInY(G4double)` | Sets the first sigma for 2D beam |
| `void SetParAlpha(G4double)` | Sets the second sigma for 2D beam |
| `void SetParTheta(G4double)` | Sets the angle Alpha in the Parallelepiped shapes |
| `void SetParPhi(G4double)` | Sets the angle Theta in the Parallelepiped shapes |
| `void ConfineSourceToVolume(const G4String&)` | Sets the angle Phi in the Parallelepiped shapes |
| `void SetBiasRndm (G4SPSRandomGenerator* a)` | Used to confine the start positions to a particular volume |
| `void SetVerbosity(G4int a)` | Sets the biased random number generator |
| `G4ThreeVector GenerateOne()` | Sets the verbosity level |
| `const G4String& GetPosDisType() const` | Generate one random position |
| `const G4String& GetPosDisShape() const` | - |
| `const G4ThreeVector& GetCentreCoords() const` | - |
| `G4double GetHalfX() const` | - |
| `G4double GetHalfY() const` | - |
| `G4double GetHalfZ() const` | - |
| `G4double GetRadius() const` | - |
| `inline G4double GetRadius0() const` | - |
| `inline G4double GetParAlpha() const` | - |
| `inline G4double GetParTheta() const` | - |
| `inline G4double GetParPhi() const` | - |
| `inline const G4ThreeVector& GetRotx() const` | - |
| `inline const G4ThreeVector& GetRoty() const` | - |
| `inline const G4ThreeVector& GetRotz() const` | - |
| `inline G4bool GetConfined() const` | - |
| `inline const G4String& GetConfineVolume() const` | - |
| `const G4ThreeVector& GetSideRefVec1() const` | - |
| `const G4ThreeVector& GetSideRefVec2() const` | - |
| `const G4ThreeVector& GetSideRefVec3() const` | - |
| `const G4String& GetSourcePosType() const` | - |
| `const G4ThreeVector& GetParticlePos() const` | - |


## Header: `G4SPSAngDistribution.hh` (Target: `G4SPSAngDistribution`)

**Kind**: C++ Class (`G4SPSAngDistribution`) | **Total Extracted Public Methods**: 28

| C++ Method Signature | Comment / Description |
|---|---|
| `G4SPSAngDistribution ()` | - |
| `~G4SPSAngDistribution ()` | Constructor: Initializes variables |
| `void SetAngDistType(const G4String&)` | Destructor Angular Distribution Methods |
| `void DefineAngRefAxes(const G4String&, const G4ThreeVector&)` | Used to set the type of angular distribution wanted. Arguments are iso, cos, beam and user for isotropic, cosine-law, beam and user-defined respectively. |
| `void SetMinTheta(G4double)` | Used in a similar way as SetPosRot() to define vectors, one x' and one in the plane x'y', to create a rotated set of axes for the angular distribution. |
| `void SetMinPhi(G4double)` | Sets the minimum value for the angle theta. |
| `void SetMaxTheta(G4double)` | Sets the minimum value for phi. |
| `void SetMaxPhi(G4double)` | Sets the maximum value for theta. |
| `void SetBeamSigmaInAngR(G4double)` | Sets the maximum value for phi. |
| `void SetBeamSigmaInAngX(G4double)` | Sets the sigma for 1D beam. |
| `void SetBeamSigmaInAngY(G4double)` | Sets the first sigma for 2D beam. |
| `void UserDefAngTheta(const G4ThreeVector&)` | Sets the second sigma for 2D beam. |
| `void UserDefAngPhi(const G4ThreeVector&)` | This method allows the user to define a histogram in Theta. |
| `void SetFocusPoint(const G4ThreeVector&)` | This method allows the user to define a histogram in phi. |
| `void SetParticleMomentumDirection(const G4ParticleMomentum& aMomDirection)` | - |
| `void SetUseUserAngAxis(G4bool)` | - |
| `void SetUserWRTSurface(G4bool)` | - |
| `void SetPosDistribution(G4SPSPosDistribution* a)` | Allows user to have user-defined spectra either with respect to the coordinate system (default) or with respect to the surface normal. |
| `void SetBiasRndm(G4SPSRandomGenerator* a)` | Sets the required position generator, required for determining the cosine-law distribution. |
| `G4ParticleMomentum GenerateOne()` | Sets the biased random number generator. |
| `void ReSetHist(const G4String&)` | Generates one random direction. |
| `void SetVerbosity(G4int a)` | Re-sets the histogram for user defined distribution.. |
| `G4String GetDistType()` | Sets the verbosity level. Some accessors |
| `G4double GetMinTheta()` | - |
| `G4double GetMaxTheta()` | - |
| `G4double GetMinPhi()` | - |
| `G4double GetMaxPhi()` | - |
| `G4ThreeVector GetDirection()` | - |


## Header: `G4SPSEneDistribution.hh` (Target: `G4SPSEneDistribution`)

**Kind**: C++ Class (`G4SPSEneDistribution`) | **Total Extracted Public Methods**: 45

| C++ Method Signature | Comment / Description |
|---|---|
| `G4SPSEneDistribution()` | - |
| `~G4SPSEneDistribution()` | Constructor: initializes variables |
| `void SetEnergyDisType(const G4String&)` | Destructor |
| `const G4String& GetEnergyDisType()` | Allows the user to choose the energy distribution type. The arguments are: Mono (mono-energetic), Lin (linear), Pow (power-law), Exp (exponential), Gauss (gaussian), Brem (bremsstrahlung), BBody (black-body), Cdg (cosmic diffuse gamma-ray), User (user-defined), Arb (arbitrary point-wise), Epn (energy per nucleon) |
| `void SetEmin(G4double)` | - |
| `G4double GetEmin() const` | Sets the minimum energy |
| `G4double GetArbEmin()` | - |
| `void SetEmax(G4double)` | - |
| `G4double GetEmax() const` | Sets the maximum energy |
| `G4double GetArbEmax()` | - |
| `void SetMonoEnergy(G4double)` | - |
| `void SetAlpha(G4double)` | Sets energy for mono-energetic distribution |
| `void SetBiasAlpha(G4double)` | Sets alpha for a power-law distribution |
| `void SetTemp(G4double)` | - |
| `void SetBeamSigmaInE(G4double)` | Sets Temperature for a Brem or BBody distributions |
| `void SetEzero(G4double)` | - |
| `void SetGradient(G4double)` | Sets Ezero for an exponential distribution |
| `void SetInterCept(G4double)` | Sets gradient for a linear distribution |
| `void UserEnergyHisto(const G4ThreeVector&)` | Sets intercept for a linear distribution |
| `void ArbEnergyHisto(const G4ThreeVector&)` | Allows user to defined a histogram for the energy distribution |
| `void ArbEnergyHistoFile(const G4String&)` | Allows the user to define an Arbitrary set of points for the energy distribution |
| `void EpnEnergyHisto(const G4ThreeVector&)` | - |
| `void InputEnergySpectra(G4bool)` | Allows the user to define an Energy per nucleon histogram |
| `void InputDifferentialSpectra(G4bool)` | Allows the user to choose between momentum and energy histograms for user-defined histograms and arbitrary point-wise spectra. The default is true (energy) |
| `void ArbInterpolate(const G4String&)` | Allows the user to choose between integral and differential distributions when using the arbitrary point-wise option |
| `const G4String& GetIntType()` | Allows the user to specify the type of function to interpolate the Arbitrary points spectrum with |
| `void Calculate()` | - |
| `void SetBiasRndm(G4SPSRandomGenerator* a)` | Controls the calculation of Integral PDF for the Cdg and BBody distributions |
| `void ReSetHist(const G4String&)` | Sets the biased random number generator |
| `void SetVerbosity(G4int a)` | Resets the histogram for user defined distribution |
| `G4double GetWeight() const` | Sets the verbosity level |
| `G4double GetMonoEnergy()` | - |
| `G4double GetSE()` | Mono-energetic energy |
| `G4double Getalpha() const` | Standard deviation for Gaussian distribution in energy |
| `G4double GetEzero() const` | Alpha (pow) |
| `G4double GetTemp()` | E0 (exp) |
| `G4double Getgrad() const` | Temp (bbody,brem) |
| `G4double Getcept() const` | Gradient and intercept for linear spectra |
| `G4PhysicsFreeVector GetUserDefinedEnergyHisto()` | - |
| `G4PhysicsFreeVector GetArbEnergyHisto()` | - |
| `G4double GenerateOne(G4ParticleDefinition*)` | - |
| `G4double GetProbability (G4double)` | Generate one random energy for the specified particle |
| `G4double GetArbEneWeight(G4double)` | - |
| `inline void ApplyEnergyWeight(G4bool val)` | - |
| `inline G4bool IfApplyEnergyWeight() const` | - |


## Header: `G4ParticleTable.hh` (Target: `G4ParticleTable`)

**Kind**: C++ Class (`G4ParticleTable`) | **Total Extracted Public Methods**: 38

| C++ Method Signature | Comment / Description |
|---|---|
| `virtual ~G4ParticleTable()` | - |
| `G4ParticleTable(const G4ParticleTable&) = delete` | Copy constructor and assignment operator not allowed |
| `G4ParticleTable& operator=(const G4ParticleTable&) = delete` | - |
| `void WorkerG4ParticleTable()` | This method is similar to the constructor. It is used by each worker thread to achieve the partial effect as that of the master thread |
| `void DestroyWorkerG4ParticleTable()` | This method is similar to the destructor. It is used by each worker thread to achieve the partial effect as that of the master thread |
| `static G4ParticleTable* GetParticleTable()` | Return the pointer to the G4ParticleTable object G4ParticleTable is a "singleton" and can get its pointer by this function. At the first time of calling this function, the G4ParticleTable object is instantiated |
| `inline G4bool contains(const G4ParticleDefinition* particle) const` | Returns TRUE if the ParticleTable contains the particle's pointer |
| `G4bool contains(const G4String& particle_name) const` | - |
| `G4int entries() const` | Returns the number of particles in the ParticleTable |
| `G4int size() const` | - |
| `G4ParticleDefinition* GetParticle(G4int index) const` | Returns a pointer to the i-th particle in the ParticleTable 0 <= index < entries() |
| `const G4String& GetParticleName(G4int index) const` | Returns the name of i-th particle in the ParticleTable |
| `G4ParticleDefinition* FindParticle(G4int PDGEncoding)` | Returns a pointer to the particle (0 if not contained) |
| `G4ParticleDefinition* FindParticle(const G4String& particle_name)` | - |
| `G4ParticleDefinition* FindParticle(const G4ParticleDefinition* particle)` | - |
| `inline G4ParticleDefinition* FindAntiParticle(G4int PDGEncoding)` | Returns a pointer to its anti-particle (0 if not contained) |
| `inline G4ParticleDefinition* FindAntiParticle(const G4String& p_name)` | - |
| `inline G4ParticleDefinition* FindAntiParticle(const G4ParticleDefinition* p)` | - |
| `G4PTblDicIterator* GetIterator() const` | Returns the pointer to the Iterator |
| `void DumpTable(const G4String& particle_name = "ALL")` | Dumps information of particles specified by name |
| `G4IonTable* GetIonTable() const` | Returns the pointer to the G4IonTable object |
| `G4ParticleDefinition* Insert(G4ParticleDefinition* particle)` | Inserts the particle into ParticleTable. Returned value is the same as particle if successfully inserted or the pointer to another G4ParticleDefinition object which has same particle name or nullptr if failing to insert by other reason |
| `G4ParticleDefinition* Remove(G4ParticleDefinition* particle)` | Removes the particle from the table (not delete) |
| `void RemoveAllParticles()` | Removes all particles from G4ParticleTable |
| `void DeleteAllParticles()` | Removes and deletes all particles from G4ParticleTable |
| `G4UImessenger* CreateMessenger()` | Creates messenger |
| `void SelectParticle(const G4String& name)` | - |
| `inline const G4ParticleDefinition* GetSelectedParticle() const` | - |
| `inline void SetVerboseLevel(G4int value)` | - |
| `inline G4int GetVerboseLevel() const` | - |
| `inline void SetReadiness(G4bool val = true)` | - |
| `inline G4bool GetReadiness() const` | - |
| `inline G4ParticleDefinition* GetGenericIon() const` | - |
| `inline void SetGenericIon(G4ParticleDefinition*)` | - |
| `inline G4ParticleDefinition* GetGenericMuonicAtom() const` | - |
| `inline void SetGenericMuonicAtom(G4ParticleDefinition*)` | - |
| `static G4GLOB_DLL G4Mutex& particleTableMutex()` | Shared instance of a mutex |
| `static G4GLOB_DLL G4int& lockCount()` | - |


## Header: `G4ParticleDefinition.hh` (Target: `G4ParticleDefinition`)

**Kind**: C++ Class (`G4ParticleDefinition`) | **Total Extracted Public Methods**: 64

| C++ Method Signature | Comment / Description |
|---|---|
| `G4ParticleDefinition(const G4String& aName, G4double mass, G4double width, G4double charge, G4int iSpin, G4int iParity, G4int iConjugation, G4int iIsospin, G4int iIsospinZ, G4int gParity, const G4String& pType, G4int lepton, G4int baryon, G4int encoding, G4bool stable, G4double lifetime, G4DecayTable* decaytable, G4bool shortlived = false, const G4String& subType = "", G4int anti_encoding = 0, G4double magneticMoment = 0.0)` | Only one type of constructor can be used for G4ParticleDefinition. If you want to create new particle, you must set name of the particle at construction. Most of members seen as arguments of the constructor (except last 3 arguments concerning with decay ) are  "constant" and can not be changed later. (No "SET" methods are available) Each type of particle must be constructed as a unique object of special class derived from G4ParticleDefinition. See G4ParticleTypes for detail clang-format off |
| `virtual ~G4ParticleDefinition()` | clang-format on |
| `G4ParticleDefinition(const G4ParticleDefinition&) = delete` | Can not use "copy constructor", equality nor "default constructor"! |
| `G4ParticleDefinition& operator=(const G4ParticleDefinition&) = delete` | - |
| `G4bool operator==(const G4ParticleDefinition& right) const` | - |
| `G4bool operator!=(const G4ParticleDefinition& right) const` | - |
| `const G4String& GetParticleName() const` | With the following Getxxxx methods, one can get values for members which can not be changed |
| `G4double GetPDGMass() const` | - |
| `G4double GetPDGWidth() const` | - |
| `G4double GetPDGCharge() const` | - |
| `G4double GetPDGSpin() const` | - |
| `G4int GetPDGiSpin() const` | - |
| `G4int GetPDGiParity() const` | - |
| `G4int GetPDGiConjugation() const` | - |
| `G4double GetPDGIsospin() const` | - |
| `G4double GetPDGIsospin3() const` | - |
| `G4int GetPDGiIsospin() const` | - |
| `G4int GetPDGiIsospin3() const` | - |
| `G4int GetPDGiGParity() const` | - |
| `G4double GetPDGMagneticMoment() const` | - |
| `inline void SetPDGMagneticMoment(G4double mageticMoment)` | - |
| `G4double CalculateAnomaly() const` | Gives the anomaly of magnetic moment for spin 1/2 particles |
| `const G4String& GetParticleType() const` | - |
| `const G4String& GetParticleSubType() const` | - |
| `G4int GetLeptonNumber() const` | - |
| `G4int GetBaryonNumber() const` | - |
| `G4int GetPDGEncoding() const` | - |
| `G4int GetAntiPDGEncoding() const` | - |
| `inline void SetAntiPDGEncoding(G4int aEncoding)` | - |
| `inline G4int GetQuarkContent(G4int flavor) const` | Returns the number of quark with flavor contained in this particle. The value of flavor is assigned as follows 1:d, 2:u, 3:s, 4:c, 5:b, 6:t |
| `inline G4int GetAntiQuarkContent(G4int flavor) const` | - |
| `G4bool IsShortLived() const` | - |
| `inline G4bool GetPDGStable() const` | - |
| `void SetPDGStable(const G4bool aFlag)` | - |
| `inline G4double GetPDGLifeTime() const` | - |
| `void SetPDGLifeTime(G4double aLifeTime)` | - |
| `inline G4double GetIonLifeTime() const` | Get life time of a generic ion through G4NuclideTable. |
| `inline G4DecayTable* GetDecayTable() const` | Set/Get Decay Table !! Decay Table can be modified !! |
| `inline void SetDecayTable(G4DecayTable* aDecayTable)` | - |
| `G4ProcessManager* GetProcessManager() const` | Set/Get Process Manager !! Process Manager can be modified !! |
| `void SetProcessManager(G4ProcessManager* aProcessManager)` | - |
| `G4VTrackingManager* GetTrackingManager() const` | Set/Get Tracking Manager; nullptr means the default !! Tracking Manager can be modified !! |
| `void SetTrackingManager(G4VTrackingManager* aTrackingManager)` | - |
| `inline G4ParticleTable* GetParticleTable() const` | Get pointer to the particle table |
| `inline G4int GetAtomicNumber() const` | Get AtomicNumber and AtomicMass These properties are defined for nucleus |
| `inline G4int GetAtomicMass() const` | - |
| `void DumpTable() const` | Prints information of data members. |
| `inline void SetVerboseLevel(G4int value)` | Control flag for output message 0: Silent 1: Warning message 2: More |
| `inline G4int GetVerboseLevel() const` | - |
| `void SetApplyCutsFlag(G4bool)` | - |
| `inline G4bool GetApplyCutsFlag() const` | - |
| `inline G4bool IsGeneralIon() const` | True only if the particle is G4Ions (it means that theProcessManager is same as one for G4GenricIon) |
| `inline G4bool IsMuonicAtom() const` | True only if the particle is a G4MuonicAtom (it means that theProcessManager is same as the one for G4MuonicAtom) |
| `inline G4ProcessManager* GetMasterProcessManager() const` | Returns the process manager master pointer. |
| `inline void SetMasterProcessManager(G4ProcessManager* aNewPM)` | Sets the shadow master pointer (not to be used by user) |
| `inline G4int GetInstanceID() const` | Returns the instance ID |
| `static const G4PDefManager& GetSubInstanceManager()` | Returns the private data instance manager |
| `static void Clean()` | Clear memory allocated by sub-instance manager |
| `void SetParticleDefinitionID(G4int id = -1)` | - |
| `inline G4int GetParticleDefinitionID() const` | - |
| `inline G4bool IsHypernucleus() const` | The first two methods return "false" and 0, respectively, if the particle is not an hypernucleus; else, they return "true" and the number of Lambdas bound in the nucleus. Similarly, the last two methods return "false" and 0, respectively, if the particle is not an anti-hypernucleus; else, they return "true" and the number of anti-Lambdas bound in the anti-nucleus. Notice that, for the time being, we are assuming that (anti-)Lambda is the only type of (anti-)hyperon present in all (anti-)hypernuclei. |
| `inline G4int GetNumberOfLambdasInHypernucleus() const` | - |
| `inline G4bool IsAntiHypernucleus() const` | - |
| `inline G4int GetNumberOfAntiLambdasInAntiHypernucleus() const` | - |


## Header: `G4IonTable.hh` (Target: `G4IonTable`)

**Kind**: C++ Class (`G4IonTable`) | **Total Extracted Public Methods**: 58

| C++ Method Signature | Comment / Description |
|---|---|
| `G4IonTable()` | Constructor, destructor |
| `~G4IonTable()` | - |
| `G4IonTable(const G4IonTable&) = delete` | Forbidden copy constructor and assignment operator |
| `G4IonTable& operator=(const G4IonTable&) = delete` | - |
| `static G4IonTable* GetIonTable()` | - |
| `void WorkerG4IonTable()` | Method is used by each worker thread to copy the content from the master thread. |
| `void DestroyWorkerG4IonTable()` | Destructor for worker |
| `G4int GetNumberOfElements() const` | Get number of elements defined in the IonTable |
| `void RegisterIsotopeTable(G4VIsotopeTable* table)` | Register Isotope table |
| `G4VIsotopeTable* GetIsotopeTable(std::size_t idx = 0) const` | G4IonTable asks properties of isotopes to G4VIsotopeTable by using FindIsotope(G4IsotopeProperty* property) method |
| `void CreateAllIon()` | All ground state ions are created. Stable ground states are defined in G4NuclearProperty |
| `void CreateAllIsomer()` | All excited ions with long life time (>1.0*ns) are created. Isomers are defined in G4VIsotopeTable |
| `void PrepareNuclideTable()` | All nuclide with a life time longer than certain value are created prior to the event loop |
| `void PreloadNuclide()` | - |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4int lvl = 0)` | -------------------------------------------------------------- FindIon/GetIon FindIon() methods return pointer of ion if it exists. GetIon() methods also return pointer of ion; the designated ion is created if it does not exist.  !! PDGCharge in G4ParticleDefinition of ions is          !! !! electric charge of nucleus (i.e. fully ionized ions)  !! -------------------------------------------------------------- Find/Get "ground state" and "excited state" |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4int nL, G4int lvl)` | - |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4double E, G4int J = 0)` | - |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4double E, G4Ions::G4FloatLevelBase flb, G4int J = 0)` | - |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4double E, char flbChar, G4int J = 0)` | - |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4int nL, G4double E, G4int J = 0)` | - |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4int nL, G4double E, G4Ions::G4FloatLevelBase flb, G4int J = 0)` | - |
| `G4ParticleDefinition* GetIon(G4int Z, G4int A, G4int nL, G4double E, char flbChar, G4int J = 0)` | - |
| `G4ParticleDefinition* GetIon(G4int encoding)` | Z: Atomic Number A: Atomic Mass (nn + np +nlambda) nL: Number of Lambda E: Excitation energy lvl:  Isomer Level 0: ground state) flb:  Floating level base (enum defined in G4Ions.hh) flbChar:  Floating level base denoted by a character (<null>,X,Y,Z,U,V,W,R,S,T,A,B,C,D,E) J: Total Angular momentum (in unit of 1/2) : not used The ion can be retrieved by using PDG encoding !! Only ground state can be obtained .i.e. Isomer = 0 |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4int lvl = 0)` | Find/Get "excited state" |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4int nL, G4int lvl)` | - |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4double E, G4int J = 0)` | - |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4double E, G4Ions::G4FloatLevelBase flb, G4int J = 0)` | - |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4double E, char flbChar, G4int J = 0)` | - |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4int nL, G4double E, G4int J = 0)` | - |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4int nL, G4double E, G4Ions::G4FloatLevelBase flb, G4int J = 0)` | - |
| `G4ParticleDefinition* FindIon(G4int Z, G4int A, G4int nL, G4double E, char flbChar, G4int J = 0)` | - |
| `static G4bool IsIon(const G4ParticleDefinition*)` | Z: Atomic Number A: Atomic Mass (nn + np +nlambda) nL: Number of Lambda E: Excitaion energy lvl:  Isomer Level 0: ground state) flb:  Floating level base (enum defined in G4Ions.hh) flbChar:  Floating level base denoted by a character (<null>,X,Y,Z,U,V,W,R,S,T,A,B,C,D,E) J: Total Angular momentum (in unit of 1/2) : not used Return true if the particle is ion |
| `static G4bool IsAntiIon(const G4ParticleDefinition*)` | Return true if the particle is anti_ion |
| `G4String GetIonName(G4int Z, G4int A, G4int lvl = 0) const` | Get ion name |
| `G4String GetIonName(G4int Z, G4int A, G4double E, G4Ions::G4FloatLevelBase flb = G4Ions::G4FloatLevelBase::no_Float) const` | - |
| `G4String GetIonName(G4int Z, G4int A, G4int nL, G4double E, G4Ions::G4FloatLevelBase flb = G4Ions::G4FloatLevelBase::no_Float) const` | - |
| `G4String GetIonName(G4int Z, G4int A, G4int nL, G4int lvl) const` | - |
| `static G4int GetNucleusEncoding(G4int Z, G4int A, G4double E = 0.0, G4int lvl = 0)` | Get PDG code for Ions. Nuclear codes are given as 10-digit numbers +-100ZZZAAAI. For a nucleus consisting of np protons and nn neutrons A = np + nn and Z = np. I gives the isomer level, with I = 0 corresponding to the ground state and I >0 to excitations |
| `static G4int GetNucleusEncoding(G4int Z, G4int A, G4int nL, G4double E = 0.0, G4int lvl = 0)` | Get PDG code for Hyper-Nucleus Ions. Nuclear codes are given as 10-digit numbers +-10LZZZAAAI. For a nucleus consisting of np protons and nn neutrons A = np + nn +nlambda and Z = np. nL = nlambda I gives the isomer level, with I = 0 corresponding to the ground state and I >0 to excitations |
| `static G4bool GetNucleusByEncoding(G4int encoding, G4int& Z, G4int& A, G4double& E, G4int& lvl)` | - |
| `static G4bool GetNucleusByEncoding(G4int encoding, G4int& Z, G4int& A, G4int& L, G4double& E, G4int& lvl)` | - |
| `G4double GetIonMass(G4int Z, G4int A, G4int nL = 0, G4int lvl = 0) const` | Energy will not be given even for excited state!! These methods returns Nucleus (i.e. full ionized atom) mass, where Z is Atomic Number (number of protons) and A is Atomic Number (number of nucleons and hyperons) nL is number of lambda (A= nn + np + nlambda) lvl is isomer level |
| `G4double GetNucleusMass(G4int Z, G4int A, G4int nL = 0, G4int lvl = 0) const` | - |
| `G4double GetIsomerMass(G4int Z, G4int A, G4int lvl = 0) const` | - |
| `G4double GetLifeTime(const G4ParticleDefinition*) const` | Returns a life time of an ion. -1 for stable ion, and -1001 for ion that is not listed in G4NuclideTable |
| `G4double GetLifeTime(G4int Z, G4int A, G4double E, G4Ions::G4FloatLevelBase flb = G4Ions::G4FloatLevelBase::no_Float) const` | - |
| `G4double GetLifeTime(G4int Z, G4int A, G4double E, char flbChar) const` | - |
| `G4ParticleDefinition* GetMuonicAtom(G4Ions const*)` | - |
| `G4ParticleDefinition* GetMuonicAtom(G4int Z, G4int A)` | - |
| `G4int Entries() const` | Return number of ions in the table |
| `G4ParticleDefinition* GetParticle(G4int index) const` | Return the pointer of index-th ion in the table |
| `G4bool Contains(const G4ParticleDefinition* particle) const` | Return 'true' if the ion exists |
| `void Insert(const G4ParticleDefinition* particle)` | Insert/Remove an ion in the table |
| `void Remove(const G4ParticleDefinition* particle)` | - |
| `void clear()` | Erase all contents in the list (not delete just remove) |
| `G4int size() const` | Return number of ions in the table |
| `void DumpTable(const G4String& particle_name = "ALL") const` | Dump information of particles specified by name |
| `void InitializeLightIons()` | Needed for MT |


## Header: `G4PrimaryVertex.hh` (Target: `G4PrimaryVertex`)

**Kind**: C++ Class (`G4PrimaryVertex`) | **Total Extracted Public Methods**: 28

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PrimaryVertex() = default` | Constructors |
| `G4PrimaryVertex(G4double x0, G4double y0, G4double z0, G4double t0)` | - |
| `G4PrimaryVertex(G4ThreeVector xyz0, G4double t0)` | - |
| `virtual ~G4PrimaryVertex()` | Destructor |
| `G4PrimaryVertex(const G4PrimaryVertex& right)` | Copy constructor & assignment operator |
| `G4PrimaryVertex& operator=(const G4PrimaryVertex& right)` | - |
| `G4bool operator==(const G4PrimaryVertex& right) const` | Equality operators |
| `G4bool operator!=(const G4PrimaryVertex& right) const` | - |
| `inline void* operator new(size_t)` | Overloaded new/delete operators |
| `inline void operator delete(void* aPrimaryVertex)` | - |
| `inline G4ThreeVector GetPosition() const` | Accessors & modifiers |
| `inline void SetPosition(G4double x0, G4double y0, G4double z0)` | - |
| `inline G4double GetX0() const` | - |
| `inline G4double GetY0() const` | - |
| `inline G4double GetZ0() const` | - |
| `inline G4double GetT0() const` | - |
| `inline void SetT0(G4double t0)` | - |
| `inline G4int GetNumberOfParticle() const` | - |
| `inline void SetPrimary(G4PrimaryParticle* pp)` | - |
| `G4PrimaryParticle* GetPrimary(G4int i = 0) const` | - |
| `inline void SetNext(G4PrimaryVertex* nv)` | - |
| `inline void ClearNext()` | - |
| `inline G4PrimaryVertex* GetNext() const` | - |
| `inline G4double GetWeight() const` | - |
| `inline void SetWeight(G4double w)` | - |
| `inline void SetUserInformation(G4VUserPrimaryVertexInformation* info)` | - |
| `inline G4VUserPrimaryVertexInformation* GetUserInformation() const` | - |
| `void Print() const` | - |


## Header: `G4PrimaryParticle.hh` (Target: `G4PrimaryParticle`)

**Kind**: C++ Class (`G4PrimaryParticle`) | **Total Extracted Public Methods**: 57

| C++ Method Signature | Comment / Description |
|---|---|
| `G4PrimaryParticle()` | Constructors |
| `G4PrimaryParticle(G4int Pcode)` | - |
| `G4PrimaryParticle(G4int Pcode, G4double px, G4double py, G4double pz)` | - |
| `G4PrimaryParticle(G4int Pcode, G4double px, G4double py, G4double pz, G4double E)` | - |
| `G4PrimaryParticle(const G4ParticleDefinition* Gcode)` | - |
| `G4PrimaryParticle(const G4ParticleDefinition* Gcode, G4double px, G4double py, G4double pz)` | - |
| `G4PrimaryParticle(const G4ParticleDefinition* Gcode, G4double px, G4double py, G4double pz, G4double E)` | - |
| `virtual ~G4PrimaryParticle()` | Destructor |
| `G4PrimaryParticle(const G4PrimaryParticle& right)` | Copy constructor and assignment operator NOTE: nextParticle and daughterParticle are copied by object (i.e. deep copy); userInfo will not be copied |
| `G4PrimaryParticle& operator=(const G4PrimaryParticle& right)` | - |
| `G4bool operator==(const G4PrimaryParticle& right) const` | Equality operator returns 'true' only if same object (i.e. comparison by pointer value) |
| `G4bool operator!=(const G4PrimaryParticle& right) const` | - |
| `inline void* operator new(std::size_t)` | Overloaded new/delete operators |
| `inline void operator delete(void* aPrimaryParticle)` | - |
| `void Print() const` | Print the properties of the particle |
| `inline G4int GetPDGcode() const` | Followings are the available accessors/modifiers. "trackID" will be set if this particle is sent to G4EventManager and converted to G4Track. Otherwise = -1. The mass and charge in G4ParticleDefinition will be used by default. "SetMass" and "SetCharge" methods are used to set dynamical mass and charge of G4DynamicParticle. "GetMass" and "GetCharge" methods will return those in G4ParticleDefinition if users do not set dynamical ones |
| `void SetPDGcode(G4int Pcode)` | - |
| `inline G4ParticleDefinition* GetG4code() const` | - |
| `inline void SetG4code(const G4ParticleDefinition* Gcode)` | - |
| `inline const G4ParticleDefinition* GetParticleDefinition() const` | - |
| `void SetParticleDefinition(const G4ParticleDefinition* pdef)` | - |
| `inline G4double GetMass() const` | - |
| `inline void SetMass(G4double mas)` | - |
| `inline G4double GetCharge() const` | - |
| `inline void SetCharge(G4double chg)` | - |
| `inline G4double GetKineticEnergy() const` | - |
| `inline void SetKineticEnergy(G4double eKin)` | - |
| `inline const G4ThreeVector& GetMomentumDirection() const` | - |
| `inline void SetMomentumDirection(const G4ThreeVector& p)` | - |
| `inline G4double GetTotalMomentum() const` | - |
| `void Set4Momentum(G4double px, G4double py, G4double pz, G4double E)` | - |
| `inline G4double GetTotalEnergy() const` | - |
| `inline void SetTotalEnergy(G4double eTot)` | - |
| `inline G4ThreeVector GetMomentum() const` | - |
| `void SetMomentum(G4double px, G4double py, G4double pz)` | - |
| `inline G4double GetPx() const` | - |
| `inline G4double GetPy() const` | - |
| `inline G4double GetPz() const` | - |
| `inline G4PrimaryParticle* GetNext() const` | - |
| `inline void SetNext(G4PrimaryParticle* np)` | - |
| `inline void ClearNext()` | - |
| `inline G4PrimaryParticle* GetDaughter() const` | - |
| `inline void SetDaughter(G4PrimaryParticle* np)` | - |
| `inline G4int GetTrackID() const` | - |
| `inline void SetTrackID(G4int id)` | - |
| `inline G4ThreeVector GetPolarization() const` | - |
| `inline void SetPolarization(const G4ThreeVector& pol)` | - |
| `inline void SetPolarization(G4double px, G4double py, G4double pz)` | - |
| `inline G4double GetPolX() const` | - |
| `inline G4double GetPolY() const` | - |
| `inline G4double GetPolZ() const` | - |
| `inline G4double GetWeight() const` | - |
| `inline void SetWeight(G4double w)` | - |
| `inline G4double GetProperTime() const` | - |
| `inline void SetProperTime(G4double t)` | - |
| `inline G4VUserPrimaryParticleInformation* GetUserInformation() const` | - |
| `inline void SetUserInformation(G4VUserPrimaryParticleInformation* anInfo)` | - |


## Header: `G4RandomDirection.hh` (Target: `G4RandomDirection`)

**Kind**: Global Utilities / Functions

| Signature | Description |
|---|---|
| `inline G4ThreeVector G4RandomDirection()` | Global helper function |
| `inline G4ThreeVector G4RandomDirection(G4double cosTheta)` | Global helper function |

