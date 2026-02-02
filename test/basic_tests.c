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

  return UNITY_END();
}