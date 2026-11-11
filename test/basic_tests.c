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

#include <stdlib.h>
#include <unity.h>
#include <uthread.h>

#if defined(__aarch64__) && defined(__linux__)
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#endif

void setUp(void) {}

void tearDown(void) {}

void test_prealloc(void) {
  TEST_ASSERT_EQUAL(0, uthread_pool_size());
  TEST_ASSERT_EQUAL(0, uthread_queue_size());
  TEST_ASSERT_EQUAL(UTHREAD_SUCCESS, free_unused_uthreads());
  TEST_ASSERT_EQUAL(0, uthread_pool_size());
  TEST_ASSERT_EQUAL(0, uthread_queue_size());
  for (size_t i = 0; i < 4; ++i) {
    TEST_ASSERT_EQUAL(UTHREAD_SUCCESS, preallocate_uthreads(7 * i));
    TEST_ASSERT_EQUAL(7 * i, uthread_pool_size());
    TEST_ASSERT_EQUAL(0, uthread_queue_size());
    TEST_ASSERT_EQUAL(UTHREAD_SUCCESS, free_unused_uthreads());
    TEST_ASSERT_EQUAL(0, uthread_pool_size());
    TEST_ASSERT_EQUAL(0, uthread_queue_size());
  }
}

void test_empty_lodge(void) {
  size_t cnt = 0;
  TEST_ASSERT_EQUAL(0, uthread_queue_size());
  this_uthread_yield();
  TEST_ASSERT_EQUAL(0, uthread_queue_size());
  TEST_ASSERT_EQUAL(0, cnt);
}

void func0(void *arg) { printf("uThread %lu, cnt = %lu\n", this_uthread_id(), ++(*(size_t *)arg)); }

void test_uthread_single_lodge(void) {
  size_t cnt = 0;
  create_uthread(func0, &cnt);
  TEST_ASSERT_EQUAL(1, uthread_queue_size());
  this_uthread_yield();
  TEST_ASSERT_EQUAL(0, uthread_queue_size());
  TEST_ASSERT_EQUAL(1, cnt);
}

void test_uthread_multi_lodge(void) {
  size_t cnt = 0;
  for (size_t i = 0; i < 13; ++i) {
    create_uthread(func0, &cnt);
  }
  TEST_ASSERT_EQUAL(13, uthread_queue_size());
  this_uthread_yield();
  TEST_ASSERT_EQUAL(0, uthread_queue_size());
  TEST_ASSERT_EQUAL(13, cnt);
}

void func1(void *arg) {
  printf(
    "uThread %lu, run_count = %lu, cnt = %lu (yield)\n", this_uthread_id(),
    this_uthread_run_count(), ++(*(size_t *)arg)
  );
  this_uthread_yield();
  printf(
    "uThread %lu, run_count = %lu, cnt = %lu (resume-yield)\n", this_uthread_id(),
    this_uthread_run_count(), ++(*(size_t *)arg)
  );
  this_uthread_yield();
  printf(
    "uThread %lu, run_count = %lu, cnt = %lu (resume)\n", this_uthread_id(),
    this_uthread_run_count(), ++(*(size_t *)arg)
  );
}

void test_uthread_single_lodge_and_yield(void) {
  size_t cnt = 0;
  create_uthread(func1, &cnt);
  TEST_ASSERT_EQUAL(1, uthread_queue_size());
  this_uthread_yield();
  TEST_ASSERT_EQUAL(3, cnt);
}

void test_uthread_multi_lodge_and_yield(void) {
  size_t cnt = 0;
  for (size_t i = 0; i < 13; ++i) {
    create_uthread(func1, &cnt);
  }
  TEST_ASSERT_EQUAL(13, uthread_queue_size());
  this_uthread_yield();
  TEST_ASSERT_EQUAL(39, cnt);
}

void func2(void *arg) {
  printf(
    "uThread %lu, run_count = %lu, cnt = %lu (start)\n", this_uthread_id(),
    this_uthread_run_count(), ++(*(size_t *)arg)
  );
  if (rand() % 2 == 0) return;
  printf(
    "uThread %lu, run_count = %lu, cnt = %lu (yield), queuing another thread.\n", this_uthread_id(),
    this_uthread_run_count(), ++(*(size_t *)arg)
  );
  create_uthread(func2, arg);
  this_uthread_yield();
  printf(
    "uThread %lu, run_count = %lu, cnt = %lu (resume)\n", this_uthread_id(),
    this_uthread_run_count(), ++(*(size_t *)arg)
  );
}

void test_uthread_dynamic_sub_thread_lodge(void) {
  size_t cnt = 0;
  for (size_t i = 0; i < 13; ++i) {
    create_uthread(func2, &cnt);
  }
  TEST_ASSERT_EQUAL(13, uthread_queue_size());
  this_uthread_yield();
  // TODO: How to check?
}

#if defined(__aarch64__) && defined(__linux__)
static atomic_int signal_stop;
static pid_t signal_target;

static void signal_noop(int sig) { (void)sig; }

static void *signal_flood(void *arg) {
  (void)arg;
  while (!atomic_load(&signal_stop)) {
    syscall(SYS_tgkill, getpid(), signal_target, SIGUSR1);
  }
  return NULL;
}

static void noop_uthread(void *arg) { (void)arg; }

// Enter uthread processing with known values in the callee-saved d14/d15 and report whether they survived.
static int enter_uthreads_preserves_d14_d15(void) {
  uint64_t a = 0x1111222233334444u, b = 0x5555666677778888u, ra, rb;
  create_uthread(noop_uthread, NULL);
  __asm__ volatile(
    "fmov d14, %x[a]\n\t"
    "fmov d15, %x[b]\n\t"
    "bl this_uthread_yield\n\t"
    "fmov %x[ra], d14\n\t"
    "fmov %x[rb], d15\n\t"
    : [ra] "=r"(ra), [rb] "=r"(rb)
    : [a] "r"(a), [b] "r"(b)
    : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14",
      "x15", "x16", "x17", "x18", "x30", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28",
      "v29", "v30", "v31", "cc", "memory");
  return ra == a && rb == b;
}

// A signal delivered while entering uthread processing must not clobber the saved main-thread context.
void test_uthread_enter_with_signals(void) {
  struct sigaction sa, old;
  sa.sa_handler = signal_noop;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  TEST_ASSERT_EQUAL(0, sigaction(SIGUSR1, &sa, &old));

  signal_target = (pid_t)syscall(SYS_gettid);
  atomic_store(&signal_stop, 0);
  pthread_t flood;
  TEST_ASSERT_EQUAL(0, pthread_create(&flood, NULL, signal_flood, NULL));

  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  long corrupted = 0;
  for (long i = 0;; ++i) {
    corrupted += !enter_uthreads_preserves_d14_d15();
    if ((i & 0xffff) == 0) {
      clock_gettime(CLOCK_MONOTONIC, &t1);
      if (corrupted || t1.tv_sec - t0.tv_sec >= 10) break;
    }
  }

  atomic_store(&signal_stop, 1);
  pthread_join(flood, NULL);
  sigaction(SIGUSR1, &old, NULL);
  TEST_ASSERT_EQUAL(0, corrupted);
}
#endif

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_prealloc);

  for (size_t i = 0; i < 3; ++i) {
    RUN_TEST(test_empty_lodge);
    RUN_TEST(test_uthread_single_lodge);
    RUN_TEST(test_uthread_multi_lodge);
  }

  for (size_t i = 0; i < 3; ++i) {
    RUN_TEST(test_uthread_single_lodge_and_yield);
    RUN_TEST(test_uthread_multi_lodge_and_yield);
  }

  for (uint32_t i = 0; i < 3; ++i) {
    srand(4711 + i);
    RUN_TEST(test_uthread_dynamic_sub_thread_lodge);
  }

#if defined(__aarch64__) && defined(__linux__)
  RUN_TEST(test_uthread_enter_with_signals);
#endif

  return UNITY_END();
}