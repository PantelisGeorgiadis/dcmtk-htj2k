cmake_minimum_required(VERSION 3.12.0)

project(DCMTKHTJ2K)

set (DCMTKHTJ2K_VERSION_MAJOR 1)
set (DCMTKHTJ2K_VERSION_MINOR 0)
set (DCMTKHTJ2K_VERSION_BUILD 0)
set(DCMTKHTJ2K_VERSION "${DCMTKHTJ2K_VERSION_MAJOR}.${DCMTKHTJ2K_VERSION_MINOR}.${DCMTKHTJ2K_VERSION_BUILD}")

option(BUILD_SHARED_LIBS "Build DCMTKHTJ2K shared library" ON)

set (CMAKE_CXX_STANDARD 11)

find_package(DCMTK REQUIRED)
find_package(OPENJPH REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include ${DCMTK_INCLUDE_DIRS} ${OPENJPH_INCLUDE_DIRS})
LINK_DIRECTORIES(${DCMTKHTJ2K}/lib)

add_definitions(-DDCMTKHTJ2K_EXPORTS)

set(DCMTKHTJ2K_HEADERS
	include/dcmtkhtj2k/djcodecd.h
	include/dcmtkhtj2k/djcodece.h
	include/dcmtkhtj2k/djcparam.h
	include/dcmtkhtj2k/djdecode.h
	include/dcmtkhtj2k/djencode.h
	include/dcmtkhtj2k/djutils.h
	include/dcmtkhtj2k/djrparam.h
	include/dcmtkhtj2k/dldefine.h
)

set(DCMTKHTJ2K_SRCS
    ${DCMTKHTJ2K_HEADERS}
	libsrc/djcodecd.cc
	libsrc/djcodece.cc
	libsrc/djcparam.cc
	libsrc/djdecode.cc
	libsrc/djencode.cc
	libsrc/djrparam.cc
	libsrc/djutils.cc
)

if (MSVC)
	add_compile_options(/Zc:__cplusplus)
endif()
if(WIN32)
	add_definitions(-D_BIND_TO_CURRENT_VCLIBS_VERSION=1)
endif()

add_library(DCMTKHTJ2K ${DCMTKHTJ2K_SRCS})
set(DCMTKHTJ2K_LIBRARY_NAME DCMTKHTJ2K)
TARGET_LINK_LIBRARIES(DCMTKHTJ2K
  DCMTK::DCMTK
  openjph
  )

include(GenerateExportHeader)
generate_export_header(DCMTKHTJ2K)
set_property(TARGET DCMTKHTJ2K PROPERTY VERSION ${DCMTKHTJ2K_VERSION})
set_property(TARGET DCMTKHTJ2K PROPERTY SOVERSION 1)
set_property(TARGET DCMTKHTJ2K PROPERTY
  INTERFACE_DCMTKHTJ2K_MAJOR_VERSION 1)
set_property(TARGET DCMTKHTJ2K APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING DCMTKHTJ2K_MAJOR_VERSION
)

install(TARGETS DCMTKHTJ2K EXPORT DCMTKHTJ2KTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  FILES
    ${DCMTKHTJ2K_HEADERS}
  DESTINATION
    include/DCMTKHTJ2K
  COMPONENT
    Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/DCMTKHTJ2KConfigVersion.cmake"
  VERSION ${DCMTKHTJ2K_VERSION}
  COMPATIBILITY AnyNewerVersion
)

export(TARGETS DCMTKHTJ2K
  FILE "${CMAKE_CURRENT_BINARY_DIR}/DCMTKHTJ2KExports.cmake"
)
configure_file( ${CMAKE_SOURCE_DIR}/cmake/DCMTKHTJ2KConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/DCMTKHTJ2KConfig.cmake
  @ONLY
)

set(ConfigPackageLocation lib/cmake/DCMTKHTJ2K)
install(EXPORT DCMTKHTJ2KTargets
  FILE
    DCMTKHTJ2KTargets.cmake
  DESTINATION
    ${ConfigPackageLocation}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DCMTKHTJ2KConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DCMTKHTJ2KConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)

option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING)
  enable_testing()
  
  # Fetch Google Test
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )
  
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  
  FetchContent_MakeAvailable(googletest)
  
  add_subdirectory(tests)
endif()
