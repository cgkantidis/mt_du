cmake_minimum_required(VERSION 3.16)

project(mt_du
  VERSION 0.0.1
  DESCRIPTION "A tool to find the disk usage of directories using all available threads"
  LANGUAGES CXX)

# set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# generate the compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# enable all warnings and treat them as errors
add_compile_options(
  -Wall
  -Wextra
  -Wcast-align
  -Wconversion
  -Wfloat-equal
  -Wformat=2
  -Wnon-virtual-dtor
  -Woverloaded-virtual
  -Wshadow
  -Wsign-conversion
  -Wunused
  -pedantic
  -Werror=sign-compare
  -Werror=unused
  -Werror=unused-but-set-variable
  -Werror=unused-variable)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-fcolor-diagnostics)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
  add_compile_options(
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wuseless-cast)
endif()

#add_compile_options(-Werror)

option(ENABLE_STATIC_CHECKS
       "Enable the cppcheck and clang-tidy static code analyzers"
       OFF)
if(ENABLE_STATIC_CHECKS)
  # enable clang-tidy and cppcheck checks
  find_program(CMAKE_CXX_CLANG_TIDY NAMES clang-tidy)
  if(CMAKE_CXX_CLANG_TIDY)
    list(APPEND
         CMAKE_CXX_CLANG_TIDY
         "--use-color"
         "--extra-arg=-Wno-unknown-warning-option")
  endif()

  find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
  if(CMAKE_CXX_CPPCHECK)
    list(APPEND
         CMAKE_CXX_CPPCHECK
         "--language=c++"
         "--std=c++20"
         "--enable=all"
         "--suppress=*:${CMAKE_BINARY_DIR}/*"
         "--suppress=missingIncludeSystem"
         "--report-progress"
         "--template='{file}:{line}:{column}: {severity}: {message} [cppcheck-{id}]\\n{code}'"
    )
  endif()
endif()

set(AVAIL_BUILD_TYPES
    Debug
    Release
    MinSizeRel
    RelWithDebInfo
    ASAN
    MSAN
    UBSAN
    TSAN
    Profile
    HeapProfile)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${AVAIL_BUILD_TYPES}")
# set the CMAKE_BUILD_TYPE to the default if no build type was specified
set(DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}'")
  set(CMAKE_BUILD_TYPE
      "${DEFAULT_BUILD_TYPE}"
      CACHE STRING "Choose the type of build." FORCE)
endif()
if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST AVAIL_BUILD_TYPES)
  message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}\nAvailable build types: ${AVAIL_BUILD_TYPES}")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0" CACHE STRING "Debug build" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3" CACHE STRING "Release build" FORCE)
set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -O1" CACHE STRING "Address Sanitizer build" FORCE)
set(CMAKE_CXX_FLAGS_MSAN "-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -O1" CACHE STRING "Memory Sanitizer build" FORCE)
set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -g -O0" CACHE STRING "Undefined Behaviour Sanitizer" FORCE)
set(CMAKE_CXX_FLAGS_TSAN "-fsanitize=thread -g -O0" CACHE STRING "Thread Sanitizer" FORCE)
set(CMAKE_CXX_FLAGS_PROFILE "-pg -g" CACHE STRING "gprof profiler" FORCE)
set(CMAKE_CXX_FLAGS_HEAPPROFILE "-g -O0" CACHE STRING "Heap Profiler" FORCE)

if("${CMAKE_BUILD_TYPE}" STREQUAL "MSAN" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message(FATAL_ERROR "Memory sanitizer is not currently supported by gcc. Try clang instead.")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
  if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  endif()
endif()

include(FetchContent)
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG        v3.0
  OVERRIDE_FIND_PACKAGE
)
FetchContent_Declare(
  thread-pool
  GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
  GIT_TAG        v4.0.0
  OVERRIDE_FIND_PACKAGE
)
find_package(argparse REQUIRED)
find_package(thread-pool REQUIRED)
find_package(Threads REQUIRED)

add_library(thread-pool INTERFACE)
target_include_directories(thread-pool INTERFACE ${FETCHCONTENT_BASE_DIR}/thread-pool-src/include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(mt_du mt_du.cpp)
target_link_libraries(mt_du PRIVATE argparse thread-pool Threads::Threads)

if(CMAKE_BUILD_TYPE STREQUAL Profile)
  target_link_options(mt_du PRIVATE "-pg")
elseif(CMAKE_BUILD_TYPE STREQUAL HeapProfile)
  target_link_libraries(mt_du PRIVATE tcmalloc)
elseif(CMAKE_BUILD_TYPE STREQUAL MSAN)
  target_link_options(mt_du PRIVATE -fsanitize=memory)
endif()

install(TARGETS mt_du)

