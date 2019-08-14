#lang racket
(letrec ([fact (lambda () fact)]) fact)