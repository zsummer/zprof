
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
#include <limits.h>
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
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/shm.h>
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


enum PerfCounterType
{
    PERF_COUNTER_NULL,
    PERF_COUNTER_SYS,
    PERF_COUNTER_CLOCK,
    PERF_CONNTER_CHRONO,
    PERF_COUNTER_RDTSC,
    PERF_COUNTER_RDTSC_NOFENCE,
    PERF_COUNTER_MAX,
};

#define PERF_COUNTER_DEFAULT PERF_COUNTER_RDTSC
template<PerfCounterType T>
struct PerfCounterTypeClass
{
};



inline long long perf_tsc_rdtsc()
{
#ifdef WIN32
    return (long long)__rdtsc();
#else
    unsigned int lo, hi;
    __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}

inline long long perf_tsc_rdtsc_nofence()
{
#ifdef WIN32
    return (long long)__rdtsc();
#else
    unsigned long hi, lo;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#endif
}
inline long long perf_tsc_rdtscp()
{
#ifdef WIN32
    return (long long)__rdtsc();
#else
    unsigned long hi, lo;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi));
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#endif
}
inline long long perf_tsc_mfence()
{
#ifdef WIN32
    return (long long)__rdtsc();
#else
    unsigned int lo, hi;
    __asm__ __volatile__("mfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}

inline long long perf_tsc_clock()
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


inline long long perf_tsc_clock_thread()
{
#if (defined WIN32)
    long long count = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&count);
    return count;
#else
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
#endif
}


inline long long perf_tsc_sys()
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

inline long long perf_tsc_chrono()
{
    return std::chrono::high_resolution_clock().now().time_since_epoch().count();
}

inline long long perf_self_memory_use()
{
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
inline double perf_self_cpu_mhz()
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
    mhz /= 1000 * 1000;
#elif (defined WIN32)
    double freq_rate = 0;
    long long win_freq = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
    freq_rate = 1.0 / win_freq;
    freq_rate *= 1000.0 * 1000 * 1000;

    SYSTEM_INFO si = { 0 };
    GetSystemInfo(&si);
    std::vector<PERF_PROCESSOR_POWER_INFORMATION> pppi(si.dwNumberOfProcessors);
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

inline double perf_win_freq_rate()
{
    double freq_rate = 0;
    long long win_freq = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
    freq_rate = 1.0 / win_freq;
    freq_rate *= 1000.0 * 1000 * 1000;
    return freq_rate;
}


template<PerfCounterType T>
inline long long perf_tsc(const PerfCounterTypeClass<T>* ptr)
{
    (void)ptr;
    return perf_tsc_clock();
}



template<>
inline long long perf_tsc(const PerfCounterTypeClass<PERF_COUNTER_RDTSC>* ptr)
{
    (void)ptr;
    return perf_tsc_rdtsc();
}

template<>
inline long long perf_tsc(const PerfCounterTypeClass<PERF_COUNTER_RDTSC_NOFENCE>* ptr)
{
    (void)ptr;
    return perf_tsc_rdtsc_nofence();
}

template<>
inline long long perf_tsc(const PerfCounterTypeClass<PERF_COUNTER_CLOCK>* ptr)
{
    (void)ptr;
    return perf_tsc_clock();
}

template<>
inline long long perf_tsc(const PerfCounterTypeClass<PERF_COUNTER_SYS>* ptr)
{
    (void)ptr;
    return perf_tsc_sys();
}

template<>
inline long long perf_tsc(const PerfCounterTypeClass<PERF_CONNTER_CHRONO>* ptr)
{
    (void)ptr;
    return perf_tsc_chrono();
}



static inline const char* human_count_format(char* buff, long long count)
{
    if (count > 1000 * 1000)
    {
        sprintf(buff, "%lld,%03lld,%03lld", count / 1000 / 1000, (count / 1000) % 1000, count % 1000);
        return buff;
    }
    else if (count > 1000)
    {
        sprintf(buff, "%lld,%03lld", count / 1000, count % 1000);
        return buff;
    }
    sprintf(buff, "%lld", count);
    return buff;
}

static inline const char* human_count_format(long long count)
{
    static char buff[100] = { 0 };
    return human_count_format(buff, count);
}

static inline const char* human_time_format(char* buff, long long ns)
{
    if (ns > 1000 * 1000 * 1000)
    {
        sprintf(buff, "%.4lfs", ns / 1000.0 / 1000.0 / 1000.0);
        return buff;
    }
    else if (ns > 1000 * 1000)
    {
        sprintf(buff, "%.4lfms", ns / 1000.0 / 1000.0);
        return buff;
    }
    else if (ns > 1000)
    {
        sprintf(buff, "%.4lfus", ns / 1000.0);
        return buff;
    }
    sprintf(buff, "%lldns", ns);
    return buff;
}

static inline const char* human_time_format(long long ns)
{
    static char buff[100] = { 0 };
    return human_time_format(buff, ns);
}

static inline const char* human_mem_format(char* buff, long long bytes)
{
    if (bytes > 1024 * 1024 * 1024)
    {
        sprintf(buff, "%.4lfg", bytes / 1024.0 / 1024.0 / 1024.0);
        return buff;
    }
    else if (bytes > 1024 * 1024)
    {
        sprintf(buff, "%.4lfm", bytes / 1024.0 / 1024.0);
        return buff;
    }
    else if (bytes > 1024)
    {
        sprintf(buff, "%.4lfk", bytes / 1024.0);
        return buff;
    }
    sprintf(buff, "%lldb", bytes);
    return buff;
}

static inline const char* human_mem_format(long long bytes)
{
    static char buff[100] = { 0 };
    return human_mem_format(buff, bytes);
}







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
        start_val_ = perf_tsc<T>((PerfCounterTypeClass<T>*)NULL);
        cycles_ = 0;
    }

    PerfCounter& save()
    {
        cycles_ = perf_tsc<T>((PerfCounterTypeClass<T>*)NULL) - start_val_;
        //cycles_ = elapse > 0 ? elapse : 0;
        return *this;
    }
    PerfCounter& stop_and_save() { return save(); }

    long long cycles() { return cycles_; }
    long long duration_ns() 
    { 
        double rate = 1.0;
        switch (T)
        {
        case PERF_COUNTER_CLOCK:
#ifdef WIN32
            rate = perf_win_freq_rate();
#endif
            break;
        case PERF_COUNTER_RDTSC:
        case PERF_COUNTER_RDTSC_NOFENCE:
            rate = 1.0 / (perf_self_cpu_mhz() / 1000.0);
            break;
        default:
            break;
        }
        return (long long)(cycles_ * rate); 
    }
    double duration_second() { return (double)duration_ns() / (1000.0 * 1000.0 * 1000.0); }
    long long stop_val() { return start_val_ + cycles_; }
    long long start_val() { return start_val_; }

private:
    long long start_val_;
    long long cycles_;
};




#endif
