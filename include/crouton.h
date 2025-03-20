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
 * crouton --- simple ucontext coroutines
 * version: 0.1
 *
 * Crouton is a simple ucontext-based coroutine implementation.  A coroutine
 * invokes a function with its own stack (not the thread stack). The coroutine
 * can be interrupted by calling `crouton_yield` and if a thread invokes the
 * coroutine again, the computation proceeds from the yield call. Note that
 * crouton is not thread-safe, so you have to protect its interface with a lock.
 *
 * Caveats:
 *
 * - Windows not supported.
 * - ucontext deprecated on macOS, use -Wno-deprecated-declarations
 *
 * References:
 *
 * Inspired by
 * https://probablydance.com/2012/11/18/implementing-coroutines-with-ucontext
 *
 */
#ifndef _CROUTON_H_
#define _CROUTON_H_
#include <stddef.h>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

/* only one of the these options should be set:
 * - CROUTON_DISABLE
 * - CROUTON_UCONTEXT
 * - CROUTON_ASSEMBLY
 */

/* When verifying the sequencers using crouton, disable crouton */
#ifdef VSYNC_VERIFICATION
#undef CROUTON_UCONTEXT
#undef CROUTON_ASSEMBLY
#define CROUTON_DISABLE
#endif

#if !defined(CROUTON_DISABLE) && !defined(CROUTON_UCONTEXT)
#define CROUTON_ASSEMBLY
#endif

#if defined(CROUTON_ASSEMBLY) && !defined(__aarch64__)
#undef CROUTON_ASSEMBLY
#define CROUTON_UCONTEXT
#endif

#if defined(CROUTON_UCONTEXT)
#include <ucontext.h>
#define crouton_ctx ucontext_t

#elif defined(CROUTON_ASSEMBLY)
#include <stdint.h>
#if defined(__aarch64__)
typedef struct {
  uintptr_t x19, x20;
  uintptr_t x21, x22;
  uintptr_t x23, x24;
  uintptr_t x25, x26;
  uintptr_t x27, x28;
  uintptr_t x29, x30;
  uintptr_t d8, d9;
  uintptr_t d10, d11;
  uintptr_t d12, d13;
  uintptr_t d14, d15;
  uintptr_t sp, unused;
} crouton_ctx;
void crouton_swap(crouton_ctx *caller, crouton_ctx *calee, void *arg);
#else
#error "assembly sequence for architecture not available"
#endif /* defined(__aarch64__) */
#else  /* defined(CROUTON_DISABLE) */
#define crouton_ctx int
#endif /* defined(CROUTON_UCONTEXT) */

typedef struct crouton crouton_t;
typedef void (*crouton_f)(crouton_t *);

struct crouton {
  crouton_f func;
  crouton_ctx caller;
  crouton_ctx callee;
};

#define CROUTON_OK 0
#define CROUTON_DONE 1
#define CROUTON_OVER 2
#define CROUTON_AGAIN 3
#define CROUTON_ERROR -1

static int crouton_init(crouton_t *c, crouton_f func, void *sp, size_t sz);
static void crouton_fini(crouton_t *c);
static int crouton_invoke(crouton_t *c);
static int crouton_yield(crouton_t *c);

/******************************************************************************/

#include <assert.h>
#include <stdint.h>
#define CROUTON_MASK (((uintptr_t)1 << 32) - 1)
#define CROUTON_HGH(ptr) ((uint32_t)((uintptr_t)(ptr) >> 32))
#define CROUTON_LOW(ptr) ((uint32_t)((uintptr_t)(ptr) & CROUTON_MASK))
#define CROUTON_PTR(hgh, low)                                                  \
  ((crouton_t *)(((uintptr_t)(hgh) << 32) | ((uintptr_t)(low) & CROUTON_MASK)))
#define CROUTON_ALIGNUP(addr, align)                                           \
  ((void *)(((uintptr_t)(addr) + ((align) - 1)) & ~((align) - 1)))

static void _crouton_do_ptr(crouton_t *c) {
  assert(c);
  assert(c->func);
  c->func(c);
  c->func = NULL;
}

static void _crouton_do(uint32_t high, uint32_t low) {
  crouton_t *c = CROUTON_PTR(high, low);
  _crouton_do_ptr(c);
}

int crouton_init(crouton_t *c, crouton_f func, void *sp, size_t sz) {
  c->func = func;

  void *spa = CROUTON_ALIGNUP(sp, 16);
  size_t sza = sz - ((char *)spa - (char *)sp);

#if defined(CROUTON_UCONTEXT)
  if (getcontext(&c->callee) == -1)
    return -1;

  c->callee.uc_link = &c->caller;
  c->callee.uc_stack.ss_size = sza;
  c->callee.uc_stack.ss_sp = spa;

  makecontext(&c->callee, _crouton_do, 2, CROUTON_HGH(c), CROUTON_LOW(c));

#elif defined(CROUTON_ASSEMBLY)
  memset(&c->callee, 0, sizeof(crouton_ctx));
  c->callee.sp = (uintptr_t)spa + (uintptr_t)sza;
  c->callee.x30 = (uintptr_t)_crouton_do_ptr;

#else /* CROUTON_DISABLE */
  (void)spa;
  (void)sza;
#endif
  return 0;
}

void crouton_fini(crouton_t *c) { c->func = NULL; }

/*
 * Returns true if done
 */
int crouton_invoke(crouton_t *c) {
  int err = CROUTON_ERROR;
  if (c == NULL)
    return CROUTON_ERROR;

  if (c->func == NULL)
    return CROUTON_OVER;

#if defined(CROUTON_UCONTEXT)
  if (swapcontext(&c->caller, &c->callee) == -1)
    return CROUTON_ERROR;
  return c->func == NULL ? CROUTON_DONE : CROUTON_OK;

#elif defined(CROUTON_ASSEMBLY)
  crouton_swap(&c->caller, &c->callee, c);
  return c->func == NULL ? CROUTON_DONE : CROUTON_OK;

#else /* CROUTON_DISABLE */
  c->func(c);
  return CROUTON_OK;
#endif
}

int crouton_yield(crouton_t *c) {
  assert(c);
  assert(c->func != NULL);

#if defined(CROUTON_UCONTEXT)
  if (swapcontext(&c->callee, &c->caller) == -1)
    return CROUTON_ERROR;
  else
    return CROUTON_AGAIN;

#elif defined(CROUTON_ASSEMBLY)
  crouton_swap(&c->callee, &c->caller, 0);
  return CROUTON_AGAIN;

#else /* CROUTON_DISABLE */
  return CROUTON_DONE;
#endif
}

#if defined(CROUTON_ASSEMBLY)

#if defined(__APPLE__)
#define FUNC_NAME "_crouton_swap"
#define TYPE_LINE
#else
#define FUNC_NAME "crouton_swap"
#define TYPE_LINE ".type	_crouton_swap, @function"
#endif

#if defined(__aarch64__)
__asm__("	.align	4\n"
        "	.globl	" FUNC_NAME "\n"
        "	" TYPE_LINE "\n"
        "" FUNC_NAME ":\n"
        "	.cfi_startproc\n"
        "	/* save context */\n"
        "	stp	x19, x20, [x0], #16\n"
        "	stp	x21, x22, [x0], #16\n"
        "	stp	x23, x24, [x0], #16\n"
        "	stp	x25, x26, [x0], #16\n"
        "	stp	x27, x28, [x0], #16\n"
        "	stp	x29, x30, [x0], #16\n"
        "	stp	d8,  d9,  [x0], #16\n"
        "	stp	d10, d11, [x0], #16\n"
        "	stp	d12, d13, [x0], #16\n"
        "	stp	d14, d15, [x0], #16\n"
        "	mov     x10, sp\n"
        "	str     x10, [x0]\n"
        "	/* swap context */\n"
        "	ldp	x19, x20, [x1], #16\n"
        "	ldp	x21, x22, [x1], #16\n"
        "	ldp	x23, x24, [x1], #16\n"
        "	ldp	x25, x26, [x1], #16\n"
        "	ldp	x27, x28, [x1], #16\n"
        "	ldp	x29, x30, [x1], #16\n"
        "	ldp	d8,  d9,  [x1], #16\n"
        "	ldp	d10, d11, [x1], #16\n"
        "	ldp	d12, d13, [x1], #16\n"
        "	ldp	d14, d15, [x1], #16\n"
        "	ldr     x10, [x1]\n"
        "	mov	sp, x10\n"
        "	/* move arg to first function argument */\n"
        "	mov x0, x2\n"
        "	ret\n"
        "	.cfi_endproc");
#else
#error "Unsupported architecture"
#endif

#undef FUNC_NAME
#undef TYPE_LINE

#endif /* !CROUTON_ASSEMBLY */

#endif /* _CROUTON_H_ */
