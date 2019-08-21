#lang racket
(define fact 
      (lambda (val)
            (if (= val 1) 1
                  (* val (fact (- val 1))))))
(fact 12) ;479001600