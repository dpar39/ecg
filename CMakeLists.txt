#========================================
# Passport Photo Print application (ppp-app)
#========================================
cmake_minimum_required(VERSION 2.8.12)

project(ECGDemo CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ARCH_NAME "x64")
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_CONFIG_LOWER)
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME_LOWER)
set(BUILD_NAME_SUFFIX ${SYSTEM_NAME_LOWER}_${BUILD_CONFIG_LOWER}_${ARCH_NAME} )
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
set(THIRD_PARTY_INSTALL_DIR "${THIRD_PARTY_DIR}/install_${BUILD_NAME_SUFFIX}")

#----------------------------
# Third party dependencies
#----------------------------
set(CMAKE_PREFIX_PATH
    ${THIRD_PARTY_INSTALL_DIR}
    ${THIRD_PARTY_INSTALL_DIR}/share/OpenCV
    ${THIRD_PARTY_INSTALL_DIR}/lib/cmake/opencv4
)

if(EXISTS ${THIRD_PARTY_INSTALL_DIR}/share/eigen3/cmake/Eigen3Config.cmake)
    set(Eigen3_DIR ${THIRD_PARTY_INSTALL_DIR}/share/eigen3/cmake)
endif()

if(EXISTS ${THIRD_PARTY_INSTALL_DIR}/lib/cmake/RapidJSON/RapidJSONConfig.cmake)
    set(RapidJSON_DIR ${THIRD_PARTY_INSTALL_DIR}/lib/cmake/RapidJSON)
endif()

message(STATUS "Setting Eigen3_DIR=${Eigen3_DIR}")
message(STATUS "Setting RapidJSON_DIR=${RapidJSON_DIR}")

find_package(RapidJSON REQUIRED)
find_package(Eigen3 REQUIRED)

if (NOT ANDROID AND NOT IOS AND NOT DEFINED EMSCRIPTEN)
    set(GTest_DIR ${THIRD_PARTY_INSTALL_DIR})
    set(GTest_INCLUDE_DIRS ${GTest_DIR}/include CACHE INTERNAL "" FORCE)
    message(STATUS "Setting GTest_INCLUDE_DIRS=${GTest_INCLUDE_DIRS}")
    set(DEBUG_SUFFIX "")
    if(${BUILD_CONFIG_LOWER} STREQUAL debug)
        set(DEBUG_SUFFIX "d")
    endif()
    if (MSVC)
        set(GTest_LIBRARIES ${GTest_DIR}/lib/gtest${DEBUG_SUFFIX}.lib ${GTest_DIR}/lib/gmock${DEBUG_SUFFIX}.lib CACHE INTERNAL "" FORCE)
    else()
        set(GTest_LIBRARIES ${GTest_DIR}/lib/libgtest${DEBUG_SUFFIX}.a ${GTest_DIR}/lib/libgmock${DEBUG_SUFFIX}.a CACHE INTERNAL "" FORCE)
    endif()
endif()

# Configure shared data for tests
configure_file("${CMAKE_SOURCE_DIR}/data/config.h.in" "${CMAKE_SOURCE_DIR}/data/config.h")
include_directories(${CMAKE_SOURCE_DIR})

# Add the ECG library for build and tests
include_directories(libecg/include)
add_subdirectory(libecg)

# Add the ECG library for build and tests
include_directories(libcodec/include)
add_subdirectory(libcodec)



