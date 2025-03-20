// clang-format off
#include <stdint.h>
#include <vsync/atomic.h>
#include <string.h>
#include <crostini.h>
#include <chicken.h>
// clang-format on

#include <assert.h>
#include <sys/errno.h>
#include <vsync/spinlock/caslock.h>

extern int bar(int);

void _chicken_init(void *_) { CHICKEN_run(C_toplevel); }

static crostini_t _cr;

#define STACK_SIZE (1024 * 1024 * 5)

void chicken_init() {
  void *stack = malloc(STACK_SIZE);
  assert(stack);
  int err = crostini_init(&_cr, stack, STACK_SIZE);
  CROSTINI_ENSURE_OK(err);
  err = crostini_invoke(&_cr, _chicken_init, 0);
  CROSTINI_ENSURE_OK(err);
}

extern int foo(int x);

struct dispatch {
  int arg;
  int ret;
};

static void _dispatch(void *arg) {
  struct dispatch *job = arg;
  job->ret = bar(job->arg);
}

#define INVOKE(job)                                                            \
  do {                                                                         \
    int err = crostini_invoke(&_cr, _dispatch, job);                           \
    CROSTINI_ENSURE_OK(err);                                                   \
  } while (0)

int foo(int x) {
  struct dispatch job = (struct dispatch){.arg = x, .ret = 0};
  INVOKE(&job);
  return job.ret;
}
