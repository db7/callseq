(import (chicken platform)
        (chicken foreign)
        (chicken base)
        (chicken format)
        (concurrent-native-callbacks))

(define-concurrent-native-callback
 (foo (integer x))
 (+ x 100))


(define-external (runloop) void
                 (dispatch))

;;(foreign-declare #<<EOF
;;void runloop(void);
;;static void *start_thread(void *arg)
;;{
;;    runloop();
;;    return 0;
;;}
;;
;;EOF
;;)
;;
;;(foreign-code #<<EOF
;;      pthread_t t1;
;;      pthread_create(&t1, NULL, start_thread, NULL);
;;EOF
;;)
(return-to-host)
