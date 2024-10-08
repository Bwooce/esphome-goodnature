cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/tools/unit-test-app/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(goodnature_ble_test)

idf_component_register(SRC_DIRS "."
                       INCLUDE_DIRS "."
                       REQUIRES unity esp32_ble_tracker)

add_executable(${PROJECT_NAME} test_goodnature_ble.cpp ../goodnature_ble_listener.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ..)
target_link_libraries(${PROJECT_NAME} PRIVATE idf::freertos idf::unity idf::bt)
