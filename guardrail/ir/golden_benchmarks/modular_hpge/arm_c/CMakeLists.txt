cmake_minimum_required(VERSION 3.16)
project(hpge_spectrometer CXX)

set(CMAKE_CXX_STANDARD 17)
find_package(Geant4 REQUIRED)
include(${Geant4_USE_FILE})

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCES
    main.cc
    src/DetectorConstruction.cc
    src/PrimaryGeneratorAction.cc
    src/RunAction.cc
    src/EventAction.cc
    src/SteppingAction.cc
)

add_executable(hpge_sim ${SOURCES})
target_link_libraries(hpge_sim ${Geant4_LIBRARIES})
