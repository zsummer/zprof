/*
 *
 * MIT License
 *
 * Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ===============================================================================
 *
 * (end of COPYRIGHT)
 */


#include <cstddef>
#include <type_traits>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <array>
#include <mutex>
#include <thread>
#include <functional>
#include <regex>
#include <atomic>
#include <cmath>
#include <cfloat>
#include <list>
#include <deque>
#include <queue>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <memory>
#include <atomic>

#ifdef WIN32


#define WIN32_LEAN_AND_MEAN                      
#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "User32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define NFLOG_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif

#include "fn_log.h"

#ifndef BASE_DEF_H_
#define BASE_DEF_H_
typedef char s8;
typedef unsigned char u8;
typedef short int s16;
typedef unsigned short int u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;
typedef unsigned long pointer;
typedef unsigned int qq_t;
typedef int BOOL;
typedef float f32;


#if __GNUG__ && __GNUC__ < 5
#define IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define IS_TRIVIALLY_COPYABLE(T) std::is_trivially_copyable<T>::value
#endif

#if __GNUG__
#define MAY_ALIAS __attribute__((__may_alias__))
#else
#define MAY_ALIAS
#endif


enum AllocStatusType:u32
{
    ALLOC_SYS = 0,
    ALLOC_LARGE = 1,
    ALLOC_DLMALLOC,
    ALLOC_ZMALLOC,
    ALLOC_STATUS_END,
};

#define SHM_DEBUG false
#define ZMALLOC_OPEN_CHECK false
#define ZMALLOC_OPEN_FENCE true
#define OPEN_UNITS_STRESS true
#define OPEN_STATISTIC false
#define OPEN_ALLOCATOR_STATUS true
#define OPEN_SYS_VM_MMAP_STRESS false
#define OPEN_BUDDY_FIRST_FIT true

const std::map<u32, bool> OPEN_ALLOC_STRESS =
{
    {ALLOC_SYS,true},
    {ALLOC_LARGE,true},
    {ALLOC_DLMALLOC,true},
    {ALLOC_ZMALLOC,true},
};

const std::map<u32, bool> OPEN_ALLOC_STRESS_MOD =
{
    {0,true}, //small
    {1,true}, //medium
    {2,true}, //big
    {3,true}, //dyn
    {4,true}, //rand
    {5,true}, // continuous  
};


#ifdef WIN32 
#define forceinline __forceinline
#elif defined __GNUC__ // for gcc on Linux/Apple OS X
#define forceinline __inline__ __attribute__((always_inline))
#endif


#endif
