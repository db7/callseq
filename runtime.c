#include <chicken.h>
#include <crostini.h>

#define STACK_SIZE (1024 * 1024 * 5)

int calseq_invoke(void (*func)(void *), void *arg);

static crostini_t _cr;
static void _chicken_init(void *_) { CHICKEN_run(C_toplevel); }

void callseq_init(void) {
  // TODO: ensure called once
  void *stack = malloc(STACK_SIZE);
  assert(stack);
  int err = crostini_init(&_cr, stack, STACK_SIZE);
  CROSTINI_ENSURE_OK(err);
  err = crostini_invoke(&_cr, _chicken_init, 0);
  CROSTINI_ENSURE_OK(err);
}

int callseq_invoke(void (*func)(void *), void *arg) {
  int err = crostini_invoke(&_cr, func, arg);
  CROSTINI_ENSURE_OK(err);
  // TODO: return error
  return 0;
}
