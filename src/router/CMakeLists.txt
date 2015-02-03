set(ROUTER_INCLUDES ${PROJECT_SOURCE_DIR}/src/driver)
file(GLOB ROUTER_SRC_FILES ${PROJECT_SOURCE_DIR}/src/router/*.cpp)

include_directories(${HIREDIS_INCLUDE_DIRS} ${GLOG_INCLUDE_DIRS} ${COMMON_INCLUDES} ${ROUTER_INCLUDES})

add_executable(router ${ROUTER_SRC_FILES})
target_link_libraries(router xbee_network)

install(TARGETS router DESTINATION bin)
