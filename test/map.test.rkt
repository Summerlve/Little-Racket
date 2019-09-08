#lang racket
(map (lambda (pre nxt lst) (+ pre nxt lst)) '(1 2) '(1 2) '(1 2))
(define plus (lambda (pre nxt lst) (+ pre nxt lst)))
(map plus '(1 2 3) '(1 2 3) '(1 2 3))