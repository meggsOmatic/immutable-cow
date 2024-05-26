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

find_package(GTest CONFIG REQUIRED)

# Add source to this project's executable.
add_library(cow "src/cow.cpp")
target_include_directories(cow PUBLIC "include")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET cow PROPERTY CXX_STANDARD 20)
endif()

enable_testing()
add_executable(cow_test "test/test.cpp")
target_link_libraries(cow_test PRIVATE cow GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(cow_test)
