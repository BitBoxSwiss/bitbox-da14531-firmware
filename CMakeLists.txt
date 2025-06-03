cmake_minimum_required(VERSION 3.16)

project(bitbox-da14531-firmware LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include Dialog SDK
set(DIALOG_SDK_PATH $ENV{DIALOG_SDK_PATH})
include(${CMAKE_SOURCE_DIR}/dialog-sdk.cmake)

# PRODUCTION_DEBUG_OUTPUT will output some status information if the watchdog
# or hardfault triggers.
option(PRODUCTION_DEBUG_OUTPUT NO)

add_custom_target(generated-version-header
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/src
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/get_version --header > ${CMAKE_BINARY_DIR}/src/__version.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/src/__version.h ${CMAKE_BINARY_DIR}/src/version.h
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/src/__version.h
    COMMENT "Generating src/version.h if different"
    BYPRODUCTS ${CMAKE_BINARY_DIR}/src/version.h
)

add_executable(${PROJECT_NAME}
    ${DIALOG_SDK_SOURCES}
    # Libraries
    external/RTT/SEGGER_RTT.c
    external/RTT/SEGGER_RTT_printf.c
    # Debug
    src/debug.c
    # Source
    src/user_custs_config.c
    src/user_custs1_def.c
    src/user_custs1_impl.c
    src/user_periph_setup.c
    src/user_app.c
    src/uart_task.c
    src/serial_link.c
    src/crc.c
)

add_dependencies(${PROJECT_NAME} generated-version-header)

# Valid types are Debug, Release, RelWithDebInfo
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # DEVELOPMENT_DEBUG will
    # * cause a debugger break on hardfaults,
    # * simulate OTP loading,
    # * cause a debugger break if same pin is allocated twice (unless that is
    #   disabled with GPIO_DRV_PIN_ALLOC_MON_DISABLED)
    #
    # GPIO_DRV_PIN_ALLOC_MON_DISABLED can be commented out to add runtime
    # checks if pins are by mistake double reserved.
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        CFG_DEVELOPMENT_DEBUG
        GPIO_DRV_PIN_ALLOC_MON_DISABLED
        DEBUG_SEGGER
    )
else() # Release / RelWithDebInfo
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NDEBUG
        $<$<BOOL:${PRODUCTION_DEBUG_OUTPUT}>:CFG_PRODUCTION_DEBUG_OUTPUT>
    )
endif()

#  __STACK_SIZE can be used to reduce the stack size, default is 0x700. Check
#  startup_DA14531.S
target_compile_definitions(${PROJECT_NAME} PRIVATE
    __DA14531__
    #__STACK_SIZE=0x500
)

target_compile_options(${PROJECT_NAME} PRIVATE
    "-mcpu=cortex-m0plus"
    "-mthumb"
    "-flto"
    "-fsigned-char"
    "-Os"
    "-ggdb"
    "--specs=nano.specs"
    "--specs=nosys.specs"
    $<$<COMPILE_LANGUAGE:C>:-include${CMAKE_SOURCE_DIR}/config/da1458x_config_basic.h>
    $<$<COMPILE_LANGUAGE:C>:-include${CMAKE_SOURCE_DIR}/config/da1458x_config_advanced.h>
    $<$<COMPILE_LANGUAGE:C>:-include${CMAKE_SOURCE_DIR}/config/user_config.h>
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    $<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-variable>
    $<$<COMPILE_LANGUAGE:C>:-Wno-address>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Werror>
)

set(USER_INCLUDES
    ${CMAKE_SOURCE_DIR}/config
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/RTT
)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:${DIALOG_SDK_INCLUDES}>
    $<$<COMPILE_LANGUAGE:C,CXX>:${USER_INCLUDES}>
)

# Linker file needs to be preprocessed
set(LINKER_SCRIPT_OUT "ldscript.lds")

# Linker script includes
set(LINKER_SCRIPT_INCLUDES
    -I ${CMAKE_SOURCE_DIR}/config
    -I ${DIALOG_SDK_PATH}/sdk/common_project_files
    -I ${DIALOG_SDK_PATH}/sdk/common_project_files/ldscripts
)

link_directories(
    ${CMAKE_SOURCE_DIR}/gcc
)

function(generate_linker_script target_name linker_script_includes linker_script_defs linker_script_input linker_script_output)
    # Check the input file
    if(NOT EXISTS ${linker_script_input})
        message(FATAL_ERROR "Linker input file not found. ${linker_script_input}")
    else()
        message(STATUS "Found linker input file: ${linker_script_input}")
    endif()

    add_custom_command(OUTPUT ${linker_script_output}
                    COMMAND ${CMAKE_C_COMPILER} ${linker_script_includes} ${linker_script_defs} -E -P ${linker_script_input} -o ${linker_script_output}
                    DEPENDS ${linker_script_input} ${CMAKE_SOURCE_DIR}/config/da14531_config_basic.h ${CMAKE_SOURCE_DIR}/config/da14531_config_advanced.h ${CMAKE_SOURCE_DIR}/config/user_config.h
                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                    COMMENT "Generating linker script: ${linker_script_output}"
                    VERBATIM)

    add_custom_target(linkerscript DEPENDS ${linker_script_output} VERBATIM)

    # Add linker preprocessed file as dependency
    add_dependencies(${target_name} linkerscript)
    # Add dependency on linker input file
    set_target_properties(${target_name} PROPERTIES LINK_DEPENDS ${linker_script_input})
endfunction(generate_linker_script)

generate_linker_script(${PROJECT_NAME} "${LINKER_SCRIPT_INCLUDES}" "-D__DA14531__" "${CMAKE_SOURCE_DIR}/ldscript_DA14531.lds.S" "${LINKER_SCRIPT_OUT}")

target_link_directories(${PROJECT_NAME} PRIVATE
    ${DIALOG_SDK_PATH}/sdk/gcc
    ${DIALOG_SDK_PATH}/sdk/platform/system_library/output/IAR/
    ${DIALOG_SDK_PATH}/sdk/common_project_files/scatterfiles/
    ${DIALOG_SDK_PATH}/sdk/common_project_files/misc/
)

# Linker specs flags are already set in `arm.cmake`
target_link_libraries(${PROJECT_NAME} PRIVATE
    :da14531.a
)

# SPECS are defined as default linker flags in arm.cmake
target_link_options(${PROJECT_NAME} PRIVATE
    "-mcpu=cortex-m0plus"
    "-mthumb"
    "-flto"
    "-Os"
    "-T${LINKER_SCRIPT_OUT}"
    "-Wl,--gc-sections"
    "-Wl,--print-memory-usage"
    "-Wl,--no-wchar-size-warning" # Suppress the warning from linking Dialog's system library
    "-Wl,-Map,output.map" # Produce map file
)

# Post build actions

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Print output application size"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary "$<TARGET_FILE:${PROJECT_NAME}>" ${PROJECT_NAME}.bin
    COMMENT "Convert output to binary"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Ibinary -Oelf32-littlearm --rename-section .data=.rodata,alloc,load,readonly,data,contents ${PROJECT_NAME}.bin ${PROJECT_NAME}.o
    COMMENT "Convert output to object file for inclusion in other firmware"
)
