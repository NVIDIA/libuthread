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

// rdi: Argument 0
// rsi: Function
// rdx: Stack begin
// rcx: Main stack
function launch_first_uthread
  pushq     %rbx
  pushq     %rbp
  pushq     %r12
  pushq     %r13
  pushq     %r14
  pushq     %r15
  movq      %rsp, (%rcx)

  mov       %rdx, %rsp
  callq     *%rsi
  retq
end_function

// rdi: Main stack
function restore_main_thread
  movq      (%rdi), %rsp
  popq      %r15
  popq      %r14
  popq      %r13
  popq      %r12
  popq      %rbp
  popq      %rbx

  xor       %rax, %rax
  movq      %rax, (%rdi)
  retq
end_function

.macro save_regs  dst
  movq      %rbx, 0 (\dst)
  movq      %rsp, 8 (\dst)
  movq      %rbp, 16(\dst)
  movq      %r12, 24(\dst)
  movq      %r13, 32(\dst)
  movq      %r14, 40(\dst)
  movq      %r15, 48(\dst)
.endm

.macro load_regs  src
  movq      0 (\src), %rbx
  movq      8 (\src), %rsp
  movq      16(\src), %rbp
  movq      24(\src), %r12
  movq      32(\src), %r13
  movq      40(\src), %r14
  movq      48(\src), %r15
.endm

// rdi: Register state of next thread.
// rsi: Register state of this thread.
function switch_uthread
  save_regs %rsi
  load_regs %rdi
  retq
end_function

// rdi: Argument 0
// rsi: Function
// rdx: Stack begin
// rcx: Register state of this thread.
function backup_and_launch_uthread
  save_regs %rcx
  mov       %rdx, %rsp
  callq     *%rsi
  retq
end_function

// rdi: Register state of next thread.
function restore_uthread
  load_regs %rdi
  retq
end_function

// rdi: Argument 0
// rsi: Function
// rdx: Stack begin
function launch_uthread
  mov       %rdx, %rsp
  callq     *%rsi
  retq
end_function
