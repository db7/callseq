(import (chicken platform)
        callseq)

(define-sequenced-callback (foo (int a)) int
                           (+ a 123))

(return-to-host)
