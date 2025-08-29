# PICO SDK defaults to 3.13 minimum
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

if(PICO_BOARD)
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

set(PICO_USE_FASTEST_SUPPORTED_CLOCK 1)
endif()

# The PICO SDK will also need CXX & ASM toolchains to assemble the firmware image
project(cvalue VERSION 0.1.0 LANGUAGES C CXX ASM)


if (PICO_BOARD)
# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
endif()


add_executable(cvalue
    generic_size.c
    value_nan.c
    value_union.c
    value_main.c)


if (PICO_BOARD)
set(PICO_USE_FASTEST_SUPPORTED_CLOCK 1)

endif()


if (PICO_BOARD)
# Setup the firmware image and details of the executable it will host


pico_set_program_name(cvalue "cvalue")
pico_set_program_version(cvalue "0.1.0")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(cvalue 1)
pico_enable_stdio_usb(cvalue 1)

# Add the standard library to the build
target_link_libraries(cvalue
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(cvalue PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(cvalue)
endif()