if(NOT PICO_BOARD)
message(FATAL_ERROR "PICO_BOARD is not set")
endif()

# take non-default faster clocks
set(PICO_USE_FASTEST_SUPPORTED_CLOCK 1)
# we will intercept and implement the c malloc/free family.
set(SKIP_PICO_MALLOC 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

target_compile_definitions(cyarg PRIVATE CYARG_PICO_SDK_TARGET)
target_compile_definitions(cyarg PRIVATE CYARG_PICO_STDLIB)
target_compile_definitions(cyarg PRIVATE CYARG_PICO_SDK_SYNC)
target_compile_definitions(cyarg PRIVATE CYARG_PICO_BUSY_SYNC)
target_compile_definitions(cyarg PRIVATE PICO_STACK_SIZE=0x1000)

# a library that wraps the C heap functions, so we can intercept 
# them and manage memory ourselves.
pico_add_library(vm_c_heap)
target_sources(vm_c_heap INTERFACE vm-c-heap/vm-c-heap.c)
pico_wrap_function(vm_c_heap malloc)
pico_wrap_function(vm_c_heap calloc)
pico_wrap_function(vm_c_heap realloc)
pico_wrap_function(vm_c_heap free)

# Setup the firmware image and details of the executable it will host
pico_set_program_name(cyarg "cyarg")
pico_set_program_version(cyarg "0.1.0")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(cyarg 1)
pico_enable_stdio_usb(cyarg 1)

# Add the standard library to the build
target_link_libraries(cyarg
        hardware_pio
        hardware_flash
        vm_c_heap
        pico_stdlib
        pico_multicore
        pico_sync
        )

# Add the standard include files to the build
target_include_directories(cyarg PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

pico_set_linker_script(cyarg ${CMAKE_SOURCE_DIR}/memmap_custom.ld)

pico_add_extra_outputs(cyarg)
