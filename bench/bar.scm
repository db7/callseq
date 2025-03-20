(import (chicken platform)
        (chicken foreign)
        (chicken base)
        (chicken format))

(define-external (bar (integer x)) int
    (+ x 100))

(return-to-host)
