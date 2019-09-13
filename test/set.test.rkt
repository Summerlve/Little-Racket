#lang racket
(define a 1)
(set! a 2)
a
(set! a (lambda (x) x))
a
(a 1)
(set! a (lambda (index)
                (if (= index 1) 1 
                     (if (= index 2) 1
                        (+ (a (- index 1)) (a (- index 2)))))))
(a 6)