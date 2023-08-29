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

add_compile_options(-Werror)
# enable clang-tidy checks
#set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(allowedBuildTypes Debug Release ASAN MSAN UBSAN TSAN Profile HeapProfile)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowedBuildTypes}")
if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
  message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
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

add_library(thread-pool INTERFACE)
target_include_directories(thread-pool INTERFACE ${CMAKE_SOURCE_DIR}/thread-pool/include)
find_package(Threads REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(mt_du mt_du.cpp)
target_link_libraries(mt_du PRIVATE Threads::Threads)

if(CMAKE_BUILD_TYPE STREQUAL Profile)
  target_link_options(mt_du PRIVATE "-pg")
elseif(CMAKE_BUILD_TYPE STREQUAL HeapProfile)
  target_link_libraries(mt_du PRIVATE tcmalloc)
elseif(CMAKE_BUILD_TYPE STREQUAL MSAN)
  target_link_options(mt_du PRIVATE -fsanitize=memory)
endif()

install(TARGETS mt_du)

