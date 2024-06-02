# CMakeList.txt : CMake project for cow, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
#if (POLICY CMP0141)
#  cmake_policy(SET CMP0141 NEW)
#  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
#endif()

project ("cow")

add_library(cow
  "src/cow.cpp"
  "include/cow/ptr.h"
  "include/cow/detail/control_block.h"
  "include/cow/detail/ptr.h" "include/cow/path.h")

target_include_directories(cow PUBLIC "include")


find_package(GTest CONFIG REQUIRED)
add_executable(cow_test "test/ptr_test.cpp" "test/path_test.cpp")
target_link_libraries(cow_test PUBLIC cow GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cow_test)

set_property(TARGET cow PROPERTY CXX_STANDARD 20)
set_property(TARGET cow_test PROPERTY CXX_STANDARD 20)
