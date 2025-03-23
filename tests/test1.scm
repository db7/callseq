(import (chicken platform)
        (chicken format)
        (chicken foreign)
        foreigners
        callseq)

(define-sequenced-callback (foo (int a)) integer
                           (* a a))

(define-sequenced-callback (bar (c-string msg)) void
                           (printf "Hello ~a!~n" msg))

(define-foreign-record-type plan
                            (int next plan-next plan-next!)
                            (c-string msg plan-msg plan-msg!))

(define-sequenced-callback (baz ((c-pointer (struct plan)) p)) void
                           (printf "plan->next = ~a~n" (plan-next p))
                           (printf "plan->msg  = ~a~n" (plan-msg p)))


(define-sequenced-callback (bap (plan p)) void
                           (printf "plan->next = ~a~n" (plan-next p))
                           (printf "plan->msg  = ~a~n" (plan-msg p)))


(define-foreign-enum-type
 (event unsigned-integer32)
 (event->int int->event)
 ((evx event/x) EV_X)
 ((evy event/y) EV_Y)
 ((evz event/z) EV_Z)
 )

(define-foreign-record-type stuff ;; "struct stuff")
                            (int abc stuff-abc)
                            (event ev stuff-ev))

(define-sequenced-callback (bat ((c-pointer (struct stuff)) s1)
                                (stuff s2)
                                (event ev)
                                ) void
                           (printf "s1->abc = ~a~n" (stuff-abc s1))
                           (printf "s1->ev  = ~a~n" (stuff-ev s1))
                           (printf "s2->abc = ~a~n" (stuff-abc s2))
                           (printf "s2->ev  = ~a~n" (stuff-ev s2))
                           (printf "event   = ~a~n" ev))



#>
#include <assert.h>
//struct stuff {
//int abc;
//};
void callseq_init(void);
int foo(int x);
void bar(char *x);
void baz(struct plan *x);
void bap(struct plan *x);

#define EV_X 1
#define EV_Y 2
#define EV_Z 3

void bat(struct stuff *x, struct stuff *y, uint32_t e);

int main(void) {
callseq_init();
int r = foo(4);
assert(r == 16);

bar("world");
baz(&(struct plan){.msg = "hello world", .next = 123});
bap(&(struct plan){.msg = "hello world", .next = 123});

bat(
    &(struct stuff){.abc = 111, .ev = EV_X},
    &(struct stuff){.abc = 222, .ev = EV_Y},
    EV_Z
    );
return 0;
}

<#

(return-to-host)
