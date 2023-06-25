cmake_minimum_required(VERSION 3.15)

project(tomoconv CXX)

set(CMAKE_CXX_STANDARD 20)

if (WIN32)
    set(OPT_FLAGS "/O2")
    set(WARN_FLAGS "")
    set(XTRA_FLAGS "/Zc:__cplusplus /arch:AVX2")
else ()
    set(OPT_FLAGS "-Og -g3")
    set(WARN_FLAGS "-Wall -Wextra -W")
    set(XTRA_FLAGS "-mfma -mavx2")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_FLAGS} ${WARN_FLAGS} ${XTRA_FLAGS}")

find_package(pugixml REQUIRED)
find_package(DCMTK REQUIRED)

# This little hack is because I develop on Debian
if (NOT WIN32)
    set(DCMTK::DCMTK ${DCMTK_LIBRARIES})
endif ()

add_executable(${PROJECT_NAME}
            ${CMAKE_SOURCE_DIR}/main.cpp
            ${CMAKE_SOURCE_DIR}/src/archive.cpp
            ${CMAKE_SOURCE_DIR}/src/log.cpp
            ${CMAKE_SOURCE_DIR}/src/machine.cpp
            ${CMAKE_SOURCE_DIR}/src/patient.cpp
            ${CMAKE_SOURCE_DIR}/src/disease.cpp
            ${CMAKE_SOURCE_DIR}/src/image.cpp
            ${CMAKE_SOURCE_DIR}/src/plan.cpp
            ${CMAKE_SOURCE_DIR}/src/structures.cpp
            #${CMAKE_SOURCE_DIR}/src/ivdt.cpp
            ${CMAKE_SOURCE_DIR}/src/dbinfo.cpp
            ${CMAKE_SOURCE_DIR}/src/lookup.cpp
            ${CMAKE_SOURCE_DIR}/src/error.cpp
            ${CMAKE_SOURCE_DIR}/src/dicom/dicom.cpp
            ${CMAKE_SOURCE_DIR}/src/dicom/ctseries.cpp
            ${CMAKE_SOURCE_DIR}/src/dicom/rtdose.cpp
            ${CMAKE_SOURCE_DIR}/src/dicom/rtstruct.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
                           ${CMAKE_SOURCE_DIR}/src
                           ${CMAKE_SOURCE_DIR}/src/dicom)

target_link_libraries(${PROJECT_NAME}
              PRIVATE pugixml DCMTK::DCMTK)

if (WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY
                 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

