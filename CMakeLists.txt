cmake_minimum_required(VERSION 3.22.0)

# Add nano cmake.
execute_process(COMMAND wget -O - https://nano-cmake.onrender.com/nano.cmake OUTPUT_VARIABLE NANO_CMAKE ERROR_QUIET)
cmake_language(EVAL CODE ${NANO_CMAKE})

project(nano-graphics VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NANO_GRAPHICS_BUILD_TESTS "Build tests." OFF)
option(NANO_GRAPHICS_DEV "Development build" OFF)

# nano-graphics lib.
set(NAME graphics)
set(MODULE_NAME nano-graphics)

nano_add_module(geometry)
set(NANO_GRAPHICS_SRC_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/nano")

file(GLOB_RECURSE NANO_GRAPHICS_SOURCE_FILES
    "${NANO_GRAPHICS_SRC_DIRECTORY}/*.h"
    "${NANO_GRAPHICS_SRC_DIRECTORY}/*.cpp")

add_library(${MODULE_NAME} STATIC ${NANO_GRAPHICS_SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NANO_GRAPHICS_SOURCE_FILES})
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${MODULE_NAME} PUBLIC nano::geometry)

add_library(nano::${NAME} ALIAS ${MODULE_NAME})

set_target_properties(${MODULE_NAME} PROPERTIES XCODE_GENERATE_SCHEME OFF)

if (APPLE) 
    target_link_libraries(${MODULE_NAME} PUBLIC
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework ImageIO"
        "-framework CoreServices"

        
    )
else()
    # target_link_libraries(${PROJECT_NAME} PUBLIC
    #     Gdiplus.lib
    #     d2d1
    # )
    
endif()

if (NANO_GRAPHICS_DEV)
    set(NANO_GRAPHICS_BUILD_TESTS ON)
    # nano_clang_format(${MODULE_NAME} ${NANO_GRAPHICS_SOURCE_FILES})
endif()

if (NANO_GRAPHICS_BUILD_TESTS)
    nano_add_module(test)

    file(GLOB_RECURSE TEST_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h")

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" FILES ${TEST_SOURCE_FILES})

    set(TEST_NAME nano-${NAME}-tests)
    add_executable(${TEST_NAME} ${TEST_SOURCE_FILES})
    target_include_directories(${TEST_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_link_libraries(${TEST_NAME} PUBLIC nano::test ${MODULE_NAME})

    set(CLANG_OPTIONS -Weverything -Wno-c++98-compat)
    set(MSVC_OPTIONS /W4)

    target_compile_options(${TEST_NAME} PUBLIC
        "$<$<CXX_COMPILER_ID:Clang,AppleClang>:${CLANG_OPTIONS}>"
        "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>")

    # set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD 20)
endif()