cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
project(y_lang_new)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add output directory for the compiled binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/release")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use flags exactly as requested + CPU optimizations
if(MSVC)
    # MSVC specific flags for optimization
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0 /O2 /GL /openmp")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
else()
    # GCC/Clang specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -O3 -flto -fopenmp -march=native -mtune=native")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")
    if(NOT WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie -rdynamic")
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

# System packages
find_package(ZLIB REQUIRED)


# Core Source files (everything except main/yrun)
set(CORE_SOURCES
    src/types.cpp
    src/errors.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/objects.cpp
    src/compiler.cpp
    src/transpiler.cpp
    src/vm.cpp
)

add_library(ylang_core SHARED ${CORE_SOURCES})
set_target_properties(ylang_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(ylang_core PUBLIC include/ylang include src ${replxx_SOURCE_DIR}/include)

add_executable(y_lang_new src/yrun.cpp)

# Include directories
target_include_directories(y_lang_new PRIVATE include/ylang include src)

# Link libraries to the core library
if(WIN32)
    target_link_libraries(ylang_core PRIVATE
        ${REPLXX_LIBRARY}
    )
    target_link_libraries(ylang_core PUBLIC
        ZLIB::ZLIB zstd
    )
else()
    target_link_libraries(ylang_core PRIVATE
        ${REPLXX_LIBRARY}
    )
    target_link_libraries(ylang_core PUBLIC
        ZLIB::ZLIB zstd
        m
        Threads::Threads
        dl
        X11
        Xtst
        rt
    )
endif()

# Link the main executable to the core library
target_link_libraries(y_lang_new PUBLIC ylang_core ${REPLXX_LIBRARY})

# Add a permanent target for transpiled scripts to avoid CMake reconfiguration overhead
add_executable(transpiled_bin "${CMAKE_SOURCE_DIR}/build/release/transpiled.cpp")
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
