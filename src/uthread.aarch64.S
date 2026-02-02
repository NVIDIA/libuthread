// SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

.section .text

.macro function name
  .macro end_function
    .size \name, . -\name
    .purgem end_function
  .endm
  .global \name
  .type \name, %function
  .align 16
  \name:
.endm

// x0: Argument 0
// x1: Function
// x2: Stack begin
// x3: Main stack
function launch_first_uthread
  sub     x4, sp, #160
  str     x4, [x3]

  stp     x19, x20, [x4,   #0]
  stp     x21, x22, [x4,  #16]
  stp     x23, x24, [x4,  #32]
  stp     x25, x26, [x4,  #48]
  stp     x27, x28, [x4,  #64]
  stp     x29, x30, [x4,  #80]
  stp     d8,  d9,  [x4,  #96]
  stp     d10, d11, [x4, #112]
  stp     d12, d13, [x4, #128]
  stp     d14, d15, [x4, #144]

  mov     sp, x2
  br      x1
  ret
end_function

// x0: Main stack
function restore_main_thread
  ldr     x1, [x0]

  ldp     x19, x20, [x1,   #0]
  ldp     x21, x22, [x1,  #16]
  ldp     x23, x24, [x1,  #32]
  ldp     x25, x26, [x1,  #48]
  ldp     x27, x28, [x1,  #64]
  ldp     x29, x30, [x1,  #80]
  ldp     d8,  d9,  [x1,  #96]
  ldp     d10, d11, [x1, #112]
  ldp     d12, d13, [x1, #128]
  ldp     d14, d15, [x1, #144]

  str     xzr, [x0]

  add     sp, x1, #160
  ret
end_function

.macro save_regs  dst
  mov     x10, sp
  stp     d8,  d9,  [\dst,   #0]
  stp     d10, d11, [\dst,  #16]
  stp     d12, d13, [\dst,  #32]
  stp     d14, d15, [\dst,  #48]
  stp     x10, x19, [\dst,  #64]
  stp     x20, x21, [\dst,  #80]
  stp     x22, x23, [\dst,  #96]
  stp     x24, x25, [\dst, #112]
  stp     x26, x27, [\dst, #128]
  stp     x28, x29, [\dst, #144]
  str     x30,      [\dst, #160]
.endm

.macro load_regs  src
  ldp     d8,  d9,  [\src,   #0]
  ldp     d10, d11, [\src,  #16]
  ldp     d12, d13, [\src,  #32]
  ldp     d14, d15, [\src,  #48]
  ldp     x10, x19, [\src,  #64]
  ldp     x20, x21, [\src,  #80]
  ldp     x22, x23, [\src,  #96]
  ldp     x24, x25, [\src, #112]
  ldp     x26, x27, [\src, #128]
  ldp     x28, x29, [\src, #144]
  ldr     x30,      [\src, #160]
  mov     sp, x10
.endm

// x0: Register state of next thread.
// x1: Register state of this thread.
function switch_uthread
  save_regs x1
  load_regs x0
  ret
end_function

// x0: Argument 0
// x1: Function
// x2: Stack begin
// x3: Register state of this thread.
function backup_and_launch_uthread
  save_regs x3
  mov       sp, x2
  br        x1
  ret
end_function

// x0: Register state of next thread.
function restore_uthread
  load_regs x0
  ret
end_function

// x0: Argument 0
// x1: Function
// x2: Stack begin
function launch_uthread
  mov     sp, x2
  br      x1
  ret
end_function