set(DRIVER_INCLUDES ${PROJECT_SOURCE_DIR}/src/driver)
file(GLOB DRIVER_SRC_FILES ${PROJECT_SOURCE_DIR}/src/driver/*.cpp)

include_directories(${GLOG_INCLUDE_DIRS} ${HIREDIS_INCLUDE_DIRS} ${COMMON_INCLUDES} ${DRIVER_INCLUDES})

add_library(xbee_network SHARED ${DRIVER_SRC_FILES})
target_link_libraries(xbee_network
  ${GFLAGS_LIBS_DIR}/libgflags.a
  ${GLOG_LIBS_DIR}/libglog.a
  ${HIREDIS_LIBS_DIR}/libhiredis.a)

add_dependencies(xbee_network hiredis glog)

install(TARGETS xbee_network DESTINATION bin)
