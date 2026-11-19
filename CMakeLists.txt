cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
project(ymm)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add output directory for the compiled binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/release")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use flags exactly as requested + CPU optimizations
# Off by default: binaries built with -march=native crash ("illegal instruction") on other CPUs
option(YLANG_NATIVE "Optimize for the build machine's CPU (-march=native)" OFF)
# LTO makes the link step slow; turn off for quicker dev builds
option(YLANG_LTO "Enable link-time optimization" ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0 /O2")
    if(YLANG_LTO)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GL")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LTCG")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -O3")
    if(YLANG_LTO)
        # =auto runs the LTO link on all cores instead of one
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=auto")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
        endif()
    endif()
    if(YLANG_NATIVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie")
    endif()
    if(NOT WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
    endif()
endif()
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# 1. Thread support
find_package(Threads REQUIRED)

# 2. Setup Include Directories and Local Libraries






# Replxx
find_library(REPLXX_LIBRARY NAMES replxx)
if(REPLXX_LIBRARY)
    message(STATUS "Found Replxx locally")
else()
    message(STATUS "Replxx not found locally, fetching via CMake...")
    set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        replxx
        URL https://github.com/AmokHuginnsson/replxx/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(replxx)
    set(REPLXX_LIBRARY replxx)
endif()



# Core Source files (everything except main/yrun)
set(CORE_SOURCES
    src/types.cpp
    src/errors.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/objects.cpp
    src/compiler.cpp
    src/assembler.cpp
    src/serialization.cpp
    src/transpiler.cpp
    src/interpreter.cpp
    src/methods.cpp
    src/vm.cpp
    src/vm_ops.cpp
    src/stdlib/builtins.cpp
    src/stdlib/events.cpp
    src/stdlib/ffi.cpp
    src/stdlib/io.cpp
    src/stdlib/math.cpp
    src/stdlib/regex.cpp
    src/stdlib/serial.cpp
    src/stdlib/time.cpp
)

# Windows: static, since MinGW can't import data (globals) from a DLL without dllimport annotations
if(WIN32)
    add_library(ylang_core STATIC ${CORE_SOURCES})
else()
    add_library(ylang_core SHARED ${CORE_SOURCES})
endif()
set_target_properties(ylang_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(ylang_core PUBLIC include/ylang include src ${replxx_SOURCE_DIR}/include)

add_executable(ymm src/yrun.cpp)

# Include directories
target_include_directories(ymm PRIVATE include/ylang include src)

# Link libraries to the core library
if(WIN32)
    target_link_libraries(ylang_core PRIVATE
        ${REPLXX_LIBRARY}
    )
    target_link_libraries(ylang_core PUBLIC ws2_32)
else()
    target_link_libraries(ylang_core PRIVATE
        ${REPLXX_LIBRARY}
    )
    target_link_libraries(ylang_core PUBLIC
        m
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(ylang_core PUBLIC rt)
    endif()
    # X11/XTest only power the Events module; without them it builds as a no-op
    find_package(X11)
    if(X11_FOUND AND X11_XTest_FOUND)
        target_compile_definitions(ylang_core PRIVATE YLANG_HAS_XTEST)
        target_link_libraries(ylang_core PUBLIC X11::X11 X11::Xtst)
    else()
        message(STATUS "X11/XTest not found: Events module will be a no-op")
    endif()
endif()

# Link the main executable to the core library
target_link_libraries(ymm PUBLIC ylang_core ${REPLXX_LIBRARY})

# Add a permanent target for transpiled scripts to avoid CMake reconfiguration overhead
# yrun overwrites this file on every `ymm -t`; create a stub so a fresh clone configures
set(TRANSPILED_SRC "${CMAKE_SOURCE_DIR}/build/release/transpiled.cpp")
if(NOT EXISTS "${TRANSPILED_SRC}")
    file(WRITE "${TRANSPILED_SRC}" "int main() { return 0; }\n")
endif()
# EXCLUDE_FROM_ALL: a broken transpile must not break `make`; yrun builds it by name
add_executable(transpiled_bin EXCLUDE_FROM_ALL "${TRANSPILED_SRC}")
target_include_directories(transpiled_bin PRIVATE include/ylang include src)
target_link_libraries(transpiled_bin PUBLIC ylang_core)
set_target_properties(transpiled_bin PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/release")
if (NOT MSVC)
    target_compile_options(transpiled_bin PRIVATE -fno-lto)
    target_link_options(transpiled_bin PRIVATE -fno-lto)
endif()
target_compile_definitions(transpiled_bin PRIVATE YLANG_NO_HEAVY_INCLUDES)

# Use precompiled headers for massive speedup during repeated transpilation!
target_precompile_headers(transpiled_bin PRIVATE include/ylang/objects.hpp include/ylang/vm.hpp)
