(module callseq
        (define-sequenced-callback)
        (import scheme (chicken base))
        (import matchable
                (srfi-1)
                (chicken base)
                (chicken platform)
                (chicken format)
                (chicken syntax)
                (chicken foreign))
        (begin
          (begin-for-syntax
           (include "comptime.scm"))
          (define-syntax define-sequenced-callback
            (er-macro-transformer
             (lambda (x r c)
               (callseq-define-transformer x r c))))))
