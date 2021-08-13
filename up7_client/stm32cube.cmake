#-- STM32 Cube Makefile parser -------------------------------------------------
function(mk_read_vars mkfile)
    file(READ "${mkfile}" file_raw)
    string(REPLACE "\\\n" "" file_raw ${file_raw})
    string(REPLACE "\n" ";" file_lines ${file_raw})
    list(REMOVE_ITEM file_lines "")
    foreach(line ${file_lines})
        string(FIND ${line} "=" loc )
        list(LENGTH line_split count)
        if (loc LESS 2)
            continue()
        endif()
        string(SUBSTRING ${line} 0 ${loc} var_name)
        math(EXPR loc "${loc} + 1")
        string(SUBSTRING ${line} ${loc} -1 value)
        string(STRIP ${value} value)
        string(STRIP ${var_name} var_name)
        set(CUBEMK_${var_name} ${value} PARENT_SCOPE)
    endforeach()
endfunction()

mk_read_vars("Makefile")

#-- Extract needed variables ---------------------------------------------------
# Project name from Makefile
set(STM32_PROJECT "${CUBEMK_TARGET}")

# Linker script name from Makefile
set(STM32_LDSCRIPT "${CUBEMK_LDSCRIPT}")

# Defines from Makefile
set(STM32_DEFINES "${CUBEMK_C_DEFS}")

# Includes from Makefile
set(STM32_INCLUDES "")
string(REPLACE " " ";" CUBEMK_C_INCLUDES ${CUBEMK_C_INCLUDES})
foreach(f ${CUBEMK_C_INCLUDES})
    string(STRIP ${f} f)
    string(REGEX REPLACE "^-I" "" f ${f})
    set(STM32_INCLUDES "${STM32_INCLUDES};${f}")
endforeach()

# C sources from Makefile
string(REPLACE " " ";" CUBEMK_C_SOURCES ${CUBEMK_C_SOURCES})
set(STM32_LIB_SOURCES "")
set(STM32_CORE_SOURCES "")
foreach(f ${CUBEMK_C_SOURCES})
    string(FIND ${f} "Core/" res)
    if (res EQUAL  0)
        set(STM32_CORE_SOURCES "${STM32_CORE_SOURCES};${f}")
    else()
        set(STM32_LIB_SOURCES "${STM32_LIB_SOURCES};${f}")
    endif()
endforeach()

# Assembler sources from Makefile
set(STM32_ASM_SOURCES "")
string(REPLACE " " ";" CUBEMK_ASM_SOURCES ${CUBEMK_ASM_SOURCES})
set(STM32_CORE_SOURCES "${STM32_CORE_SOURCES};${CUBEMK_ASM_SOURCES}")
