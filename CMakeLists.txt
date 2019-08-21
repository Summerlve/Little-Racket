cmake_minimum_required(VERSION 3.14)
project(Little-Racket LANGUAGES C)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(NOT WIN32)
        string(ASCII 27 Esc)
        set(ColourReset "${Esc}[m")
        set(Green "${Esc}[32m")
    endif()
    message("${Green}!! Little Racket (Release mode).${ColourReset}")
    add_compile_definitions(RELEASE_MODE)
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    if(NOT WIN32)
        string(ASCII 27 Esc)
        set(ColourReset "${Esc}[m")
        set(Red "${Esc}[31m")
    endif()
    message("${Red}!! Little Racket (Debug mode).${ColourReset}")
    add_compile_definitions(DEBUG_MODE)
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Test")
    if(NOT WIN32)
        string(ASCII 27 Esc)
        set(ColourReset "${Esc}[m")
        set(Yellow "${Esc}[33m")
    endif()
    message("${Yellow}!! Little Racket (Test mode).${ColourReset}")
    add_compile_definitions(TEST_MODE)

    # test
    include(CTest)

    add_test(factorial-test Little-Racket ../test/fact.test.rkt)
    set_tests_properties (factorial-test PROPERTIES PASS_REGULAR_EXPRESSION "479001600")

    add_test(fibonacci-test Little-Racket ../test/fib.test.rkt)
    set_tests_properties (fibonacci-test PROPERTIES PASS_REGULAR_EXPRESSION "21")

    add_test(calling-test Little-Racket ../test/calling.test.rkt)
    set_tests_properties (calling-test PROPERTIES PASS_REGULAR_EXPRESSION "1")

    add_test(define-lambda-test Little-Racket ../test/define-lambda.test.rkt)
    set_tests_properties (define-lambda-test PROPERTIES PASS_REGULAR_EXPRESSION "#<procedure:fn>")

    add_test(define-test Little-Racket ../test/define.test.rkt)
    set_tests_properties (define-test PROPERTIES PASS_REGULAR_EXPRESSION "101")

    add_test(let-test Little-Racket ../test/let.test.rkt)
    set_tests_properties (let-test PROPERTIES PASS_REGULAR_EXPRESSION "6")

    add_test(let*-test Little-Racket ../test/let*.test.rkt)
    set_tests_properties (let*-test PROPERTIES PASS_REGULAR_EXPRESSION "3")
    
    add_test(letrec-test Little-Racket ../test/letrec.test.rkt)
    set_tests_properties (letrec-test PROPERTIES PASS_REGULAR_EXPRESSION "#<procedure:access-binding-itself>")
    
    add_test(list-test Little-Racket ../test/list.test.rkt)
    set_tests_properties (list-test PROPERTIES PASS_REGULAR_EXPRESSION " '( 1  2.2  3 \"list\"  #t  #f ) ")
    
    add_test(pair-test Little-Racket ../test/pair.test.rkt)
    set_tests_properties (pair-test PROPERTIES PASS_REGULAR_EXPRESSION " '( 1  .  2.2 ) ")
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

