cmake_minimum_required(VERSION 3.0.0)

set(MODULE_NAME ecg)
project(${MODULE_NAME} CXX)
set(LIB_NAME lib${MODULE_NAME})
set(TEST_NAME ${MODULE_NAME}_test)

message(STATUS "-------- CMake for module ${MODULE_NAME} --------")

#----------------------------
# Third party dependencies
#----------------------------
find_package(Boost REQUIRED COMPONENTS filesystem)


#-----------------------------
# Build the module library
#-----------------------------
file(GLOB LIB_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
set(MODULE_INC_DIRS
    ${CMAKE_SOURCE_DIR}/share/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN_ROOT}
)

set(MODULE_LIB_DEPS
    ${Boost_LIBRARIES}
)

if (WIN32)
    # Build a static library for Windows
    set(LIB_MODE STATIC)
else()
    # Build a shared version for Linux
    # Avoid node-gyp mechanism to resolve third party libraries
    set(LIB_MODE SHARED)
endif()
add_library(${LIB_NAME} ${LIB_MODE} ${LIB_SRC_FILES})
target_include_directories(${LIB_NAME} PUBLIC ${MODULE_INC_DIRS})
target_link_libraries(${LIB_NAME} ${MODULE_LIB_DEPS})

# Install the library
install(TARGETS ${LIB_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/libecg.h DESTINATION ${CMAKE_INSTALL_PREFIX})

#-----------------------------
# Build the module tests
#-----------------------------
file(GLOB TEST_SRC_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp")
set(TEST_INC_DIRS
    "tests/include"
    ${GMOCK_ROOT}/gtest/include
    ${GMOCK_ROOT}/include
    ${MODULE_INC_DIRS}
)
set(TEST_LIB_DEPS
    ${LIB_NAME}
    ${MODULE_LIB_DEPS}
    gmock
)
set(TEST_LIB_DIRS
    ${MODULE_INC_DIRS}
)

add_executable(${TEST_NAME} ${TEST_SRC_FILES})
target_include_directories(${TEST_NAME} PUBLIC ${TEST_INC_DIRS})
link_directories(${TEST_LIB_DIRS})
target_link_libraries(${TEST_NAME} ${TEST_LIB_DEPS})

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} --gtest_output=xml:${TEST_NAME}.xml)
