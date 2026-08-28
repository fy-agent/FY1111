cmake_minimum_required(VERSION 3.16)
# Registry package pkolt/vl53l0x 1.0.0 pins idf>=6.0. The same git sources use
# i2c_master (IDF 5.2+); clone them locally and relax that pin for IDF 5.5.4.
set(VL53L0X_DIR "${CMAKE_CURRENT_LIST_DIR}/components/vl53l0x")
if(NOT EXISTS "${VL53L0X_DIR}/CMakeLists.txt")
    execute_process(
        COMMAND git clone --depth 1 --branch master https://github.com/pkolt/vl53l0x-esp-idf.git "${VL53L0X_DIR}"
        RESULT_VARIABLE VL53L0X_CLONE
    )
    if(NOT VL53L0X_CLONE EQUAL 0)
        message(FATAL_ERROR "failed to clone https://github.com/pkolt/vl53l0x-esp-idf")
    endif()
endif()
if(EXISTS "${VL53L0X_DIR}/idf_component.yml")
    file(READ "${VL53L0X_DIR}/idf_component.yml" VL53L0X_YML)
    string(REPLACE "idf: \">=6.0\"" "idf: \">=5.4\"" VL53L0X_YML "${VL53L0X_YML}")
    file(WRITE "${VL53L0X_DIR}/idf_component.yml" "${VL53L0X_YML}")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(board_c_demo)

