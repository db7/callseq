/* Copyright (c) 2025 Diogo Behrens
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * -----------------------------------------------------------------------------
 * crostini --- flat-combining coroutine
 * version: 0.1
 *
 * Crostini is a flat-combining mechanism to sequentially execute concurrent
 * requests of multiple threads in a dedicated "coroutine".  When a thread
 * invokes a function with crostini, it effectively enqueues the invocation as a
 * request, which is then executed by one of the threads currently invoking the
 * same crostini object.  The execution of the requests is performed with a
 * separate stack.
 *
 */
#ifndef _CROSTINI_H_
#define _CROSTINI_H_

#include <crouton.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef VSYNC_VERIFICATION
#include <assert.h>
#define perror(msg) assert(0 && (msg))
#endif

#include <vsync/spinlock/caslock.h>

typedef void (*crostini_f)(void *);

typedef struct crostini_node {
  void *arg;
  crostini_f func;
  vatomicptr_t next;
  vatomic32_t grant;
} crostini_node;

typedef struct {
  crouton_t cr;
  void *stack;

  vatomicptr_t tail;
  crostini_node *cur;

} crostini_t;

static uint32_t _crostini_wait(crostini_node *node) {
  return vatomic32_await_neq(&node->grant, 0);
}

static void _crostini_wake(crostini_node *node, uint32_t grant) {
  vatomic32_write(&node->grant, grant);
}

static void _crostini_combine(crostini_t *cr, crostini_node *node) {
  while (1) {
    node->func(node->arg);

    crostini_node *next = vatomicptr_read(&node->next);
    if (next == NULL || 0) {
      cr->cur = node;
      return;
    } else {
      _crostini_wake(node, 2);
      node = next;
    }
  }
}

static void _crostini_loop(crouton_t *c) {
  crostini_t *cr = (crostini_t *)c;
  int err;
  do {
    _crostini_combine(cr, cr->cur);
  } while ((err = crouton_yield(c)) == CROUTON_AGAIN);

  if (err == CROUTON_ERROR)
    abort();
}

static int crostini_init(crostini_t *cr, void *stack, size_t size) {
  cr->stack = stack;
  cr->cur = NULL;
  vatomicptr_init(&cr->tail, NULL);
  return crouton_init(&cr->cr, _crostini_loop, stack, size);
}

static void *crostini_fini(crostini_t *cr) {
  void *stack = cr->stack;
  crouton_fini(&cr->cr);
  cr->stack = NULL;
  return stack;
}

static int crostini_invoke(crostini_t *cr, crostini_f func, void *arg) {
  int err;

  crostini_node node = {.arg = arg,
                        .func = func,
                        .grant = VATOMIC_INIT(0),
                        .next = VATOMIC_INIT(0)};

  /* add node to queue */
  crostini_node *prev = vatomicptr_xchg(&cr->tail, &node);
  if (prev != NULL) {
    /* if there is a previous node, we enqueue our node to the previous */
    vatomicptr_write(&prev->next, &node);

    /* and wait for a signal a la MCS. If task already served, return */
    if (_crostini_wait(&node) == 2) {
      return 0; // done
    }
  }

  /* it's our turn. Save node in cur pointer and invoke function call. */
  cr->cur = &node;
  err = crouton_invoke(&cr->cr);

  /* wake up next */
  prev = cr->cur;

  crostini_node *next = vatomicptr_read(&prev->next);
  if (next == NULL) {
    next = vatomicptr_cmpxchg(&cr->tail, prev, NULL);
    if (next == prev)
      goto end;
    next = vatomicptr_await_neq(&prev->next, 0);
  }
  _crostini_wake(next, 1);

end:
  if (prev != &node)
    _crostini_wake(prev, 2);
  return err;
}

#define CROSTINI_ENSURE_OK(err)                                                \
  if ((err) != 0 && (err) != CROUTON_OVER) {                                   \
    perror("crostini not ok");                                                 \
    abort();                                                                   \
  }

#endif /* _CROSTINI_H_ */
