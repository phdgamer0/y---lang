cmake_minimum_required(VERSION 3.14)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
project(y_lang_new)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add output directory for the compiled binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/release")

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

# Try to find OpenNN locally first, via environment variables or system paths
find_path(OPENNN_INCLUDE_DIR opennn/opennn.h HINTS $ENV{OPENNN_ROOT} /usr/local/include /usr/include /opt/opennn)
find_library(OPENNN_LIBRARY NAMES opennn HINTS $ENV{OPENNN_ROOT}/build/opennn $ENV{OPENNN_ROOT}/build /usr/local/lib /usr/lib /opt/opennn/build)

if(OPENNN_INCLUDE_DIR AND OPENNN_LIBRARY)
    message(STATUS "Found OpenNN at ${OPENNN_INCLUDE_DIR}")
else()
    message(STATUS "OpenNN not found locally, fetching via CMake (this may take a few minutes)...")
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(OpenNN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(OpenNN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(OpenNN_BUILD_BLANK OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        opennn
        URL https://github.com/Artelnics/opennn/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(opennn)
    set(OPENNN_INCLUDE_DIR "${opennn_SOURCE_DIR}")
    set(OPENNN_LIBRARY opennn)
endif()

# Try to find Eigen3 locally first
find_path(EIGEN_INCLUDE_DIR Eigen/Dense HINTS $ENV{EIGEN_ROOT} $ENV{OPENNN_ROOT}/eigen /usr/local/include/eigen3 /usr/include/eigen3 /usr/local/include /usr/include)
if(EIGEN_INCLUDE_DIR)
    message(STATUS "Found Eigen at ${EIGEN_INCLUDE_DIR}")
else()
    message(STATUS "Eigen not found locally, fetching via CMake...")
    FetchContent_Declare(
        eigen
        URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip
    )
    FetchContent_MakeAvailable(eigen)
    set(EIGEN_INCLUDE_DIR "${eigen_SOURCE_DIR}")
endif()

# Nlohmann JSON
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp HINTS /usr/local/include /usr/include)
if(NLOHMANN_JSON_INCLUDE_DIR)
    message(STATUS "Found nlohmann_json locally at ${NLOHMANN_JSON_INCLUDE_DIR}")
else()
    message(STATUS "nlohmann_json not found locally, fetching via CMake...")
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
    )
    FetchContent_MakeAvailable(nlohmann_json)
    set(NLOHMANN_JSON_INCLUDE_DIR "${nlohmann_json_SOURCE_DIR}/include")
endif()

include_directories(${OPENNN_INCLUDE_DIR} ${EIGEN_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})

# Raylib
find_library(RAYLIB_LIBRARY NAMES raylib)
if(RAYLIB_LIBRARY)
    message(STATUS "Found Raylib locally")
else()
    message(STATUS "Raylib not found locally, fetching via CMake...")
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/919ad68c.zip
    )
    FetchContent_MakeAvailable(raylib)
    set(RAYLIB_LIBRARY raylib)
endif()

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
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenGL REQUIRED)

if(NOT WIN32)
    find_package(Curses REQUIRED)
    find_package(X11 REQUIRED)
endif()

# Source files
set(SOURCES
    src/yrun.cpp
    src/qrcodegen.cpp
    src/types.cpp
    src/errors.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/objects.cpp
    src/compiler.cpp
    src/vm.cpp
    src/hash-library/md5.cpp
    src/hash-library/sha1.cpp
    src/hash-library/sha256.cpp
    src/hash-library/sha3.cpp
    src/hash-library/keccak.cpp
)

add_executable(y_lang_new ${SOURCES})

# Include directories
target_include_directories(y_lang_new PRIVATE include/ylang include src)

# Link libraries (matching -Wl,-Bstatic and -Wl,-Bdynamic semantics as much as possible)
if(WIN32)
    target_link_libraries(y_lang_new PRIVATE
        ${OPENNN_LIBRARY}
        ${RAYLIB_LIBRARY}
        ${REPLXX_LIBRARY}
        OpenSSL::SSL OpenSSL::Crypto
        ZLIB::ZLIB zstd
        OpenGL::GL
        ws2_32 crypt32
    )
else()
    target_link_libraries(y_lang_new PRIVATE
        ${OPENNN_LIBRARY}
        ${RAYLIB_LIBRARY}
        ${REPLXX_LIBRARY}
        OpenSSL::SSL OpenSSL::Crypto
        ${CURSES_LIBRARIES} tinfo
        ZLIB::ZLIB zstd
        OpenGL::GL
        X11::X11 Xtst
        m
        Threads::Threads
        dl
        rt
    )
endif()
