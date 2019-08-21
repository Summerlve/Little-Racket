#lang racket
(letrec ([one 1]
         [access-binding-itself (lambda () one access-binding-itself)])
            (access-binding-itself)) 