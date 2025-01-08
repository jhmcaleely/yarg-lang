#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//#define NAN_BOXING
//#define DEBUG_PRINT_CODE
//#define DEBUG_TRACE_EXECUTION

//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC

// Use a main() that only hosts a repl over stdin/stdout
// Useful on microcontroller boards that have no filesystem, but 
// have a serial terminal (somehow)
//#define HOST_REPL

// Set if we are building in the PICO SDK.
//#define LOX_PICO_SDK

#define UINT8_COUNT (UINT8_MAX + 1)

#endif