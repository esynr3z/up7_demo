#-- Misc -----------------------------------------------------------------------
set(CMAKE_TOOLCHAIN_FILE gcc_cm4f.cmake)
enable_language(ASM)
cmake_minimum_required(VERSION 3.8.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include("stm32cube.cmake")

#-- Project config -------------------------------------------------------------
project(${STM32_PROJECT}) # Project name

#-- Defines --------------------------------------------------------------------
add_definitions(${STM32_DEFINES})

#-- Project paths --------------------------------------------------------------
set(APP_PATH App)
set(UP7_PATH up7)

#-- Include dirs ---------------------------------------------------------------
include_directories(${APP_PATH})
include_directories(${UP7_PATH}/lib)
include_directories(${UP7_PATH}/preproc)
include_directories(${STM32_INCLUDES})

#-- Sources --------------------------------------------------------------------
file(GLOB_RECURSE APP_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${APP_PATH}/*.c)
file(GLOB_RECURSE UP7_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${UP7_PATH}/lib/*.c)

# Special case for the uP7 generated header files
set(UP7_PREPROC_H ${UP7_PATH}/preproc/uP7preprocessed.h)
set(UP7_ID_H ${UP7_PATH}/up7/preproc/uP7IDs.h)

#-- Libs -----------------------------------------------------------------------
add_library(stm32 ${STM32_LIB_SOURCES})
add_library(up7 ${UP7_SRC} ${UP7_PREPROC_H} ${UP7_ID_H})

#-- Project linking ------------------------------------------------------------
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_SOURCE_DIR}/${STM32_LDSCRIPT} -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map,--cref,--print-memory-usage,--gc-sections")

add_executable(${PROJECT_NAME}.elf ${STM32_CORE_SOURCES} ${APP_SRC})
target_link_libraries(${PROJECT_NAME}.elf stm32 up7)

#-- Custom commands ------------------------------------------------------------
add_custom_command(OUTPUT ${UP7_PREPROC_H} ${UP7_ID_H}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ../up7_utils/build/uP7preProcessor/Debug/uP7preProcessor ${UP7_PATH}/preproc/up7_preproc.xml ./ ${UP7_PATH}/preproc
    VERBATIM
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} "-Oihex" ${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} "-Obinary" ${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJDUMP} "-DS" ${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dasm
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)
