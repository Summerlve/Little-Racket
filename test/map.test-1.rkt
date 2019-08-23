#lang racket
(map (lambda (pre nxt lst) (+ pre nxt lst)) '(1 2) '(1 2) '(1 2))