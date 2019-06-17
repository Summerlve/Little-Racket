cmake_minimum_required(VERSION 3.10)
project(Little-Racket LANGUAGES C)
list(APPEND my_source_files 
            ./src/main.c
            ./src/load_racket_file.c
            ./src/interpreter.c
            ./src/vector.c)
add_executable(Little-Racket ${my_source_files})