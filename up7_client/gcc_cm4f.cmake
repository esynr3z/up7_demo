#-- System ---------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CPU "-mcpu=cortex-m4")
set(FPU "-mfpu=fpv4-sp-d16")
set(FLOAT-ABI "-mfloat-abi=hard")
set(MCU "${CPU} -mthumb ${FPU} ${FLOAT-ABI}")

#-- Toolchain ------------------------------------------------------------------
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy" CACHE STRING "objcopy tool")
set(CMAKE_OBJDUMP "arm-none-eabi-objdump" CACHE STRING "objdump tool")
set(CMAKE_SIZE "arm-none-eabi-size" CACHE STRING "size tool")

#-- Common flags ---------------------------------------------------------------
set(COMPILER_COMMON_FLAGS "${MCU} -Wall -Wextra -ffunction-sections -fdata-sections -mlong-calls")
set(CMAKE_C_FLAGS "${COMPILER_COMMON_FLAGS} -std=gnu99" CACHE STRING "c compiler flags")
set(CMAKE_CXX_FLAGS "${COMPILER_COMMON_FLAGS} -std=c++11" CACHE STRING "c++ compiler flags")
set(CMAKE_ASM_FLAGS "-x assembler-with-cpp ${COMPILER_COMMON_FLAGS}" CACHE STRING "assembler compiler flags")
set(CMAKE_EXE_LINKER_FLAGS "${MCU} -mlong-calls -Wl,--gc-sections  -specs=nosys.specs -specs=nano.specs -lgcc -lnosys -lc -lm" CACHE STRING "executable linker flags")
set(CMAKE_MODULE_LINKER_FLAGS "${MCU}" CACHE STRING "module linker flags")
set(CMAKE_SHARED_LINKER_FLAGS "${MCU}" CACHE STRING "shared linker flags")

#-- Debug flags ----------------------------------------------------------------
set(CMAKE_C_FLAGS_DEBUG "-g -gdwarf-2 -DDEBUG" CACHE STRING "c compiler flags debug")
set(CMAKE_CXX_FLAGS_DEBUG "-g -gdwarf-2 -DDEBUG" CACHE STRING "c++ compiler flags debug")
set(CMAKE_ASM_FLAGS_DEBUG "-g -gdwarf-2" CACHE STRING "assembler compiler flags debug")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE STRING "linker flags debug")

#-- Release flags --------------------------------------------------------------
set(CMAKE_C_FLAGS_RELEASE "-O2 " CACHE STRING "c compiler flags release")
set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "c++ compiler flags release")
set(CMAKE_ASM_FLAGS_RELEASE "" CACHE STRING "assembler compiler flags release")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "" CACHE STRING "linker flags release")

#-- Release with debug info flags ----------------------------------------------
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2" CACHE STRING "c compiler flags release with debug info")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2" CACHE STRING "c++ compiler flags release with debug info")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "" CACHE STRING "assembler compiler flags release with debug info")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "" CACHE STRING "linker flags release with debug info")

#-- Minimum size release flags -------------------------------------------------
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -flto -ffat-lto-objects" CACHE STRING "c compiler flags minimum size release")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -flto -ffat-lto-objects" CACHE STRING "c++ compiler flags minimum size release")
set(CMAKE_ASM_FLAGS_MINSIZEREL "" CACHE STRING "assembler compiler flags minimum size release")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-flto" CACHE STRING "linker flags minimum size release")
