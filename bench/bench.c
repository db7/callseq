#include "now.h"
#include <assert.h>
#include <chicken.h>
#include <pthread.h>
#include <stdio.h>

#define SAMPLES 1000000UL

extern int foo(int);

struct samples {
  size_t count;
  nanosec_t ts[];
};

const char *name;

void *run(void *arg) {
  sleep(2); // to ensure cncb-dispatch thread has started

  size_t count = SAMPLES;

  nanosec_t ts = now();
  while (count--) {
    (void)foo(count);
  }
  nanosec_t then = now();
  printf("%20s\ttput: %.2fop/s\tlatency: %0.0fns/op\n", name + 8,
         SAMPLES / in_sec(then - ts), 1.0 * (then - ts) / SAMPLES);
  exit(0);

  return 0;
}

#ifdef CNCB

void runloop(void);

int main(int argc, char *argv[]) {
  /* cannot start this in another thread, so I inverted the roles: main thread
   * is the Scheme thread. */
  name = argv[0];
  CHICKEN_run(C_toplevel);
  pthread_t t1;
  pthread_create(&t1, NULL, run, NULL);
  runloop();
  return 0;
}
#elif defined(CALLSEQ)

void callseq_init();
int main(int argc, char *argv[]) {
  name = argv[0];
  callseq_init();
  (void)run(0);
  return 0;
}

#else
__attribute__((weak)) void chicken_init() { CHICKEN_run(C_toplevel); }
int main(int argc, char *argv[]) {
  name = argv[0];
  chicken_init();
  (void)run(0);
  return 0;
}
#endif
