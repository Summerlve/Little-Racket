#lang racket
(define a '(11 2.2 233 "abcd" "123abc"))
(define v '(#t #f)); comment here
(define c 2)
a
v
c
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
(define closure (lambda (x) x))
(closure 1)
(+ 1 (+ 1 2))
(= 1 1.0 1.0000) 
(= 1 2 3)
(= 1 2)
(= (+ 1) (+ 1.2))
(define fact 
      (lambda (val)
            (if (= val 1) 1
                  (* val (fact (- val 1))))))
(fact 5)
(define is-one 
      (lambda (val) (if (= val 1) #t #f)))
(is-one 1.1)
(map is-one '(1 2 3))
'()
(map is-one '())