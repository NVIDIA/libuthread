<!--
SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-->

# `libuthread` - User-level threading support

`libuthread` is a minimalist library that provides software-controlled user-level thread management and context switching to alleviate low CPU utilization issues by hiding memory latencies on ARM64 and x86 Linux systems.

This is achieved through providing individual operating system (OS) threads with the ability to declare multiple workloads and allow processing them pseudo-parallel. That is, for a given OS thread, the user program can enqueue multiple sequences of mutually largely independent command sequences to execute. These command sequences can be augmented to indicate expected stalls, for example due to a likely cache miss, which would in turn introduce memory access latencies.

Instead of conducting a memory lookup that is likely to lead to the cache miss, the user program just issues memory prefetch operations (*e.g.*, using `__builtin_prefetch(ptr, 3)`) followed by invoking `this_uthread_yield()`. The latter signals `libuthread` that this might be a good location to do something else. Using this information `libuthread` alternates between the command sequences by transparently saving/restoring the CPU state, so that individual latencies incurred from bespoke cache misses are hidden.

For suitable scenarios, such user-level thread switching can significantly increase the overall throughput of individiual OS threads. This is achieved without creating more threads, which would subsequently add costly OS scheduling overheads. In contrast, "uThreads" are more efficient because the toggling between register states and execution stacks is done in user-level code without needing intervention from the operating system's kernel.


## Building
`libuthread` is implemented in the C language, and is built using the [CMake](https://cmake.org) build system.

### Prerequisites (Debian/Ubuntu + GCC)
```shell
sudo apt-get update
sudo apt-get install -y make cmake gcc
export CC="gcc"
```

### Prerequisites (Debian/Ubuntu + Clang)
```shell
sudo apt-get update
sudo apt-get install -y make cmake clang
export CC="clang"
```

### Compile just `libuthread` as a static link library
```
git clone https://gitlab-master.nvidia.com/dsheffield/uthread_switch.git libuthread
cd libuthread
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DNVUT_BUILD_TESTS=OFF ..
make -j
```

### Compile `libuthread` with unit tests, and run unit tests
```
git clone https://gitlab-master.nvidia.com/dsheffield/uthread_switch.git libuthread
git submodule update --init --recursive
cd libuthread
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DNVUT_BUILD_TESTS=ON ..
make -j
test/basic_tests
```


## Project status
Actively maintained.


## License

```text
SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
