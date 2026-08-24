# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Yarg device selection: RASPBERRY_PI_PICO or GENERIC_HOST
# determines if cyarg is built as hosted or self-hosted binary
# for Pico, sets the PICO_BOARD variable for the Pico SDK
if (YARG_DEVICE STREQUAL "RASPBERRY_PI_PICO")
set(PICO_BOARD "pico" CACHE STRING "Raspberry Pi Pico board")
elseif (YARG_DEVICE STREQUAL "GENERIC_HOST")
endif()


# Pull in Raspberry Pi Pico SDK (must be before project)
if(YARG_DEVICE STREQUAL "RASPBERRY_PI_PICO")
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
endif()

project(big-int C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
if(PICO_BOARD)
pico_sdk_init()
endif()

add_compile_definitions(CYARG_FEATURE_TEST_INT)

# Add executable. Default name is the project name, version 0.1

add_executable(big-int ../../cyarg/big-int/big-int.c)

if (PICO_BOARD)

pico_set_program_name(big-int "big-int")
pico_set_program_version(big-int "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(big-int 1)
pico_enable_stdio_usb(big-int 0)

# Add the standard library to the build
target_link_libraries(big-int
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(big-int PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(big-int)
endif()
