set(APPS_INCLUDES ${PROJECT_SOURCE_DIR}/src/driver)
file(GLOB APPS_SRC_FILES ${PROJECT_SOURCE_DIR}/src/apps/*.cpp)

include_directories(${HIREDIS_INCLUDE_DIRS} ${GLOG_INCLUDE_DIRS} ${COMMON_INCLUDES} ${APPS_INCLUDES})

add_executable(rpi_temperature ${PROJECT_SOURCE_DIR}/src/apps/rpi_temperature.cpp)
target_link_libraries(rpi_temperature xbee_network)
install(TARGETS rpi_temperature DESTINATION bin)

add_executable(echo ${PROJECT_SOURCE_DIR}/src/apps/echo.cpp)
target_link_libraries(echo xbee_network)
install(TARGETS echo DESTINATION bin)

add_executable(console ${PROJECT_SOURCE_DIR}/src/apps/console.cpp)
target_link_libraries(console xbee_network readline)
install(TARGETS console DESTINATION bin)
