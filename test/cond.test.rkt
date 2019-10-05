#lang racket
(cond
    [else 5]) ; 5
(cond
    [(= 1 2) 1 2 "is-one"]
    [else 6]) ; 6
(define fib (lambda (index)
                (cond [(= index 1) 1] 
                      [(= index 2) 1]
                      [else
                        (+ (fib (- index 1)) (fib (- index 2)))])))
(fib 19) ; 4181