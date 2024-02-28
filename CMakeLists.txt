cmake_minimum_required(VERSION 3.28.3)

set(DTC_OVERLAY_FILE "boards/esp32.overlay")
find_package(Zephyr)

project(rfid_smart_lock)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

add_subdirectory(drivers)
include_directories(include)
