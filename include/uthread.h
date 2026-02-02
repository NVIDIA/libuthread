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

#ifndef __UTHREAD_H__
#define __UTHREAD_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*uthread_fun_t)(void *);

typedef enum {
  UTHREAD_SUCCESS = 0,
  UTHREAD_ERROR_ALLOC = 1
} uthread_status_t;

size_t uthread_pool_size(void);

uthread_status_t preallocate_uthreads(size_t n);

uthread_status_t free_unused_uthreads(void);

uthread_status_t create_uthread(uthread_fun_t fun, void *arg);

size_t uthread_queue_size(void);

size_t this_uthread_id(void);

size_t this_uthread_run_count(void);

void this_uthread_yield(void);

#ifdef __cplusplus
}
#endif

#endif