# PICO SDK defaults to 3.13 minimum
cmake_minimum_required(VERSION 3.13)

if(PICO_BOARD)
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
endif()

# The PICO SDK will also need CXX & ASM toolchains to assemble the firmware image
project(clox VERSION 0.1.0 LANGUAGES C CXX ASM)


if (PICO_BOARD)
# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
endif()

add_executable(clox 
    chunk.c
    main.c
    memory.c
    debug.c
    value.c
    vm.c
    compiler.c
    scanner.c
    object.c
    table.c)

add_compile_definitions(NAN_BOXING)
add_compile_definitions(DEBUG_PRINT_CODE)
#add_compile_definitions(DEBUG_TRACE_EXECUTION)
add_compile_definitions(DEBUG_STRESS_GC)
#add_compile_definitions(DEBUG_LOG_GC)

if (PICO_BOARD)
add_compile_definitions(HOST_REPL)
add_compile_definitions(LOX_PICO_SDK)
endif()


if (PICO_BOARD)
# Setup the firmware image and details of the executable it will host


pico_set_program_name(clox "clox")
pico_set_program_version(clox "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(clox 0)
pico_enable_stdio_usb(clox 1)

# Add the standard library to the build
target_link_libraries(clox
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(clox PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(clox)
endif()