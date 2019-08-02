#lang racket
(define a '(11 2.2 233 '(1 . 2) "abcd" "123abc"))
(define v '(#\c #\a #t #f)); comment here
(define c 1)
(let ([a (let ([d (/ (+ 1 100) 5)]) d)]
      [b (let* ([e (letrec ([f 1.5]) f)]) e)]
      [c 3]) 1 (+ a b) (+ a (+ a b) c) #t (+ -11911.11 11.11)); let expression
(define plus (lambda (x y) 1 (+ x y)))
(plus 1 2)
(plus 2 (plus 1 (plus (plus 1 2) (plus c 1))))
(define x (lambda (x y z) x))