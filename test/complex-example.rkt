#lang racket
(define a '(11 2.2 233 '(1 . 2) "abcd" "123abc"))
(define v '(#\c #\a #t #f)); comment here
(define c 2)
(let ([a (let ([d (/ (+ 1 100) 5)]) d)]
      [b (let* ([e (letrec ([f 1.5]) f)]) e)]
      [c 3]) 1 (+ a b) (+ a (+ a b) c) #t (+ -11911.11 11.11)); let expression
(define plus (lambda (x y) (+ x y)))
(plus 101 12)
(plus 2 (plus 1 (plus (plus 81 2) (plus c 11))))
(define x (lambda (x y z) x))
(x 1 2 3)
(if a 1 2)
(define p (- (+ 1 2.1) 0.1))
(+ p)
p
#t
#f
(if (if a 1 2) (+ 1 2.0) #f)
(+ 1 2)
1
(if (+ 1 2) #\a #\b)
(define closure (lambda (x) x a))
(closure 1)
(+ 1 (+ 1 2))