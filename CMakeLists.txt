cmake_minimum_required(VERSION 3.16)
project(MarsShield)

# Find Geant4 package
find_package(Geant4 REQUIRED)

# Include directories (src folder)
include_directories(${PROJECT_SOURCE_DIR}/src ${Geant4_INCLUDE_DIRS})

# Collect all source files from src/
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/src/*.hh)

# Create the executable
add_executable(MarsShield main.cc ${SOURCES} ${HEADERS})
target_link_libraries(MarsShield ${Geant4_LIBRARIES})

# Copy the macro file to build directory for easy access
configure_file(run_study.mac run_study.mac COPYONLY)