cmake_minimum_required(VERSION 3.13.0) # Pico SDK requires at least 3.13

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# some common definitions for all builds of the cyarg source tree
# note that some have corresponding cmake configuration options for
# CMakeUserPresets.json
# Here at the top of the file for convenience and ease of discovery by
# those building the project.
add_compile_definitions(cyarg CYARG_ORDERED_STRING_TREE)
#add_compile_definitions(cyarg CYARG_CSTDLIB_HEAP)
#add_compile_definitions(cyarg DEBUG_STRESS_GC)
#add_compile_definitions(cyarg DEBUG_LOG_GC)

# Yarg device selection: RASPBERRY_PI_PICO or GENERIC_POSIX_HOST
# determines if cyarg is built as hosted or self-hosted binary
# for Pico, sets the PICO_BOARD variable for the Pico SDK
if (YARG_DEVICE STREQUAL "RASPBERRY_PI_PICO")
set(CYARG_HOSTING "SELF_HOSTED" CACHE STRING "cyarg built as self-hosting binary")
set(CYARG_SDK "PICO_SDK" CACHE STRING "SDK to use for the build")
set(PICO_BOARD "pico" CACHE STRING "Raspberry Pi Pico board")
elseif (YARG_DEVICE STREQUAL "GENERIC_POSIX_HOST")
set(CYARG_HOSTING "HOSTED" CACHE STRING "cyarg built as hosted binary")
set(CYARG_SDK "GENERIC_POSIX_C" CACHE STRING "SDK to use for the build")
endif()

# Project definition, and language toolchain requirements
if (CYARG_SDK STREQUAL "PICO_SDK")
# must be pulled in before the project is defined
include(cyarg_pico_sdk_setup.cmake)
# The PICO SDK will also need CXX & ASM toolchains to assemble the firmware image
project(cyarg VERSION 0.1.0 LANGUAGES C CXX ASM)
elseif (CYARG_SDK STREQUAL "GENERIC_POSIX_C")
project(cyarg VERSION 0.1.0 LANGUAGES C)
endif()

add_executable(cyarg)

# configures defaults for hosting and self hosted builds.
if (CYARG_HOSTING STREQUAL "SELF_HOSTED")
target_compile_definitions(cyarg PRIVATE CYARG_SELF_HOSTED)
target_compile_definitions(cyarg PRIVATE CYARG_FEATURE_SELF_HOSTED_REPL)
set(CYARG_FEATURE_FILESYSTEM "LITTLEFS" CACHE STRING "Filesystem feature to include")
elseif(CYARG_HOSTING STREQUAL "HOSTED")
target_compile_definitions(cyarg PRIVATE CYARG_OS_HOSTED)
target_compile_definitions(cyarg PRIVATE CYARG_FEATURE_HOSTED_REPL)
target_compile_definitions(cyarg PRIVATE CYARG_PTHREADS_SYNC)
set(CYARG_FEATURE_FILESYSTEM "CSTDLIB" CACHE STRING "Filesystem feature to include")
set(CYARG_FEATURE_TEST_SYSTEM "TRUE" CACHE STRING "Include the test system")
endif()

if (CYARG_FEATURE_HEAP STREQUAL "CSTDLIB")
target_compile_definitions(cyarg PRIVATE CYARG_CSTDLIB_HEAP)
endif()

target_sources(cyarg PRIVATE
    main.c
    common.h
    print.h
    chunk.h
    chunk.c
    memory.h
    memory.c
    debug.h
    debug.c
    value.h
    value.c
    value_cell.h
    value_cell.c
    routine.h
    routine.c
    vm.h
    vm.c
    ast.h
    ast.c
    parser.h
    parser.c
    compiler.h
    compiler.c
    scanner.h
    scanner.c
    object.h
    object.c
    object_gc.h
    object_gc.c
    object_gc_impl.h
    object_gc_impl.c
    object_dyn_array.h
    object_dyn_array.c
    heap.h
    heap.c
    vmobject.h
    vmobject.c
    table.h
    table.c
    native.h
    native.c
    builtin.h
    builtin.c
    channel.h
    channel.c
    yargtype.h
    yargstructtype.h
    orderedtreetypes.h
    yargtype.c
    yargobject.h
    yargobject.c
    packed_containers.h
    packed_containers.c
    vm_mutex.h
    vm_mutex.c
    sync_group.h
    sync_group.c
    xip_library.c
    xip_library.h
    fs/fs.h
    big-int/big-int.h
    big-int/big-int.c
    pack_load.c
    pack_format.h
    pack.h
    ../external/o1heap/o1heap/o1heap.c
    ../external/o1heap/o1heap/o1heap.h
    stringstore.h
    stringstore.c
    keyvaluestore.h
    keyvaluestore.c
    )

if (CYARG_HOSTING STREQUAL "HOSTED")
target_sources(cyarg
    PRIVATE
      pack.c
      hosted.c
      hosted.h)
target_sources(cyarg PRIVATE bootstrap/cyarg_hosted_ylib.c)
else()
target_sources(cyarg PRIVATE bootstrap/cyarg_ylib.c)
endif()


if (CYARG_FEATURE_FILESYSTEM STREQUAL "LITTLEFS")
target_sources(cyarg
    PRIVATE
      fs/fs_littlefs.c
      ../external/littlefs/lfs.c
      ../external/littlefs/lfs_util.c)
if (YARG_DEVICE STREQUAL "RASPBERRY_PI_PICO")
target_sources(cyarg
    PRIVATE
      fs/pico_flash_fs.h
      fs/pico_lfs_hal.h
      fs/pico_lfs_hal.c)
endif()
elseif(CYARG_FEATURE_FILESYSTEM STREQUAL "CSTDLIB")
target_sources(cyarg
    PRIVATE
      fs/fs_stdlib.c)
endif()

if (CYARG_FEATURE_TEST_SYSTEM STREQUAL "TRUE")
target_sources(cyarg
    PRIVATE
      test-system/testBuiltin.h
      test-system/testBuiltin.c
      test-system/testSystem.h
      test-system/testSystem.c
      test-system/testIntrinsics.h
      )

target_compile_definitions(cyarg PRIVATE CYARG_FEATURE_TEST_SYSTEM)
endif()

if (CYARG_FEATURE_INTERACTIVE_TRACE STREQUAL "TRUE")
add_compile_definitions(DEBUG_TRACE_EXECUTION)
add_compile_definitions(DEBUG_STRESS_GC)
endif()

if (CYARG_FEATURE_DEBUG_LOG_GC STREQUAL "TRUE")
add_compile_definitions(DEBUG_LOG_GC)
endif()

if (CYARG_SDK STREQUAL "GENERIC_POSIX_C")
# Add the math library for the definition of pow()
target_link_libraries(cyarg m)
elseif(CYARG_SDK STREQUAL "PICO_SDK")
include(cyarg_pico_sdk_targets.cmake)
endif()
