cmake_minimum_required(VERSION 3.14)
project(Little-Racket LANGUAGES C)

# Release mode
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    if(NOT WIN32)
        string(ASCII 27 Esc)
        set(ColourReset "${Esc}[m")
        set(Green "${Esc}[32m")
    endif()
    message("${Green}!! Little Racket (Release mode)${ColourReset}")
    add_compile_definitions(RELEASE_MODE)

# Install mode
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Install")
    if(NOT WIN32)
        string(ASCII 27 Esc)
        set(ColourReset "${Esc}[m")
        set(Blue "${Esc}[34m")
    endif()
    message("${Blue}!! Little Racket (Install mode)${ColourReset}")
    add_compile_definitions(RELEASE_MODE) # first release then install
    set(CMAKE_INSTALL_PREFIX /usr/local) # This variable defaults to /usr/local in cmake
    set(z_install_dest bin)
    message("${Blue}!! Default install to ${CMAKE_INSTALL_PREFIX}/${z_install_dest}${ColourReset}")

# Debug mode
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    if(NOT WIN32)
        string(ASCII 27 Esc)
        set(ColourReset "${Esc}[m")
        set(Red "${Esc}[31m")
    endif()
    message("${Red}!! Little Racket (Debug mode)${ColourReset}")
    add_compile_definitions(DEBUG_MODE)

# Test mode
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Test")
    if(NOT WIN32)
        string(ASCII 27 Esc)
        set(ColourReset "${Esc}[m")
        set(Yellow "${Esc}[33m")
    endif()
    message("${Yellow}!! Little Racket (Test mode)${ColourReset}")
    add_compile_definitions(TEST_MODE)

    # test
    include(CTest)

    add_test(factorial-test ${PROJECT_NAME} ../test/fact.test.rkt)
    set_tests_properties(factorial-test PROPERTIES PASS_REGULAR_EXPRESSION "87178291200")

    add_test(fibonacci-test ${PROJECT_NAME} ../test/fib.test.rkt)
    set_tests_properties(fibonacci-test PROPERTIES PASS_REGULAR_EXPRESSION "2584")

    add_test(calling-test ${PROJECT_NAME} ../test/calling.test.rkt)
    set_tests_properties(calling-test PROPERTIES PASS_REGULAR_EXPRESSION "1")

    add_test(define-lambda-test ${PROJECT_NAME} ../test/define-lambda.test.rkt)
    set_tests_properties(define-lambda-test PROPERTIES PASS_REGULAR_EXPRESSION
"#<procedure:fn>[\r\n\t ]*\
#<procedure>[\r\n\t ]*\
1"
    )

    add_test(define-test ${PROJECT_NAME} ../test/define.test.rkt)
    set_tests_properties(define-test PROPERTIES PASS_REGULAR_EXPRESSION "101")

    add_test(let-test ${PROJECT_NAME} ../test/let.test.rkt)
    set_tests_properties(let-test PROPERTIES PASS_REGULAR_EXPRESSION "6")

    add_test(let*-test ${PROJECT_NAME} ../test/let*.test.rkt)
    set_tests_properties(let*-test PROPERTIES PASS_REGULAR_EXPRESSION "3")
    
    add_test(letrec-test ${PROJECT_NAME} ../test/letrec.test.rkt)
    set_tests_properties(letrec-test PROPERTIES PASS_REGULAR_EXPRESSION "#<procedure:access-binding-itself>")
    
    add_test(list-test ${PROJECT_NAME} ../test/list.test.rkt)
    set_tests_properties(list-test PROPERTIES PASS_REGULAR_EXPRESSION "'\\(1 2.2 3 \"list\" #t #f\\)")
    
    add_test(pair-test ${PROJECT_NAME} ../test/pair.test.rkt)
    set_tests_properties(pair-test PROPERTIES PASS_REGULAR_EXPRESSION "'\\(1 . 2.2\\)")

    add_test(complex-example-test ${PROJECT_NAME} ../test/complex-example.rkt)
    set_tests_properties(complex-example-test PROPERTIES PASS_REGULAR_EXPRESSION
"'\\(11 2.2 233 \"abcd\" \"123abc\"\\)[\r\n\t ]*\
'\\(#t #f\\)[\r\n\t ]*\
2[\r\n\t ]*\
-11900.000000[\r\n\t ]*\
113[\r\n\t ]*\
99[\r\n\t ]*\
1[\r\n\t ]*\
1[\r\n\t ]*\
3.000000[\r\n\t ]*\
3.000000[\r\n\t ]*\
#t[\r\n\t ]*\
#f[\r\n\t ]*\
3.000000[\r\n\t ]*\
3[\r\n\t ]*\
1[\r\n\t ]*\
1[\r\n\t ]*\
4[\r\n\t ]*\
#t[\r\n\t ]*\
#f[\r\n\t ]*\
#f[\r\n\t ]*\
#f[\r\n\t ]*\
120[\r\n\t ]*\
#f[\r\n\t ]*\
'\\(#t #f #f\\)[\r\n\t ]*\
'\\(\\)[\r\n\t ]*\
'\\(\\)[\r\n\t ]*\
#f[\r\n\t ]*\
#<procedure:=>[\r\n\t ]*\
#<procedure:>=>"
    )

    add_test(map-test ${PROJECT_NAME} ../test/map.test.rkt)
    set_tests_properties(map-test PROPERTIES PASS_REGULAR_EXPRESSION
"'\\(3 6\\)[\r\n\t ]*\
'\\(3 6 9\\)"
    )

    add_test(is-list-test ${PROJECT_NAME} ../test/is-list.test.rkt)
    set_tests_properties(is-list-test PROPERTIES PASS_REGULAR_EXPRESSION
"#t[\r\n\t ]*\
#f"
    )

    add_test(is-pair-test ${PROJECT_NAME} ../test/is-pair.test.rkt)
    set_tests_properties(is-pair-test PROPERTIES PASS_REGULAR_EXPRESSION
"#f[\r\n\t ]*\
#t"
    )

    add_test(is-list-error-test ${PROJECT_NAME} ../test/is-list-error.test.rkt)
    set_tests_properties(is-list-error-test PROPERTIES PASS_REGULAR_EXPRESSION
"list\\?: arity mismatch\;[\r\n\t ]*\
the expected number of arguments does not match the given number[\r\n\t ]*\
expected: 1[\r\n\t ]*\
given: 0[\r\n\t ]*"
    )

    add_test(filter-test ${PROJECT_NAME} ../test/filter.test.rkt)
    set_tests_properties(filter-test PROPERTIES PASS_REGULAR_EXPRESSION "'\\(1\\)")

    add_test(and-test ${PROJECT_NAME} ../test/and.test.rkt)
    set_tests_properties(and-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#t[\r\n\t ]*\
1[\r\n\t ]*\
#f[\r\n\t ]*\
1[\r\n\t ]*\
#t"
    )

    add_test(not-test ${PROJECT_NAME} ../test/not.test.rkt)
    set_tests_properties(not-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#f[\r\n\t ]*\
#t[\r\n\t ]*\
#f"
    )

    add_test(or-test ${PROJECT_NAME} ../test/or.test.rkt)
    set_tests_properties(or-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#f[\r\n\t ]*\
1[\r\n\t ]*\
#t"
    )

    add_test(number-equal-test ${PROJECT_NAME} ../test/number-equal.test.rkt)
    set_tests_properties(number-equal-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#t[\r\n\t ]*\
#t[\r\n\t ]*\
#f"
    )

    add_test(number-more-than-test ${PROJECT_NAME} ../test/number-more-than.test.rkt)
    set_tests_properties(number-more-than-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#t[\r\n\t ]*\
#f[\r\n\t ]*\
#f"
    )

    add_test(number-less-than-test ${PROJECT_NAME} ../test/number-less-than.test.rkt)
    set_tests_properties(number-less-than-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#t[\r\n\t ]*\
#t[\r\n\t ]*\
#f[\r\n\t ]*\
#t[\r\n\t ]*\
#f"
    )

    add_test(car-test ${PROJECT_NAME} ../test/car.test.rkt)
    set_tests_properties(car-test PROPERTIES PASS_REGULAR_EXPRESSION 
"1[\r\n\t ]*\
\"name\"[\r\n\t ]*\
2"
    )

    add_test(cdr-test ${PROJECT_NAME} ../test/cdr.test.rkt)
    set_tests_properties(cdr-test PROPERTIES PASS_REGULAR_EXPRESSION 
"'\\(2\\)[\r\n\t ]*\
'\\(\\)[\r\n\t ]*\
6"
    )

    add_test(proc-list-test ${PROJECT_NAME} ../test/proc-list.test.rkt)
    set_tests_properties(proc-list-test PROPERTIES PASS_REGULAR_EXPRESSION 
"'\\(1 2 3\\)[\r\n\t ]*\
'\\(1 2 3 4\\)[\r\n\t ]*\
'\\(\\(1 2\\) \\(3 4\\)\\)"
    )

    add_test(proc-cons-test ${PROJECT_NAME} ../test/proc-cons.test.rkt)
    set_tests_properties(proc-cons-test PROPERTIES PASS_REGULAR_EXPRESSION 
"'\\(1 . 2\\)[\r\n\t ]*\
'\\(1\\)[\r\n\t ]*\
'\\(\\(1 2 3\\) 4\\)"
    )

    add_test(set-test ${PROJECT_NAME} ../test/set.test.rkt)
    set_tests_properties(set-test PROPERTIES PASS_REGULAR_EXPRESSION 
"2[\r\n\t ]*\
#<procedure:a>[\r\n\t ]*\
1[\r\n\t ]*\
8"
    )

    add_test(map-error-test ${PROJECT_NAME} ../test/map-error.test.rkt)
    set_tests_properties(map-error-test PROPERTIES PASS_REGULAR_EXPRESSION 
"map\: argument mismatch\;[\r\n\t ]*\
the given procedure's expected number of arguments does not match the given number of lists[\r\n\t ]*\
expected: 1[\r\n\t ]*\
given: 2"
    )

    add_test(negative-number-test ${PROJECT_NAME} ../test/negative-number.test.rkt)
    set_tests_properties(negative-number-test PROPERTIES PASS_REGULAR_EXPRESSION 
"-199.98[\r\n\t ]*\
-45"
    )

    add_test(number-less-or-equal-than-test ${PROJECT_NAME} ../test/number-less-or-equal-than.test.rkt)
    set_tests_properties(number-less-or-equal-than-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#t[\r\n\t ]*\
#t[\r\n\t ]*\
#f[\r\n\t ]*\
#t"
    )

    add_test(number-more-or-equal-than-test ${PROJECT_NAME} ../test/number-more-or-equal-than.test.rkt)
    set_tests_properties(number-more-or-equal-than-test PROPERTIES PASS_REGULAR_EXPRESSION 
"#t[\r\n\t ]*\
#t[\r\n\t ]*\
#t[\r\n\t ]*\
#f"
    )

    add_test(cond-test ${PROJECT_NAME} ../test/cond.test.rkt)
    set_tests_properties(cond-test PROPERTIES PASS_REGULAR_EXPRESSION 
"9[\r\n\t ]*\
6[\r\n\t ]*\
4181"
    )

    add_test(null-empty-test ${PROJECT_NAME} ../test/null-empty.test.rkt)
    set_tests_properties(null-empty-test PROPERTIES PASS_REGULAR_EXPRESSION 
"'\\(\\)[\r\n\t ]*\
'\\(\\)[\r\n\t ]*\
'\\(1 \\(\\)\\)"
    )

    add_test(sha256-test ${PROJECT_NAME} ../test/sha256.test.rkt)
    set_tests_properties(sha256-test PROPERTIES PASS_REGULAR_EXPRESSION
"\"881dad820d90a1ee555a48ac9ab322dda62914143c96a505dca9f7b17f386904\"[\r\n\t ]*\
#<procedure:string-sha256>"
    )

    add_test(non-argu-fn-call-test ${PROJECT_NAME} ../test/non-argu-fn-call.test.rkt)
    set_tests_properties(non-argu-fn-call-test PROPERTIES PASS_REGULAR_EXPRESSION
"\"abcd\"[\r\n\t ]*\
1[\r\n\t ]*\
\"ngfct\""
    )
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(Sodium REQUIRED)
include_directories(${sodium_INCLUDE_DIR})
aux_source_directory(./src my_source_files)
add_executable(${PROJECT_NAME} ${my_source_files})
target_link_libraries(${PROJECT_NAME} m ${sodium_LIBRARY_RELEASE})

# Install mode
if (${CMAKE_BUILD_TYPE} STREQUAL "Install")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${z_install_dest})
endif()