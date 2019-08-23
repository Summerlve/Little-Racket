#lang racket
(define plus (lambda (pre nxt lst) (+ pre nxt lst)))
(map plus '(1 2 3) '(1 2 3) '(1 2 3))