#lang racket
(define a '(11 2.2 233 '(1 . 2) "abcd" "123abc"))
(define v '(#\c #\a #t #f)); comment here
(let ([a (let ([d (/ (+ 1 100) 5)]) d)]
      [b (let* ([e (letrec ([f 1.5]) f)]) e)]
      [c 3]) 1 (+ a b) (+ a (+ a b) c) #t); let expression