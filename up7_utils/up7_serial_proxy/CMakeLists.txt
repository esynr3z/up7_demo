cmake_minimum_required(VERSION 3.0)

project(up7_serial_proxy)

include_directories(${PATH_UP7_API})
include_directories(${PATH_UP7_PLATFORM})
include_directories(${PATH_SHARED})
include_directories(${PATH_SHARED_PLATFORM})
include_directories(${PATH_UP7_PROXY_API})

IF(WIN32)
    SET(SRC main_win.cpp)
ELSE()
    SET(SRC main_linux.cpp)
ENDIF()
add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries (${PROJECT_NAME} uP7proxy p7 uP7)

set_ide_folder(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
