cmake_minimum_required(VERSION 3.12.0)

# Add test executable
add_executable(dcmtkhtj2k_tests
    test.cc
)

# Link against Google Test and the library
target_link_libraries(dcmtkhtj2k_tests
    PRIVATE
    DCMTK::DCMTK
    DCMTKHTJ2K
    gtest_main
)

# Include directories
target_include_directories(dcmtkhtj2k_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${DCMTK_INCLUDE_DIRS}
)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(dcmtkhtj2k_tests)
