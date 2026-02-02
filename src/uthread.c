/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <uthread.h>

#ifndef NDEBUG
#include <stdio.h>
#define UTHREAD_DEBUG_PRINT_(...) printf(__VA_ARGS__)
#else
#define UTHREAD_DEBUG_PRINT_(...) \
  do {                            \
  } while (0)
#endif

#define UTHREAD_LIKELY_(_expr_) __builtin_expect((_expr_), 1)
#define UTHREAD_UNLIKELY_(_expr_) __builtin_expect((_expr_), 0)

#if defined(__x86_64__)
// callee_saved: [
//   rbx, rsp, rbp,
//   r12, r13, r14, r15
// ]
#define UTHREAD_NUM_SAVED_REGS 7
#elif defined(__aarch64__)
// callee_saved: [
//   d8, d9, d10, d11,
//   d12, d13, d14, d15,
//   sp,
//   x19, x20, x21, x22,
//   x23, x24, x25, x26,
//   x27, x28, x29, x30
// ]
// #define UTHREAD_NUM_SAVED_REGS 13
#define UTHREAD_NUM_SAVED_REGS 21
#else
#error "Unsupported architecture"
#endif
#define UTHREAD_STACK_SIZE (1 << 21)

struct uthread;
typedef struct uthread *uthread_t;

typedef void (*uthread_main_fun_t)(uthread_t);

#ifdef __cplusplus
extern "C" {
#endif

extern void launch_first_uthread(void *arg, uthread_main_fun_t fun, void *stack, void **main_stack);
extern void restore_main_thread(void **main_stack);

extern void switch_uthread(uint64_t *next_regs, uint64_t *this_regs);
extern void backup_and_launch_uthread(
  void *arg, uthread_main_fun_t fun, void *stack, uint64_t *this_regs
);
extern void restore_uthread(uint64_t *next_regs);
extern void launch_uthread(void *arg, uthread_main_fun_t fun, void *stack);

#ifdef __cplusplus
}
#endif

struct uthread {
  uthread_t next, prev;

  size_t id;
  size_t run_cnt;
  uthread_fun_t fun;
  void *arg;

  uint64_t regs[UTHREAD_NUM_SAVED_REGS] __attribute__((aligned(16)));
  uint8_t stack[UTHREAD_STACK_SIZE] __attribute__((aligned(16)));
};

static void *stack_of(uthread_t thr) { return &thr->stack[UTHREAD_STACK_SIZE - 16]; }

static __thread size_t next_uthread_id = 1;
static __thread uthread_t pool = NULL;
static __thread uthread_t queue = NULL;
static __thread void *main_stack = NULL;

size_t uthread_pool_size(void) {
  size_t n = 0;
  for (uthread_t thr = pool; thr; thr = thr->next) {
    ++n;
  }
  return n;
}

uthread_status_t preallocate_uthreads(size_t n) {
  for (size_t i = 0; i < n; ++i) {
    uthread_t thr = (uthread_t)malloc(sizeof(*thr));
    if (!thr) return UTHREAD_ERROR_ALLOC;
    thr->id = next_uthread_id++;

    thr->next = pool;
    pool = thr;
  }
  return UTHREAD_SUCCESS;
}

uthread_status_t free_unused_uthreads(void) {
  uthread_t thr = pool;
  while (thr) {
    uthread_t next = thr->next;
    free(thr);
    thr = next;
  }
  pool = NULL;
  return UTHREAD_SUCCESS;
}

uthread_status_t create_uthread(uthread_fun_t fun, void *arg) {
  uthread_t thr;
  if (pool) {
    thr = pool;
    pool = thr->next;
  } else {
    thr = (uthread_t)malloc(sizeof(*thr));
    if (!thr) return UTHREAD_ERROR_ALLOC;
    thr->id = next_uthread_id++;
  }

  // Append the thread to the end of the queue.
  uthread_t beg = queue, end;
  if (beg) {
    end = beg->prev;
    end->next = thr;
    beg->prev = thr;
  } else {
    beg = thr;
    end = thr;
    queue = thr;
  }
  thr->next = beg;
  thr->prev = end;

  // Initialize the thread.
  thr->run_cnt = 0;
  thr->fun = fun;
  thr->arg = arg;
  return UTHREAD_SUCCESS;
}

size_t uthread_queue_size(void) {
  uthread_t beg = queue;
  if (!beg) return 0;

  uthread_t end = beg->prev;
  size_t n = 1;
  for (uthread_t thr = beg; thr != end; thr = thr->next) {
    ++n;
  }
  return n;
}

size_t this_uthread_id(void) {
  uthread_t thr = queue;
  if (!thr) {
    UTHREAD_DEBUG_PRINT_("ERROR: Not inside a uthread.\n");
    return 0;
  }

  return thr->id;
}

size_t this_uthread_run_count(void) {
  uthread_t thr = queue;
  if (!thr) {
    UTHREAD_DEBUG_PRINT_("ERROR: Not inside a uthread.\n");
    return 0;
  }

  return thr->run_cnt;
}

void uthread_main(uthread_t self) {
  assert(self && self == queue);
  self->fun(self->arg);

  uthread_t prev = self->prev;
  uthread_t next = self->next;
  assert(prev);
  assert(next);

  // Remove this thread from the queue, and add it to the pool.
  prev->next = next;
  next->prev = prev;

  self->next = pool;
  pool = self;

  // If this is was not the last thread.
  if (UTHREAD_LIKELY_(next != self)) {
    queue = next;
    if (next->run_cnt++) {
      restore_uthread(next->regs);
    } else {
      launch_uthread(next, uthread_main, stack_of(next));
    }
  } else {
    queue = NULL;
    restore_main_thread(&main_stack);
  }
}

void this_uthread_yield(void) {
  uthread_t cur = queue;

  // If we entered uthread processing.
  if (UTHREAD_LIKELY_(main_stack != NULL)) {
    // Toggle between threads.
    assert(cur);
    uthread_t next = cur->next;
    assert(next);

    queue = next;
    if (next->run_cnt++) {
      switch_uthread(next->regs, cur->regs);
    } else {
      backup_and_launch_uthread(next, uthread_main, stack_of(next), cur->regs);
    }
  } else if (cur) {
    // Enter uthread processing.
    size_t run_cnt = cur->run_cnt++;
#ifndef NDEBUG
    assert(run_cnt == 0);
#else
    (void)run_cnt;
#endif

    UTHREAD_DEBUG_PRINT_(">>> launch_first_uthread >>>\n");
    launch_first_uthread(cur, uthread_main, stack_of(cur), &main_stack);
    UTHREAD_DEBUG_PRINT_("<<< launch_first_uthread <<<\n");
  }
}
