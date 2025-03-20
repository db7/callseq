(import (chicken platform)
        callseq)

(define-sequenced-callback (foo (int a)) int
                           (* a a))
(return-to-host)
