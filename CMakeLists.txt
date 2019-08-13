cmake_minimum_required(VERSION 3.10)
project(Little-Racket LANGUAGES C)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message("!! Little Racket (Release mode).")
    add_compile_definitions(RELEASE_MODE)
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("!! Little Racket (Debug mode).")
    add_compile_definitions(DEBUG_MODE)
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Test")
    message("!! Little Racket (Test mode).")
    add_compile_definitions(TEST_MODE)
endif()

list(APPEND my_source_files 
            ./src/main.c
            ./src/load_racket_file.c
            ./src/interpreter.c
            ./src/vector.c
            ./src/custom_handler.c
            ./src/racket_built_in.c)
add_executable(Little-Racket ${my_source_files})

target_link_libraries(Little-Racket m)