cmake_minimum_required(VERSION 3.0.0)

set(MODULE_NAME codec)
project(${MODULE_NAME} CXX)
set(LIB_NAME lib${MODULE_NAME})
set(TEST_NAME ${MODULE_NAME}_test)

message(STATUS "-------- CMake for module ${MODULE_NAME} --------")

#-----------------------------
# Build the module library
#-----------------------------
file(GLOB LIB_HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB LIB_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

SET(LIB_ALL_FILES  ${LIB_HDR_FILES} ${LIB_SRC_FILES})

set(MODULE_INC_DIRS
    #${Boost_INCLUDE_DIRS}
)

set(MODULE_LIB_DEPS
    #${Boost_LIBRARIES}
)

add_library(${LIB_NAME} ${LIB_MODE} ${LIB_ALL_FILES})
target_include_directories(${LIB_NAME} PUBLIC ${MODULE_INC_DIRS})
target_link_libraries(${LIB_NAME} ${MODULE_LIB_DEPS})

# Install the library
install(TARGETS ${LIB_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h DESTINATION ${CMAKE_INSTALL_PREFIX})

#-----------------------------
# Build the module tests
#-----------------------------
#-----------------------------
# Build the module tests
#-----------------------------
file(GLOB TEST_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp")

set(TEST_INC_DIRS
    ${GTest_INCLUDE_DIRS}
    ${MODULE_INC_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)
set(TEST_LIB_DEPS
    ${LIB_NAME}
    ${MODULE_LIB_DEPS}
    ${GTest_LIBRARIES}
)
set(TEST_LIB_DIRS
    ${MODULE_INC_DIRS}
)

include_directories(${TEST_INC_DIRS})
add_executable(${TEST_NAME} ${TEST_SRC_FILES})
link_directories(${TEST_LIB_DIRS})
target_link_libraries(${TEST_NAME} ${TEST_LIB_DEPS})

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} --gtest_output=xml:${TEST_NAME}.xml) 
