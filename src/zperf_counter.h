
/*
* zperf License
* Copyright (C) 2014-2021 YaweiZhang <yawei.zhang@foxmail.com>.
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



#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <array>
#include <limits.h>
#include <chrono>
#ifdef _WIN32
#ifndef KEEP_INPUT_QUICK_EDIT
#define KEEP_INPUT_QUICK_EDIT false
#endif

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <shlwapi.h>
#include <process.h>
#include <psapi.h>
#include <powerbase.h>
#include <powrprof.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "User32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"PowrProf.lib")
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#if !TARGET_OS_IPHONE
#define NFLOG_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#ifndef ZPERF_COUNTER_H
#define ZPERF_COUNTER_H

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif


#if __GNUC__ 
#define PERF_ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER) && !__INTEL_COMPILER && _MSC_VER >= 1310 // since Visual Studio .NET 2003
#define PERF_ALWAYS_INLINE inline __forceinline
#elif __has_attribute(always_inline)
#define PERF_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define PERF_ALWAYS_INLINE inline
#endif

/*
#if __GNUC__ 
#define PERF_NO_INLINE inline __attribute__((noinline))
#elif defined(_MSC_VER) && !__INTEL_COMPILER && _MSC_VER >= 1310 // since Visual Studio .NET 2003
#define PERF_NO_INLINE __declspec(noinline)
#elif __has_attribute(always_inline)
#define PERF_NO_INLINE inline __attribute__((noinline))
#else
#define PERF_NO_INLINE inline
#endif
*/
enum PerfCounterType
{
    PERF_COUNTER_NULL,
    PERF_COUNTER_SYS,
    PERF_COUNTER_CLOCK,
    PERF_CONNTER_CHRONO,
    PERF_COUNTER_RDTSC,
    PERF_COUNTER_RDTSC_STOP,
    PERF_COUNTER_RDTSCP,
    PERF_COUNTER_RDTSC_MFENCE,
    PERF_COUNTER_RDTSC_NOFENCE,
    PERF_COUNTER_RDTSC_PURE,
    PERF_COUNTER_MAX,
};
#ifndef PERF_COUNTER_DEFAULT
#define PERF_COUNTER_DEFAULT PERF_COUNTER_RDTSC_NOFENCE
#endif 


template<PerfCounterType T>
PERF_ALWAYS_INLINE long long perf_get_time_cycle();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_RDTSC>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_RDTSC_STOP>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_RDTSC_NOFENCE>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_RDTSC_PURE>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_RDTSC_MFENCE>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_RDTSCP>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_CLOCK>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_COUNTER_SYS>();

template<>
PERF_ALWAYS_INLINE long long perf_get_time_cycle<PERF_CONNTER_CHRONO>();

// all frequency is per ns  
template<PerfCounterType T>
PERF_ALWAYS_INLINE double perf_get_time_frequency();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_RDTSC>();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_RDTSC_STOP>();


template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_RDTSC_NOFENCE>();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_RDTSC_PURE>();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_RDTSC_MFENCE>();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_RDTSCP>();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_CLOCK>();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_COUNTER_SYS>();

template<>
PERF_ALWAYS_INLINE double perf_get_time_frequency<PERF_CONNTER_CHRONO>();


template<PerfCounterType T>
PERF_ALWAYS_INLINE double perf_get_time_inverse_frequency();


PERF_ALWAYS_INLINE long long perf_get_mem_use();



template<PerfCounterType T = PERF_COUNTER_DEFAULT>
class PerfCounter
{
public:
    PerfCounter()
    {
        start_val_ = 0;
        cycles_ = 0;
    }
    PerfCounter(long long val)
    {
        start_val_ = val;
        cycles_ = 0;
    }
    void start()
    {
        start_val_ = perf_get_time_cycle<T>();
        cycles_ = 0;
    }

    PerfCounter& save()
    {
        cycles_ = perf_get_time_cycle<T>() - start_val_;
        return *this;
    }

    PerfCounter& stop_and_save() { return save(); }

    long long stop_val() { return start_val_ + cycles_; }
    long long start_val() { return start_val_; }

    long long cycles() { return cycles_; }
    PERF_ALWAYS_INLINE long long duration_ns() { return (long long)(cycles_ * perf_get_time_inverse_frequency<T>()); }
    double duration_second() { return (double)duration_ns() / (1000.0 * 1000.0 * 1000.0); }

private:
    long long start_val_;
    long long cycles_;
};









template<PerfCounterType T>
long long perf_get_time_cycle()
{
    return 0;
}

template<>
long long perf_get_time_cycle<PERF_COUNTER_RDTSC>()
{
#ifdef WIN32
    //__faststorefence();
    _mm_lfence();
    return (long long)__rdtsc();
#else
    unsigned int lo, hi;
    __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}

template<>
long long perf_get_time_cycle<PERF_COUNTER_RDTSC_STOP>()
{
#ifdef WIN32
    //__faststorefence();
    long long ret;
    ret = (long long)__rdtsc();
    _mm_lfence();
    return ret;
#else
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc;lfence" : "=a" (lo), "=d" (hi) :: "memory");
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}


template<>
long long perf_get_time_cycle<PERF_COUNTER_RDTSC_NOFENCE>()
{
#ifdef WIN32
    return (long long)__rdtsc();
#else
    unsigned long hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#endif
}

template<>
long long perf_get_time_cycle<PERF_COUNTER_RDTSC_PURE>()
{
#ifdef WIN32
    return (long long)__rdtsc();
#else
    unsigned long hi, lo;
    __asm__ ("rdtsc" : "=a"(lo), "=d"(hi));
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#endif
}

template<>
long long perf_get_time_cycle<PERF_COUNTER_RDTSC_MFENCE>()
{
#ifdef WIN32
    _mm_mfence();
    return (long long)__rdtsc();
#else
    unsigned int lo, hi;
    __asm__ __volatile__("mfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}

template<>
long long perf_get_time_cycle<PERF_COUNTER_RDTSCP>()
{
#ifdef WIN32
    unsigned int ui;
    return (long long)__rdtscp(&ui);
#else
    unsigned long hi, lo;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"memory");
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#endif
}


template<>
long long perf_get_time_cycle<PERF_COUNTER_CLOCK>()
{
#if (defined WIN32)
    long long count = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&count);
    return count;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
#endif
}

template<>
long long perf_get_time_cycle<PERF_COUNTER_SYS>()
{
#if (defined WIN32)
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long tsc = ft.dwHighDateTime;
    tsc <<= 32;
    tsc |= ft.dwLowDateTime;
    tsc /= 10;
    tsc -= 11644473600000000ULL;
    return (long long)tsc * 1000; //ns
#else
    struct timeval tm;
    gettimeofday(&tm, nullptr);
    return tm.tv_sec * 1000 * 1000 * 1000 + tm.tv_usec * 1000;
#endif
}

template<>
long long perf_get_time_cycle<PERF_CONNTER_CHRONO>()
{
    return std::chrono::high_resolution_clock().now().time_since_epoch().count();
}





long long perf_get_mem_use()
{
#ifdef WIN32
    HANDLE hproc = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hproc, &pmc, sizeof(pmc)))
    {
        CloseHandle(hproc);// ignore  
        return pmc.WorkingSetSize;
    }
    return 0;
#else
    const char* file = "/proc/self/status";
    FILE* fp = fopen(file, "r");
    if (NULL == fp)
    {
        return 0;
    }

    char line_buff[256];
    int vm_size = 0;
    while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
    {
        if (strstr(line_buff, "VmSize") != NULL)
        {
            const char* p = line_buff;
            while (p < &line_buff[255] && (*p < '0' || *p > '9'))
            {
                p++;
            }
            if (p == &line_buff[255])
            {
                break;
            }

            int ret = sscanf(p, "%d", &vm_size);
            if (ret <= 0)
            {
                vm_size = 0;
                break;
            }
            break;
        }
    }

    fclose(fp);
    return (long long)vm_size * 1000;
#endif
}

#ifdef WIN32
struct PERF_PROCESSOR_POWER_INFORMATION
{
    ULONG  Number;
    ULONG  MaxMhz;
    ULONG  CurrentMhz;
    ULONG  MhzLimit;
    ULONG  MaxIdleState;
    ULONG  CurrentIdleState;
};
#endif
PERF_ALWAYS_INLINE double perf_get_cpu_mhz()
{
    double mhz = 1;
#ifdef __APPLE__
    int mib[2];
    unsigned int freq;
    size_t len;
    mib[0] = CTL_HW;
    mib[1] = HW_CPU_FREQ;
    len = sizeof(freq);
    sysctl(mib, 2, &freq, &len, NULL, 0);
    mhz = freq;
    mhz /= 1000.0 * 1000.0;
#elif (defined WIN32)
    SYSTEM_INFO si = { 0 };
    GetSystemInfo(&si);
    std::array< PERF_PROCESSOR_POWER_INFORMATION, 128> pppi;
    DWORD dwSize = sizeof(PERF_PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors;
    memset(&pppi[0], 0, dwSize);
    long ret = CallNtPowerInformation(ProcessorInformation, NULL, 0, &pppi[0], dwSize);
    if (ret != 0 || pppi[0].MaxMhz <= 0)
    {
        return 1;
    }
    mhz = pppi[0].MaxMhz;
#else
    const char* file = "/proc/cpuinfo";
    FILE* fp = fopen(file, "r");
    if (NULL == fp)
    {
        return 0;
    }

    char line_buff[256];

    while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
    {
        if (strstr(line_buff, "cpu MHz") != NULL)
        {
            const char* p = line_buff;
            while (p < &line_buff[255] && (*p < '0' || *p > '9'))
            {
                p++;
            }
            if (p == &line_buff[255])
            {
                break;
            }

            int ret = sscanf(p, "%lf", &mhz);
            if (ret <= 0)
            {
                mhz = 1;
                break;
            }
            break;
        }
    }
    fclose(fp);
#endif // __APPLE__
    return mhz;
}




template<PerfCounterType T>
double perf_get_time_frequency()
{
    return 1.0;
}

template<>
double perf_get_time_frequency<PERF_COUNTER_RDTSC>()
{
    static double frequency_per_ns = perf_get_cpu_mhz() * 1000.0 * 1000.0 / 1000.0 / 1000.0 / 1000.0;
    return frequency_per_ns;
}
template<>
double perf_get_time_frequency<PERF_COUNTER_RDTSC_STOP>()
{
    return perf_get_time_frequency<PERF_COUNTER_RDTSC>();
}

template<>
double perf_get_time_frequency<PERF_COUNTER_RDTSC_NOFENCE>()
{
    return perf_get_time_frequency<PERF_COUNTER_RDTSC>();
}

template<>
double perf_get_time_frequency<PERF_COUNTER_RDTSC_PURE>()
{
    return perf_get_time_frequency<PERF_COUNTER_RDTSC>();
}

template<>
double perf_get_time_frequency<PERF_COUNTER_RDTSC_MFENCE>()
{
    return perf_get_time_frequency<PERF_COUNTER_RDTSC>();
}

template<>
double perf_get_time_frequency<PERF_COUNTER_RDTSCP>()
{
    return perf_get_time_frequency<PERF_COUNTER_RDTSC>();
}

template<>
double perf_get_time_frequency<PERF_COUNTER_CLOCK>()
{
#ifdef WIN32
    double frequency_per_ns = 0;
    long long win_freq = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
    frequency_per_ns = win_freq / 1000.0 / 1000.0 / 1000.0;
    return frequency_per_ns;
#else
    return 1.0;
#endif
}

template<>
double perf_get_time_frequency<PERF_COUNTER_SYS>()
{
    return 1.0;
}

template<>
double perf_get_time_frequency<PERF_CONNTER_CHRONO>()
{
    static double chrono_frequency = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
    return chrono_frequency;
}


template<PerfCounterType T>
double perf_get_time_inverse_frequency()
{
    static double inverse_frequency_per_ns = 1.0 / (perf_get_time_frequency<T>() <= 0.0 ? 1.0 : perf_get_time_frequency<T>());
    return inverse_frequency_per_ns;
}


#endif
