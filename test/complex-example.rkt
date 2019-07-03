#lang racket
(define a '(11 2.2 233 '(1 . 2) "abcd" "123abc"))
(define v '(#\c #\a)); comment here
(let ([a 1][b 2][c 3]) (+ a b) (+ a b c)); let expression