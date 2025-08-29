set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cvalue LANGUAGES C CXX ASM)


add_executable(cvalue 
    value_main.c)


add_compile_definitions(NAN_BOXING)

# Add the standard library to the build
target_link_libraries(cvalue)

# Add the standard include files to the build
target_include_directories(cvalue PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)
