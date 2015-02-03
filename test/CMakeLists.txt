enable_testing()
set(PROJECT_TEST_NAME test)
include_directories(${HIREDIS_INCLUDE_DIRS} ${GLOG_INCLUDE_DIRS} ${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS} ${COMMON_INCLUDES})

set(MAIN_TEST_SRC ${TEST_DIR}/main.cpp)


file(GLOB ROUTER_TEST_SRCS ${TEST_DIR}/router/*.cpp)

file(GLOB DRIVER_SRC_FILES ${PROJECT_SOURCE_DIR}/src/driver/*.cpp)

file(GLOB ROUTER_SRC_FILES ${PROJECT_SOURCE_DIR}/src/router/*.cpp)
list(REMOVE_ITEM ROUTER_SRC_FILES ${PROJECT_SOURCE_DIR}/src/router/main.cpp)

add_executable(${PROJECT_TEST_NAME} ${ROUTER_SRC_FILES} ${DRIVER_SRC_FILES} ${MAIN_TEST_SRC} ${ROUTER_TEST_SRCS})
add_dependencies(${PROJECT_TEST_NAME} googletest googlemock)


target_link_libraries(${PROJECT_TEST_NAME}
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GTEST_LIBS_DIR}/libgtest_main.a
    ${GMOCK_LIBS_DIR}/libgmock.a
    ${GMOCK_LIBS_DIR}/libgmock_main.a
    xbee_network
    pthread
)

install(TARGETS ${PROJECT_TEST_NAME} DESTINATION bin)
