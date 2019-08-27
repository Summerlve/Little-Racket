# Little-Racket

## Target: ##

1. variable definition and setting：x
  
    DataTypes: Numbers、Strings、Characters、Lists、Pairs.

    e.g:

    (define x 1)

    (define x '(1 . 2))

    (define x '(1 2 3))

    (set! x 1)

    (define x 'a)

2. procedure definition(normally argument)：(lambda (x) expr)

    e.g: (define x (lambda (arg) expr))
3. binding:
   1. (let ([x v]) expr)
   2. (let* ...)
   3. (letrec ...)
4. calling：(proc arg ...)
5. calculate：(+-*/ expr ...)
6. Built-in procdures: (list), (cons), (list?), (pair?), (map), (filter), (car), (cdr), (=) (>) (<) for Number
7. Conditional Form: (if), (cond), (and), (or), (not)

---

## DLC: ##

### For the truth  - 2019.10 ###

1. Racket null, empty
2. ((lambda (...) ...) ...), procedure call
3. (>=), (<=)

---

## Using cmake to build this project. ##

### Compile the project: ###
```bash
> cd <path_to_the_project>
> chmod 744 ./build.sh 
> sudo ./build.sh
> cd ./build
# you can find executable file named "Little-Racket" in current folder.
> ./Little-Racket <path_to_racket_file>
> ......
```

---

## Workflow ##

Raw code

-> tokens (tokenizer)

-> Racket type AST (parser)

-> result (calculator)