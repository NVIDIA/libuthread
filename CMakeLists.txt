# SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)

#set(CMAKE_C_COMPILER clang)

project(uthread LANGUAGES C ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wconversion")

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set(NVUT_TARGET "-print-target-triple")
else()
  # Assume GNU.
  set(NVUT_TARGET "-print-multiarch")
endif()
execute_process(
  COMMAND "${CMAKE_C_COMPILER}" "${NVUT_TARGET}"
  OUTPUT_VARIABLE NVUT_TARGET
  COMMAND_ERROR_IS_FATAL ANY
)
string(REGEX MATCH "^[^-]+" NVUT_TARGET "${NVUT_TARGET}")
message(STATUS "NVUT_TARGET = ${NVUT_TARGET}")

option(NVUT_BUILD_TESTS "Build tests" ON)
message(STATUS "NVUT_BUILD_TESTS = ${NVUT_BUILD_TESTS}")

include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

add_library(uthread STATIC src/uthread.c "src/uthread.${NVUT_TARGET}.S")

if(NVUT_BUILD_TESTS)
  add_subdirectory(third_party)
  add_subdirectory(test)
endif()
