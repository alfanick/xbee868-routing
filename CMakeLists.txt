cmake_minimum_required(VERSION 2.8.8)
set(PROJECT_NAME_STR xbee868-routing-code)
project(${PROJECT_NAME_STR} CXX)
include(ExternalProject)

# Modify compiler flags here

set(CXX_CLANG_FLAGS "-Wall -Wextra -Wno-unused-parameter -std=c++11 -stdlib=libc++")
set(CXX_GNU_FLAGS "-std=gnu++11 -Wall -pthread -fPIC")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")


#CMake adds automatically debug/release compiler flags, such as (-g -O0 or -O3)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_CLANG_FLAGS}")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_GNU_FLAGS}")
endif()

set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/src)
set(TEST_DIR ${PROJECT_SOURCE_DIR}/test)

ExternalProject_Add(googlemock
    SVN_REPOSITORY http://googlemock.googlecode.com/svn/trunk
    PREFIX "${EXT_PROJECTS_DIR}/gmock"
    
    # Disable install step
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(googlemock source_dir)
set(GMOCK_INCLUDE_DIRS ${source_dir}/include)

ExternalProject_Get_Property(googlemock binary_dir)
set(GMOCK_LIBS_DIR ${binary_dir})

ExternalProject_Add(googletest
    SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk
    CMAKE_ARGS -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
               -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
               -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
               -Dgtest_force_shared_crt=ON
    PREFIX "${EXT_PROJECTS_DIR}/gtest"
    
    # Disable install step
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(googletest source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/include)

ExternalProject_Get_Property(googletest binary_dir)
set(GTEST_LIBS_DIR ${binary_dir})

ExternalProject_Add(gflags
  GIT_REPOSITORY https://code.google.com/p/gflags/
  GIT_TAG v1.7
  PREFIX "${EXT_PROJECTS_DIR}/gflags"

  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "./configure" --with-pic && make
  INSTALL_COMMAND ""
  UPDATE_COMMAND ""
)

ExternalProject_Get_Property(gflags source_dir)
set(GFLAGS_INCLUDE_DIRS ${source_dir}/src)

ExternalProject_Get_Property(gflags binary_dir)
set(GFLAGS_LIBS_DIR ${binary_dir}/.libs)

ExternalProject_Add(glog
    DEPENDS gflags
    CMAKE_ARGS -DCMAKE_LIBRARY_PATH=${GFLAGS_LIBS_DIR}
    SVN_REPOSITORY http://google-glog.googlecode.com/svn/trunk
    PREFIX "${EXT_PROJECTS_DIR}/glog"
    
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND "./configure" --with-pic && make
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(glog source_dir)
set(GLOG_INCLUDE_DIRS ${source_dir}/src)

ExternalProject_Get_Property(glog binary_dir)
set(GLOG_LIBS_DIR ${binary_dir}/.libs)

ExternalProject_Add(hiredis
    GIT_REPOSITORY https://github.com/redis/hiredis.git
    PREFIX "${EXT_PROJECTS_DIR}/hiredis"

    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND "make" static
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

ExternalProject_Get_Property(hiredis source_dir)
set(HIREDIS_INCLUDE_DIRS ${source_dir}/..)

ExternalProject_Get_Property(hiredis binary_dir)
set(HIREDIS_LIBS_DIR ${binary_dir})

add_subdirectory(${SOURCE_DIR}/driver)
add_subdirectory(${SOURCE_DIR}/router)
add_subdirectory(${SOURCE_DIR}/apps)

add_subdirectory(${TEST_DIR})

# Docs generation
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM )
endif(DOXYGEN_FOUND)
