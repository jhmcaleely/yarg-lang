cmake_minimum_required(VERSION 3.10.0)
project(clox VERSION 0.1.0 LANGUAGES C)

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
