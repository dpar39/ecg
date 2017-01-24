#========================================
# Passport Photo Print application (ppp-app)
#========================================
cmake_minimum_required(VERSION 2.8.12)

project(ECGDemo CXX)

SET(Boost_USE_STATIC_LIBS        ON)
SET(Boost_USE_MULTITHREADED      ON)
SET(Boost_USE_STATIC_RUNTIME    OFF)

add_definitions(-D_USE_BOOST)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
set(GMOCK_ROOT "${THIRD_PARTY_DIR}/gmock-1.7.0" CACHE INTERNAL "" FORCE)
set(EIGEN_ROOT "${THIRD_PARTY_DIR}/eigen-eigen-da9b4e14c255" CACHE INTERNAL "" FORCE)


if (CMAKE_SIZEOF_VOID_P MATCHES "4")
    set(ARCH_NAME "x86")
else()
    set(ARCH_NAME "x64")
endif()
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
set(THIRD_PARTY_DIR_INSTALL "${THIRD_PARTY_DIR}/install_${BUILD_TYPE}_${ARCH_NAME}")

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${THIRD_PARTY_DIR_INSTALL}" CACHE INTERNAL "" FORCE)

# Set compilers flags globally
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${GMOCK_ROOT}/include\"" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem \"${GMOCK_ROOT}/gtest/include\"" CACHE STRING "" FORCE)
endif()
if (WIN32)
    set(gtest_disable_pthreads ON CACHE INTERNAL "" FORCE)
    set(gtest_force_shared_crt ON CACHE INTERNAL "" FORCE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()

# Configure shared data
configure_file("${CMAKE_SOURCE_DIR}/data/config.h.in" "${CMAKE_SOURCE_DIR}/data/config.h")
include_directories(${CMAKE_SOURCE_DIR})

# Add GMock/GTest as it is needed by all modules
include_directories(${GMOCK_ROOT}/gtest/include)
include_directories(${GMOCK_ROOT}/include)
include_directories(${THIRD_PARTY_DIR})
add_subdirectory(${GMOCK_ROOT})

# Enable testing
enable_testing()

# Add the ECG library for build and tests
include_directories(libecg/include)
add_subdirectory(libecg)


