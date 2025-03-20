(import (chicken format)
        (chicken foreign)
        (chicken platform)
        (chicken base)
        matchable)

(define (callseq-define-transformer x r c)
  (let ((%begin (r 'begin))
        (%fd (r 'foreign-declare))
        (%de (r 'define-external)))
    `(,%begin
      (,%fd ,(trampoline x r))
      (,%de ,@(prefix-func x)))))


(define (trampoline x r)
  (let* ((sig (cdr x))
         (name (caar sig))
         (args (cdar sig))
         (retv (cadr sig)))
    (string-join
     (list
      ;; impl prototype
      (format "~a impl_~a(~a);" retv name (convert-args args))

      ;; request struct
      (format "struct req_~a {" name)
      (string-join
       (map (lambda (arg)
              (format "\t~a ~a;" (convert-type (car arg)) (cadr arg)))
            (if (not (equal? retv 'void))
                (cons (list (format "~a *" (convert-type retv)) "ret") args)
                args))
       "\n")
      #;(format "    ~a ~a")
      (format "};")

      ;; trampoline function
      (format "static void tramp_~a(struct req_~a *req) {" name name)
      (format "\t~aimpl_~a(~a);"
              (if (equal? retv 'void)
                  ""
                  (format "*req->ret = "))
              name
              (if (null? args)
                  ""
                  (string-join
                   (map (lambda (arg)
                          (format "req->~a" (cadr arg)))
                        args)
                   ", ")))
      (format "}")

      ;; entry function
      (format "int callseq_invoke(void *func, void *arg);")
      (format "~a ~a(~a) {" retv name (convert-args args))
      (format "\tint err;")
      (when (not (equal? retv 'void))
        (format "\t~a ret;" retv))
      (format "\tstruct req_~a req = {" name)
      (string-join
       (map (lambda (arg)
              (format "\t\t.~a = ~a" (car arg) (cdr arg)))
            (let ((args (map (lambda (arg) (cons (cadr arg) (cadr arg)))
                             args)))
              (if (not (equal? retv 'void))
                  (cons (cons "ret" "&ret") args)
                  args)))
       ",\n")
      (format "\t};")
      (format "\terr = callseq_invoke(&tramp_~a, &req);" name)
      (format "\t if(err) abort();")
      (when (not (equal? retv 'void))
        (format "\treturn ret;"))
      (format "}"))
     "\n")))

(define (string-join strings delimiter)
  (if (null? strings)
      ""
      (let loop ((strings (cdr strings)) (so-far (car strings)))
        (if (null? strings)
            so-far
            (loop (cdr strings)
                  (string-append so-far delimiter (car strings)))))))

(define (convert-type type)
  (match type
         ('int "int")
         ('c-string "char *")
         (_ (format "~a" type))))

(define (convert-args args)
  (string-join
   (map (lambda (arg)
          (format "~a ~a" (convert-type (car arg))
                  (cadr arg)))
        args)
   ", "))

(define (prefix-func x)
  (let* ((sig (cdr x))
         (name-args (car sig))
         (args (cdr name-args))
         (name (car name-args))
         (name (symbol->string name))
         (name (format "impl_~a" name))
         (name (string->symbol name)))
    (cons (cons name args) (cdr sig))))

