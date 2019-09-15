#lang racket
(define fib (lambda (index)
                (if (= index 1) 1 
                     (if (= index 2) 1
                        (+ (fib (- index 1)) (fib (- index 2)))))))
(fib 18) ; 2584