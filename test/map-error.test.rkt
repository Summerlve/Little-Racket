#lang racket
(define is-one 
      (lambda (val) (if (= val 1) #t #f)))
(is-one 1.1)
(map is-one '() '())