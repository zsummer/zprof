#ifdef __GNUG__
#pragma GCC push_options
#pragma GCC optimize ("O2")
#endif

/*
* zprof License
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
#include <string.h>
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
#include <profileapi.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "Kernel32")
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

#ifndef ZPROF_COUNTER_H
#define ZPROF_COUNTER_H

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif


#if __GNUC__ 
#define PROF_ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER) && !__INTEL_COMPILER && _MSC_VER >= 1310 // since Visual Studio .NET 2003
#define PROF_ALWAYS_INLINE inline __forceinline
#elif __has_attribute(always_inline)
#define PROF_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define PROF_ALWAYS_INLINE inline
#endif


enum ProfCounterType
{
    PROF_COUNTER_NULL,
    PROF_COUNTER_SYS,
    PROF_COUNTER_CLOCK,
    PROF_CONNTER_CHRONO,

    PROF_COUNTER_RDTSC_PURE,
    PROF_COUNTER_RDTSC_NOFENCE,
    PROF_COUNTER_RDTSC,
    PROF_COUNTER_RDTSC_BTB,
    PROF_COUNTER_RDTSCP,
    PROF_COUNTER_RDTSC_MFENCE,
    PROF_COUNTER_RDTSC_MFENCE_BTB,
    
    PROF_COUNTER_RDTSC_LOCK,
    PROF_COUNTER_MAX,
};
#ifndef PROF_COUNTER_DEFAULT
#define PROF_COUNTER_DEFAULT PROF_COUNTER_RDTSC_NOFENCE
#endif 


struct ProfVM
{
    unsigned long long vm_size;
    unsigned long long rss_size;
    unsigned long long shr_size;
};

template<ProfCounterType T>
PROF_ALWAYS_INLINE long long prof_get_time_cycle();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_PURE>();
template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_LOCK>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE_BTB>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSCP>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_CLOCK>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_SYS>();

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_CONNTER_CHRONO>();

// all frequency is per ns  
template<ProfCounterType T>
PROF_ALWAYS_INLINE double prof_get_time_frequency();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_BTB>();


template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_NOFENCE>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_PURE>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_LOCK>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_MFENCE>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_MFENCE_BTB>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSCP>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_CLOCK>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_SYS>();

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_CONNTER_CHRONO>();


template<ProfCounterType T>
PROF_ALWAYS_INLINE double prof_get_time_inverse_frequency();


PROF_ALWAYS_INLINE ProfVM prof_get_mem_use();



template<ProfCounterType T = PROF_COUNTER_DEFAULT>
class ProfCounter
{
public:
    ProfCounter()
    {
        start_val_ = 0;
        cycles_ = 0;
    }
    ProfCounter(long long val)
    {
        start_val_ = val;
        cycles_ = 0;
    }
    void start()
    {
        start_val_ = prof_get_time_cycle<T>();
        cycles_ = 0;
    }

    ProfCounter& save()
    {
        cycles_ = prof_get_time_cycle<T>() - start_val_;
        return *this;
    }

    ProfCounter& stop_and_save() { return save(); }

    long long stop_val() { return start_val_ + cycles_; }
    long long start_val() { return start_val_; }

    long long cycles() { return cycles_; }
    PROF_ALWAYS_INLINE long long duration_ns() { return (long long)(cycles_ * prof_get_time_inverse_frequency<T>()); }
    double duration_second() { return (double)duration_ns() / (1000.0 * 1000.0 * 1000.0); }

private:
    long long start_val_;
    long long cycles_;
};









template<ProfCounterType T>
long long prof_get_time_cycle()
{
    return 0;
}

template<>
long long prof_get_time_cycle<PROF_COUNTER_RDTSC>()
{
#ifdef WIN32
    _mm_lfence();
    return (long long)__rdtsc();
#else
    unsigned int lo, hi;
    __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) :: );
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}

template<>
long long prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>()
{
#ifdef WIN32
    long long ret;
    _mm_lfence();
    ret = (long long)__rdtsc();
    _mm_lfence();
    return ret;
#else
    unsigned int lo, hi;
    __asm__ __volatile__("lfence;rdtsc;lfence" : "=a" (lo), "=d" (hi) ::);
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}


template<>
long long prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>()
{
#ifdef WIN32
    return (long long)__rdtsc();
#else
    unsigned long hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi) :: );
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#endif
}

template<>
long long prof_get_time_cycle<PROF_COUNTER_RDTSC_PURE>()
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
long long prof_get_time_cycle<PROF_COUNTER_RDTSC_LOCK>()
{
#ifdef WIN32
    _mm_mfence();
    return (long long)__rdtsc();
#else
    unsigned long hi, lo;
    __asm__ ("lock addq $0, 0(%%rsp); rdtsc" : "=a"(lo), "=d"(hi)::"memory");
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#endif
}


template<>
long long prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE>()
{
#ifdef WIN32
    long long ret = 0;
    _mm_mfence();
    ret = (long long)__rdtsc();
    _mm_mfence();
    return ret;
#else
    unsigned int lo, hi;
    __asm__ __volatile__("mfence;rdtsc;mfence" : "=a" (lo), "=d" (hi) ::);
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;
#endif
}

template<>
long long prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE_BTB>()
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
long long prof_get_time_cycle<PROF_COUNTER_RDTSCP>()
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
long long prof_get_time_cycle<PROF_COUNTER_CLOCK>()
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
long long prof_get_time_cycle<PROF_COUNTER_SYS>()
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
long long prof_get_time_cycle<PROF_CONNTER_CHRONO>()
{
    return std::chrono::high_resolution_clock().now().time_since_epoch().count();
}




ProfVM prof_get_mem_use()
{
#ifdef WIN32
    HANDLE hproc = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hproc, &pmc, sizeof(pmc)))
    {
        CloseHandle(hproc);// ignore  
        return { (unsigned long long)pmc.WorkingSetSize, (unsigned long long)pmc.WorkingSetSize};
    }
    return { 0ULL, 0ULL };
#else
    const char* file = "/proc/self/statm";
    FILE* fp = fopen(file, "r");
    if (NULL == fp)
    {
        return { 0ULL, 0ULL };
    }

    char line_buff[256];
    ProfVM vm = { 0ULL, 0ULL, 0ULL };
    while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
    {
        int ret = sscanf(line_buff, "%lld %lld %lld ", &vm.vm_size, &vm.rss_size, &vm.shr_size);
        if (ret == 3)
        {
            vm.vm_size *= 4096;
            vm.rss_size *= 4096; 
            vm.shr_size *= 4096;
            break;
        }
        memset(&vm, 0, sizeof(vm));
        break;
    }
    fclose(fp);
    return vm;
#endif
}

#ifdef WIN32
struct PROF_PROCESSOR_POWER_INFORMATION
{
    ULONG  Number;
    ULONG  MaxMhz;
    ULONG  CurrentMhz;
    ULONG  MhzLimit;
    ULONG  MaxIdleState;
    ULONG  CurrentIdleState;
};
#endif
PROF_ALWAYS_INLINE double prof_get_cpu_mhz()
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
    std::array< PROF_PROCESSOR_POWER_INFORMATION, 128> pppi;
    DWORD dwSize = sizeof(PROF_PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors;
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




template<ProfCounterType T>
double prof_get_time_frequency()
{
    return 1.0;
}

template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSC>()
{
    static double frequency_per_ns = prof_get_cpu_mhz() * 1000.0 * 1000.0 / 1000.0 / 1000.0 / 1000.0;
    return frequency_per_ns;
}
template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSC_BTB>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSC_NOFENCE>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSC_PURE>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSC_LOCK>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSC_MFENCE>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSC_MFENCE_BTB>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
double prof_get_time_frequency<PROF_COUNTER_RDTSCP>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
double prof_get_time_frequency<PROF_COUNTER_CLOCK>()
{
#ifdef WIN32
    double frequency_per_ns = 0;
    long long win_freq = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&win_freq);
    frequency_per_ns = win_freq / 1000.0 / 1000.0 / 1000.0;
    return frequency_per_ns;
#else
    return 1.0;
#endif
}

template<>
double prof_get_time_frequency<PROF_COUNTER_SYS>()
{
    return 1.0;
}

template<>
double prof_get_time_frequency<PROF_CONNTER_CHRONO>()
{
    static double chrono_frequency = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
    return chrono_frequency;
}


template<ProfCounterType T>
double prof_get_time_inverse_frequency()
{
    static double inverse_frequency_per_ns = 1.0 / (prof_get_time_frequency<T>() <= 0.0 ? 1.0 : prof_get_time_frequency<T>());
    return inverse_frequency_per_ns;
}


#endif

/*
* zprof License
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
#include <stdio.h>
#include <cstdarg>
#include <iostream>


#ifndef ZPROF_SERIALIZE_H
#define ZPROF_SERIALIZE_H

#ifdef WIN32
#define PROF_LINE_FEED "\r\n"
#elif (defined __APPLE__)
#define PROF_LINE_FEED "\r\n"
#else
#define PROF_LINE_FEED "\n"
#endif // WIN32


class ProfSerializeBuffer
{
public:
    ProfSerializeBuffer() = delete;
    explicit ProfSerializeBuffer(char* buff, size_t buff_size)
    {
        buff_ = buff;
        buff_len_ = buff_size;
        offset_ = 0;
    }


    inline ProfSerializeBuffer& push_human_count(long long count);
    inline ProfSerializeBuffer& push_human_time(long long ns);
    inline ProfSerializeBuffer& push_human_mem(long long bytes);
    inline ProfSerializeBuffer& push_char(char ch, int repeat = 1);
    inline ProfSerializeBuffer& push_string(const char* str);
    inline ProfSerializeBuffer& push_string(const char* str, size_t size);
    inline ProfSerializeBuffer& push_now_date();
    inline ProfSerializeBuffer& push_number(unsigned long long number, int wide = 0);
    inline ProfSerializeBuffer& push_number(long long number, int wide = 0);

    inline void closing_string();
    bool is_full() { return offset_ + 1 >= buff_len_; } //saved one char  
    char* buff() { return buff_; }
    const char* buff() const { return buff_; }
    size_t offset() { return offset_; }
    size_t offset() const { return offset_; }
    size_t buff_len() { return buff_len_; }
    size_t buff_len()const { return buff_len_; }
    void reset_offset(size_t offset = 0) { offset_ = offset; }
private:
    char* buff_;
    size_t offset_;
    size_t buff_len_;
};



ProfSerializeBuffer& ProfSerializeBuffer::push_human_count(long long count)
{
    if (count > 1000 * 1000)
    {
        push_number((unsigned long long)(count / 1000 / 1000));
        push_char(',');
        push_number((unsigned long long)((count / 1000) % 1000), 3);
        push_char(',');
        push_number((unsigned long long)(count % 1000), 3);
        return *this;
    }
    else if (count > 1000)
    {
        push_number((unsigned long long)(count / 1000));
        push_char(',');
        push_number((unsigned long long)(count % 1000), 3);
        return *this;
    }
    return push_number((unsigned long long)(count));
}

ProfSerializeBuffer& ProfSerializeBuffer::push_human_time(long long ns)
{
    if (buff_len_ <= offset_ + 35)
    {
        return *this;
    }
    long long fr = 1;
    long long mr = 1000;
    char fc = 'n';
    char lc = 's';
    if (ns >= 1000*1000*1000)
    {
        fr = 1000 * 1000 * 1000;
        mr = 1000 * 1000;
        fc = 's';
        lc = ' ';
    }
    else if (ns >= 1000*1000)
    {
        fr = 1000  * 1000;
        mr = 1000;
        fc = 'm';
        lc = 's';
    }
    else if (ns >= 1000)
    {
        fr = 1000;
        mr = 1;
        fc = 'u';
        lc = 's';
    }
    else if (ns < 0) //cost may be over  when long long time  elapse . 
    {
        //ns = 0;
        push_string("invalid");
        return *this;
    }
    push_number((long long)(ns/ fr));
    buff_[offset_++] = '.';
    push_number((unsigned long long)((ns/ mr) % 1000), 3);
    buff_[offset_++] = fc;
    buff_[offset_++] = lc;
    return *this;
}


ProfSerializeBuffer& ProfSerializeBuffer::push_human_mem(long long bytes)
{
    if (bytes > 1024 * 1024 * 1024)
    {
        push_number((unsigned long long)(bytes / 1024 / 1024 / 1024));
        push_char('.');
        push_number((unsigned long long)((bytes / 1024 / 1024) % 1024), 3);
        push_char('G');
        return *this;
    }
    else if (bytes > 1024 * 1024)
    {
        push_number((unsigned long long)(bytes / 1024 / 1024));
        push_char('.');
        push_number((unsigned long long)((bytes / 1024) % 1024), 3);
        push_char('M');
        return *this;
    }
    else if (bytes > 1024)
    {
        push_number((unsigned long long)(bytes / 1024));
        push_char('.');
        push_number((unsigned long long)(bytes % 1024), 3);
        push_char('K');
        return *this;
    }
    else
    {
        push_number((unsigned long long)bytes);
        push_char('B');
    }
    return *this;
}

inline ProfSerializeBuffer& ProfSerializeBuffer::push_char(char ch, int repeat)
{
    while (repeat > 0 && offset_ < buff_len_)
    {
        buff_[offset_++] = ch;
        repeat--;
    }
    return *this;
}

inline ProfSerializeBuffer& ProfSerializeBuffer::push_number(unsigned long long number, int wide)
{
    if (buff_len_ <= offset_ + 30)
    {
        return *this;
    }
    static const char* dec_lut =
        "00010203040506070809"
        "10111213141516171819"
        "20212223242526272829"
        "30313233343536373839"
        "40414243444546474849"
        "50515253545556575859"
        "60616263646566676869"
        "70717273747576777879"
        "80818283848586878889"
        "90919293949596979899";

    static const int buf_len = 30;
    char buf[buf_len];
    int write_index = buf_len;
    unsigned long long m1 = 0;
    unsigned long long m2 = 0;
    do
    {
        m1 = number / 100;
        m2 = number % 100;
        m2 += m2;
        number = m1;
        *(buf + write_index - 1) = dec_lut[m2 + 1];
        *(buf + write_index - 2) = dec_lut[m2];
        write_index -= 2;
    } while (number);
    if (buf[write_index] == '0')
    {
        write_index++;
    }
    while (buf_len - write_index < wide)
    {
        write_index--;
        buf[write_index] = '0';
    }
    memcpy(buff_ + offset_, buf + write_index, buf_len - write_index);
    offset_ += buf_len - write_index;
    return *this;
}

inline ProfSerializeBuffer& ProfSerializeBuffer::push_number(long long number, int wide)
{
    if (buff_len_ <= offset_ + 30)
    {
        return *this;
    }
    if (number < 0)
    {
        buff_[offset_++] = '-';
        number *= -1;
        wide = wide > 1 ? wide - 1 : 0;
    }
    return push_number((unsigned long long)number, wide);
}

inline ProfSerializeBuffer& ProfSerializeBuffer::push_string(const char* str)
{
    return push_string(str, strlen(str));
}
inline ProfSerializeBuffer& ProfSerializeBuffer::push_string(const char* str, size_t size)
{
    if (str == NULL)
    {
        return *this;
    }
    size_t max_size = buff_len_ - offset_ > size ? size : buff_len_ - offset_;
    memcpy(buff_ + offset_, str, max_size);
    offset_ += max_size;
    return *this;
}
inline ProfSerializeBuffer& ProfSerializeBuffer::push_now_date()
{
    time_t timestamp = 0;
    unsigned int precise = 0;
    do
    {
#ifdef _WIN32
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        unsigned long long now = ft.dwHighDateTime;
        now <<= 32;
        now |= ft.dwLowDateTime;
        now /= 10;
        now -= 11644473600000000ULL;
        now /= 1000;
        timestamp = now / 1000;
        precise = (unsigned int)(now % 1000);
#else
        struct timeval tm;
        gettimeofday(&tm, nullptr);
        timestamp = tm.tv_sec;
        precise = tm.tv_usec / 1000;
#endif
    } while (0);

    struct tm tt = { 0 };
#ifdef WIN32
    localtime_s(&tt, &timestamp);
#else 
    localtime_r(&timestamp, &tt);
#endif

    push_char('[');
    push_number((unsigned long long)tt.tm_year + 1900, 4);
    push_number((unsigned long long)tt.tm_mon + 1, 2);
    push_number((unsigned long long)tt.tm_mday, 2);
    push_char(' ');
    push_number((unsigned long long)tt.tm_hour, 2);
    push_char(':');
    push_number((unsigned long long)tt.tm_min, 2);
    push_char(':');
    push_number((unsigned long long)tt.tm_sec, 2);
    push_char('.');
    push_number((unsigned long long)precise, 3);
    push_char(']');
    return *this;
}


inline void ProfSerializeBuffer::closing_string()
{
    size_t closed_id = offset_ >= buff_len_ ? offset_ - 1 : offset_;
    buff_[closed_id] = '\0';
}

#endif

/*
* zprof License
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

#include <algorithm>
#include <functional>
#include <atomic>
#ifndef ZPROF_RECORD_H
#define ZPROF_RECORD_H




#define PROF_MAX_DEPTH 5

#define SMOOTH_CYCLES(s_cost, cost) (   (s_cost * 12 + cost * 4) >> 4   ) 
#define SMOOTH_CYCLES_WITH_INIT(s_cost, cost) ( (s_cost) == 0 ? (cost) : SMOOTH_CYCLES(s_cost, cost) )

enum ProfLevel
{
    PROF_LEVEL_NORMAL,
    PROF_LEVEL_FAST,
    PROF_LEVEL_FULL,
};


struct ProfDesc
{
    int node_name;
    int node_name_len;
    int counter_type;
    bool resident;
};

struct ProfCPU
{
    long long c; 
    long long sum;  
    long long dv; 
    long long sm;
    long long h_sm;
    long long l_sm;
    long long max_u;
    long long min_u;
    long long t_u;
};

struct ProfTimer
{
    long long last;
};

struct ProfMEM
{
    long long c;  
    long long sum;
    long long delta;
    long long t_u;
};


struct ProfUser
{
    long long c;
    long long sum;
    long long t_u;
};

struct ProfNode
{
    bool active;  
    int parrent;
    int jump_child;
    int first_child;
    int child_count; 
    int merge_to;
    int merge_child_count;
    int merge_current_child_count;

    ProfCPU cpu; 
    ProfMEM mem; 
    ProfTimer timer;
    ProfUser user;
    ProfVM vm;
};  

enum ProfSerializeFlags : unsigned int
{
    PROF_SER_NULL,
    PROF_SER_INNER = 0x1,
    PROF_SER_RESERVE = 0x2,
    PROF_SER_DELCARE = 0x4,
};


#ifdef _FN_LOG_LOG_H_
static inline void ProfDefaultFNLogFunc(const ProfSerializeBuffer& buffer)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(buffer.buff(), (int)buffer.offset());
}
#endif

template<int INST, int RESERVE, int DECLARE>
class ProfRecord 
{
public:
    using DefaultLogFunc = void(*)(const ProfSerializeBuffer& buffer);
    enum InnerType
    {
        INNER_PROF_NULL,
        INNER_PROF_INIT_COST,
        INNER_PROF_SERIALIZE_COST,
        INNER_PROF_SERIALIZE_MERGE_COST,
        INNER_PROF_SINGLE_SERIALIZE_COST,
        INNER_PROF_SINGLE_WRITE_LOG_COST,
        INNER_PROF_MERGE_ALL_COST,
        INNER_PROF_SELF_MEM_COST,
        INNER_PROF_AUTO_TEST_COST,
        INNER_PROF_FULL_AUTO_COST,
        INNER_PROF_COUNTER_COST,
        INNER_PROF_ORIGIN_INC,
        INNER_PROF_ATOM_RELEAX,
        INNER_PROF_ATOM_COST,
        INNER_PROF_ATOM_SEQ_COST,
        INNER_PROF_MAX,
    };

    static constexpr int inst_id() { return INST; }



    static constexpr int node_reserve_begin_id() { return INNER_PROF_MAX; }
    static constexpr int node_reserve_count() { return RESERVE; }
    static constexpr int node_reserve_end_id() { return node_reserve_begin_id() + node_reserve_count(); }

    static constexpr int node_declare_begin_id() { return node_reserve_end_id(); }
    static constexpr int node_declare_count() { return DECLARE; }
    static constexpr int node_declare_end_id() { return node_declare_begin_id() + node_declare_count(); }
    inline int node_delcare_reg_end_id() { return declare_reg_end_id_; }

    static constexpr int node_begin_id() { return INNER_PROF_NULL + 1; }
    static constexpr int node_count() { return node_declare_end_id() - 1; }
    static constexpr int node_end_id() { return node_begin_id() + node_count(); }
    static constexpr int max_node_count() { return node_count(); }

    static constexpr int max_serialize_buff_size() { return 1000; }
    static constexpr int max_compact_string_size() { return 30 * (1+node_end_id()); } //reserve node no name 
    static_assert(node_end_id() == INNER_PROF_MAX + node_reserve_count() + node_declare_count(), "");

public:
    long long init_timestamp_;
    long long last_timestamp_;
public:

    ProfRecord() : compact_buffer_(compact_string_, max_compact_string_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        memset(node_descs_, 0, sizeof(node_descs_));
        merge_to_size_ = 0;
        memset(circles_per_ns_, 0, sizeof(circles_per_ns_));
        declare_reg_end_id_ = node_declare_begin_id();

        log_func_ = NULL;

#ifdef _FN_LOG_LOG_H_
        log_func_ = &ProfDefaultFNLogFunc;  //set default log;
#endif


        serialize_buff_[0] = '\0';
        init_timestamp_ = 0;
        last_timestamp_ = 0;
        static_assert(max_compact_string_size() > 150, "");
        unknown_desc_ = 0;
        compact_buffer_.push_string("unknown");
        compact_buffer_.push_char('\0');
        reserve_desc_ = (int)compact_buffer_.offset();
        compact_buffer_.push_string("reserve");
        compact_buffer_.push_char('\0');
        no_name_space_ = (int)compact_buffer_.offset();
        compact_buffer_.push_string("the string of store name is too small..");
        no_name_space_len_ = (int)(compact_buffer_.offset() - no_name_space_);
        compact_buffer_.push_char('\0');
        desc_ = 0;

    };
    static inline ProfRecord& instance()
    {
        static ProfRecord inst;
        return inst;
    }
    inline int init_prof(const char* desc);
    inline int init_jump_count();
    inline int regist_node(int idx, const char* desc, unsigned int counter, bool resident, bool re_reg);
    inline int rename_node(int idx, const char* desc);
    inline const char* node_name(int idx);
    inline int bind_childs(int idx, int child);
    inline int bind_merge(int idx, int to);

    PROF_ALWAYS_INLINE void reset_cpu(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.cpu, 0, sizeof(node.cpu));
        node.cpu.min_u = LLONG_MAX;
    }
    PROF_ALWAYS_INLINE void reset_mem(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.mem, 0, sizeof(node.mem));
    }
    PROF_ALWAYS_INLINE void reset_vm(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.vm, 0, sizeof(node.vm));
    }
    PROF_ALWAYS_INLINE void reset_timer(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.timer, 0, sizeof(node.timer));
    }
    PROF_ALWAYS_INLINE void reset_user(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.user, 0, sizeof(node.user));
    }
    PROF_ALWAYS_INLINE void reset_node(int idx)
    {
        reset_cpu(idx);
        reset_mem(idx);
        reset_vm(idx);
        reset_timer(idx);
        reset_user(idx);
    }
    void clean_node_info_range(int first_idx, int end_idx, bool keep_resident = true)
    {
        for (int idx = first_idx; idx < end_idx; idx++)
        {
            if (!keep_resident || !node_descs_[idx].resident)
            {
                reset_node(idx);
            }
        }
    }

    void clean_reserve_info(bool keep_resident = true)
    {
        clean_node_info_range(node_reserve_begin_id(), node_reserve_end_id(), keep_resident);
        last_timestamp_ = time(NULL);
    }

    void clean_declare_info(bool keep_resident = true)
    {
        clean_node_info_range(node_declare_begin_id(), node_declare_end_id(), keep_resident);
        last_timestamp_ = time(NULL);
    }



    inline void reset_childs(int idx, int depth = 0);

    PROF_ALWAYS_INLINE void call_cpu(int idx, long long c, long long cost)
    {
        long long dis = cost / c;
        ProfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        node.cpu.sm = (int)SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.max_u = (int)(node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (int)(node.cpu.min_u < dis ? node.cpu.min_u : dis);
        node.cpu.dv += abs(dis - node.cpu.sum/node.cpu.c);
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void call_cpu(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = (int)SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.max_u = (int)(node.cpu.max_u < cost ? cost : node.cpu.max_u);
        node.cpu.min_u = (int)(node.cpu.min_u < cost ? node.cpu.min_u : cost);
        node.cpu.dv += abs(cost - node.cpu.sm);
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void call_cpu_no_sm(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = (int)cost;
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void call_cpu_no_sm(int idx, long long count, long long cost)
    {
        long long dis = cost / count;
        ProfNode& node = nodes_[idx];
        node.cpu.c += count;
        node.cpu.sum += cost;
        node.cpu.sm = (int)dis;
        node.cpu.t_u += cost;
    }

    PROF_ALWAYS_INLINE void call_cpu_full(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        long long dis = cost;
        long long avg = node.cpu.sum / node.cpu.c;

        node.cpu.sm = (int)SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.h_sm = (int)(dis > avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
        node.cpu.l_sm = (int)(dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_u += cost;
        node.cpu.max_u = (int)(node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (int)(node.cpu.min_u < dis ? node.cpu.min_u : dis);
    }

    PROF_ALWAYS_INLINE void call_cpu_full(int idx, long long c, long long cost)
    {
        
        ProfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        long long dis = cost / c;
        long long avg = node.cpu.sum / node.cpu.c;

        node.cpu.sm = (int)SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.h_sm = (int) (dis > avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
        node.cpu.l_sm = (int) (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_u += cost;
        node.cpu.max_u = (int)(node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (int)(node.cpu.min_u < dis ? node.cpu.min_u : dis);
    }


    PROF_ALWAYS_INLINE void call_timer(int idx, long long stamp)
    {
        ProfNode& node = nodes_[idx];
        if (node.timer.last == 0)
        {
            node.timer.last = stamp;
            return;
        }
        call_cpu_full(idx, 1, stamp - node.timer.last);
        node.timer.last = stamp;
    }

    PROF_ALWAYS_INLINE void call_mem(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.mem.c += c;
        node.mem.sum += add;
        node.mem.t_u += add;
    }
    PROF_ALWAYS_INLINE void call_vm(int idx, const ProfVM& vm)
    {
        nodes_[idx].vm = vm;
    }
    PROF_ALWAYS_INLINE void call_user(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.user.c += c;
        node.user.sum += add;
        node.user.t_u += add;
    }

    PROF_ALWAYS_INLINE void refresh_mem(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.mem.c = c;
        node.mem.delta = add - node.mem.sum;
        node.mem.sum = add;
        node.mem.t_u = add;
    }


    void update_merge()
    {
        ProfCounter<PROF_COUNTER_DEFAULT> cost;
        cost.start();
        for (int i = 0; i < merge_to_size_; i++)
        {
            ProfNode& leaf = nodes_[merge_to_[i]];
            ProfNode* node = NULL;
            long long append_cpu = 0;
            long long append_mem = 0;
            long long append_user = 0;
            int node_id = 0;
            node = &nodes_[leaf.merge_to];
            append_cpu = leaf.cpu.t_u;
            append_mem = leaf.mem.t_u;
            append_user = leaf.user.t_u;
            node_id = leaf.merge_to;
            leaf.cpu.t_u = 0;
            leaf.mem.t_u = 0;
            leaf.user.t_u = 0;
            do
            {
                node->cpu.t_u += append_cpu;
                node->mem.t_u += append_mem;
                node->user.t_u += append_user;
                node->merge_current_child_count++;
                if (node->merge_current_child_count >= node->merge_child_count)
                {
                    node->merge_current_child_count = 0;
                    append_cpu = node->cpu.t_u;
                    append_mem = node->mem.t_u;
                    append_user = node->user.t_u;
                    if (append_cpu > 0)
                    {
                        call_cpu_full(node_id, append_cpu);
                    }
                    if (append_mem > 0)
                    {
                        call_mem(node_id, 1, append_mem);
                    }
                    if (append_user > 0)
                    {
                        call_user(node_id, 1, append_user);
                    }
                    node->cpu.t_u = 0;
                    node->mem.t_u = 0;
                    node->user.t_u = 0;
                    if (node->merge_to == 0)
                    {
                        break;
                    }
                    node_id = node->merge_to;
                    node = &nodes_[node->merge_to];
                    continue;
                }
                break;
            } while (true);
        }
        call_cpu(INNER_PROF_MERGE_ALL_COST, cost.stop_and_save().cycles());
    }
    int serialize_root(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, ProfSerializeBuffer& buffer, std::function<void(const ProfSerializeBuffer& buffer)> call_log = NULL);
    ProfSerializeBuffer serialize_root(int entry_idx, std::function<void(const ProfSerializeBuffer& buffer)> call_log = NULL);
    int serialize(unsigned int flags, std::function<void(const ProfSerializeBuffer& buffer)> call_log);


    ProfNode& node(int idx) { return nodes_[idx]; }
    ProfDesc& node_desc(int idx) { return node_descs_[idx]; }
    const char* desc() const { return &compact_string_[desc_]; }
    double circles_per_ns(int t) { return  circles_per_ns_[t == PROF_COUNTER_NULL ? PROF_COUNTER_DEFAULT : t]; }

public:
    ProfSerializeBuffer& compact_buffer() { return compact_buffer_; }
    char* serialize_buffer() { return serialize_buff_; }
public:
    void set_default_log_func(DefaultLogFunc func) { log_func_ = func; }
    DefaultLogFunc default_log_func() { return log_func_; }
private:
    DefaultLogFunc log_func_;
private:
    ProfNode nodes_[node_end_id()];
    ProfDesc node_descs_[node_end_id()];
    int desc_;
    std::array<int, node_end_id()> merge_to_;
    int merge_to_size_;
    double circles_per_ns_[PROF_COUNTER_MAX];
    int declare_reg_end_id_;
    char serialize_buff_[max_serialize_buff_size()];
    char compact_string_[max_compact_string_size()];
    ProfSerializeBuffer compact_buffer_;
    int unknown_desc_;
    int reserve_desc_;
    int no_name_space_;
    int no_name_space_len_;
};



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::bind_childs(int idx, int cidx)
{
    if (idx < node_begin_id() || idx >= node_end_id() || cidx < node_begin_id() || cidx >= node_end_id())
    {
        return -1;
    }

    if (idx == cidx)
    {
        return -2;  
    }

    ProfNode& node = nodes_[idx];
    ProfNode& child = nodes_[cidx];
    if (!node.active || !child.active)
    {
        return -3; //regist method has memset all info ; 
    }
    if (node.first_child == 0)
    {
        node.first_child = cidx;
        node.child_count = 1;
    }
    else 
    {
        if (cidx < node.first_child)
        {
            node.child_count += node.first_child - cidx;
            node.first_child = cidx;
        }
        else if (cidx >= node.first_child + node.child_count)
        {
            node.child_count = cidx - node.first_child + 1;
        }
    }
    
    child.parrent = idx;
    return 0;
}



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::bind_merge(int idx, int to)
{
    if (idx < node_begin_id() || idx >= node_end_id() || to < node_begin_id() || to >= node_end_id())
    {
        return -1;
    }

    if (idx == to)
    {
        return -2;  
    }
    if (merge_to_size_ >= node_end_id())
    {
        return -3;
    }
    ProfNode& node = nodes_[idx];
    ProfNode& to_node = nodes_[to];
    if (!node.active || !to_node.active)
    {
        return -3; //regist method has memset all info ; 
    }
    to_node.merge_child_count++;
    if (to_node.merge_child_count == 1 && to_node.merge_to != 0)
    {
        for (int i = 0; i < merge_to_size_; i++)
        {
            if (merge_to_[i] == to)
            {
                node.merge_to = to;
                merge_to_[i] = idx;
                return 0;
            }
        }
    }

    node.merge_to = to;
    merge_to_[merge_to_size_++] = idx;
    return 0;
}


template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::init_prof(const char* desc)
{
    if (desc == NULL || compact_buffer_.is_full())
    {
        desc_ = 0;
    }
    else
    {
        desc_ = (int)compact_buffer_.offset();
        compact_buffer_.push_string("desc");
        compact_buffer_.push_char('\0');
        compact_buffer_.closing_string();
    }
    ProfCounter<> counter;
    counter.start();

    last_timestamp_ = time(NULL);
    init_timestamp_ = time(NULL);

    circles_per_ns_[PROF_COUNTER_NULL] = 0;
    circles_per_ns_[PROF_COUNTER_SYS] = prof_get_time_inverse_frequency<PROF_COUNTER_SYS>();
    circles_per_ns_[PROF_COUNTER_CLOCK] = prof_get_time_inverse_frequency<PROF_COUNTER_CLOCK>();
    circles_per_ns_[PROF_CONNTER_CHRONO] = prof_get_time_inverse_frequency<PROF_CONNTER_CHRONO>();
    circles_per_ns_[PROF_COUNTER_RDTSC] = prof_get_time_inverse_frequency<PROF_COUNTER_RDTSC>();
    circles_per_ns_[PROF_COUNTER_RDTSC_BTB] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSCP] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_MFENCE] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_MFENCE_BTB] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_NOFENCE] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_PURE] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_LOCK] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_NULL] = circles_per_ns_[PROF_COUNTER_DEFAULT];

    for (int i = node_begin_id(); i < node_reserve_end_id(); i++)
    {
        regist_node(i, "reserve", PROF_COUNTER_DEFAULT, false, false);
    }

    regist_node(INNER_PROF_NULL, "INNER_PROF_NULL", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_INIT_COST, "INNER_PROF_INIT_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_SERIALIZE_COST, "INNER_PROF_SERIALIZE_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_SERIALIZE_MERGE_COST, "INNER_PROF_SERIALIZE_MERGE_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_SINGLE_SERIALIZE_COST, "INNER_PROF_SINGLE_SERIALIZE_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_SINGLE_WRITE_LOG_COST, "INNER_PROF_SINGLE_WRITE_LOG_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_MERGE_ALL_COST, "INNER_PROF_MERGE_ALL_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_SELF_MEM_COST, "INNER_PROF_SELF_MEM_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_AUTO_TEST_COST, "INNER_PROF_AUTO_TEST_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_FULL_AUTO_COST, "INNER_PROF_FULL_AUTO_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_COUNTER_COST, "INNER_PROF_COUNTER_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_ORIGIN_INC, "INNER_PROF_ORIGIN_INC", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_ATOM_RELEAX, "INNER_PROF_ATOM_RELEAX", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_ATOM_COST, "INNER_PROF_ATOM_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_ATOM_SEQ_COST, "INNER_PROF_ATOM_SEQ_COST", PROF_COUNTER_DEFAULT, true, true);

    bind_childs(INNER_PROF_SERIALIZE_MERGE_COST, INNER_PROF_SINGLE_SERIALIZE_COST);
    bind_merge(INNER_PROF_SINGLE_SERIALIZE_COST, INNER_PROF_SERIALIZE_MERGE_COST);
    bind_childs(INNER_PROF_SERIALIZE_MERGE_COST, INNER_PROF_SINGLE_WRITE_LOG_COST);
    bind_merge(INNER_PROF_SINGLE_WRITE_LOG_COST, INNER_PROF_SERIALIZE_MERGE_COST);


    if (true)
    {
        ProfCounter<> self_mem_cost;
        self_mem_cost.start();
        call_vm(INNER_PROF_SELF_MEM_COST, prof_get_mem_use());
        call_cpu(INNER_PROF_SELF_MEM_COST, self_mem_cost.stop_and_save().cycles());
        call_mem(INNER_PROF_SELF_MEM_COST, 1, sizeof(*this));
        call_user(INNER_PROF_SELF_MEM_COST, 1, max_node_count());
    }

    if (true)
    {
        ProfCounter<> cost;
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            ProfCounter<> test_cost;
            test_cost.start();
            test_cost.stop_and_save();
            call_cpu(INNER_PROF_AUTO_TEST_COST, test_cost.cycles());
        }
        call_cpu(INNER_PROF_FULL_AUTO_COST, 1000, cost.stop_and_save().cycles());
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            call_cpu_no_sm(INNER_PROF_AUTO_TEST_COST, cost.stop_and_save().cycles());
        }
        call_cpu(INNER_PROF_COUNTER_COST, 1000, cost.stop_and_save().cycles());
        std::atomic<long long> atomll_test(0);
        volatile long long origin_feetch_add_test = 0;
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            origin_feetch_add_test++;
        }
        call_cpu(INNER_PROF_ORIGIN_INC, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_relaxed);
        }
        call_cpu(INNER_PROF_ATOM_RELEAX, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test++;
        }
        call_cpu(INNER_PROF_ATOM_COST, 1000, cost.stop_and_save().cycles());

        
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_seq_cst);
        }
        call_cpu(INNER_PROF_ATOM_SEQ_COST, 1000, cost.stop_and_save().cycles());

        call_cpu(INNER_PROF_AUTO_TEST_COST, origin_feetch_add_test);
        call_cpu(INNER_PROF_AUTO_TEST_COST, atomll_test.load());
        reset_node(INNER_PROF_AUTO_TEST_COST);
    }

    call_cpu(INNER_PROF_INIT_COST, counter.stop_and_save().cycles());

    return 0;
}


template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::init_jump_count()
{
    for (int i = node_declare_begin_id(); i < node_declare_end_id(); )
    {
        int next_parrent_id = i + 1;
        while (next_parrent_id < node_declare_end_id())
        {
            if (nodes_[next_parrent_id].parrent == 0)
            {
                break;
            }
            next_parrent_id++;
        }
        for (int j = i; j < next_parrent_id; j++)
        {
            nodes_[j].jump_child = next_parrent_id - j - 1;
        }
        i = next_parrent_id;
    }
    return 0;
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::regist_node(int idx, const char* desc, unsigned int counter_type, bool resident, bool re_reg)
{
    if (idx >= node_end_id() )
    {
        return -1;
    }
    if (desc == NULL)
    {
        return -3;
    }
    
    
    ProfNode& node = nodes_[idx];


    if (!re_reg && node.active)
    {
        return 0;
    }

    memset(&node, 0, sizeof(node));
    rename_node(idx, desc);
    node_descs_[idx].counter_type = counter_type;
    node_descs_[idx].resident = resident;
    node.active = true;
    node.cpu.min_u = LLONG_MAX;

    if (idx >= node_declare_begin_id() && idx < node_declare_end_id() && idx + 1 > declare_reg_end_id_)
    {
        declare_reg_end_id_ = idx + 1;
    }

    return 0;
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::rename_node(int idx, const char* desc)
{
    if (idx < node_begin_id() || idx >= node_end_id() )
    {
        return -1;
    }
    if (desc == NULL)
    {
        return -3;
    }
    if (strcmp(desc, "reserve") == 0)
    {
        node_descs_[idx].node_name = reserve_desc_;
        node_descs_[idx].node_name_len = 7;
        return 0;
    }


    node_descs_[idx].node_name = (int)compact_buffer_.offset();// node name is "" when compact buffer full 
    compact_buffer_.push_string(desc);
    compact_buffer_.push_char('\0');
    compact_buffer_.closing_string();
    node_descs_[idx].node_name_len = (int)strlen(&compact_string_[node_descs_[idx].node_name]);
    if (node_descs_[idx].node_name_len == 0)
    {
        node_descs_[idx].node_name = no_name_space_;
        node_descs_[idx].node_name_len = no_name_space_len_;
    }
    return 0;
}


template<int INST, int RESERVE, int DECLARE>
const char* ProfRecord<INST, RESERVE, DECLARE>::node_name(int idx)
{
    if (idx < node_begin_id() || idx >= node_end_id())
    {
        return "";
    }
    ProfDesc& desc = node_descs_[idx];
    if (desc.node_name >= max_compact_string_size())
    {
        return "";
    }
    return &compact_string_[desc.node_name];
};


template<int INST, int RESERVE, int DECLARE>
void ProfRecord<INST, RESERVE, DECLARE>::reset_childs(int idx, int depth)
{
    if (idx < node_begin_id() || idx >= node_end_id())
    {
        return ;
    }
    ProfNode& node = nodes_[idx];
    reset_cpu(idx);
    reset_mem(idx);
    reset_timer(idx);
    reset_user(idx);
    if (depth > PROF_MAX_DEPTH)
    {
        return;
    }
    for (int i = node.first_child; i < node.first_child + node.child_count; i++)
    {
        ProfNode& child = nodes_[i];
        if (child.parrent == idx)
        {
           reset_childs(i, depth + 1);
        }
    }
}



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::serialize_root(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, ProfSerializeBuffer& buffer, std::function<void(const ProfSerializeBuffer& buffer)> call_log)
{
    if (entry_idx >= node_end_id())
    {
        return -1;
    }

    const int min_line_size = 120;
    if (buffer.buff_len() <= min_line_size)
    {
        return -2;
    }
    if (call_log == NULL && log_func_ != NULL )
    {
        call_log = log_func_;
    }

    if (buffer.offset() + min_line_size >= buffer.buff_len())
    {
        buffer.reset_offset(buffer.buff_len() - min_line_size);
        buffer.push_char(' ', depth * 2);
        buffer.push_string("serialize buffer too short ..." PROF_LINE_FEED);
        buffer.closing_string();
        if (call_log)
        {
            call_log(buffer);
            buffer.reset_offset();
        }
        return -3;
    }


    ProfNode& node = nodes_[entry_idx];

    if (depth == 0 && node.parrent)
    {
        return 0;
    }
    if (!node.active)
    {
        return 0;
    }
    if (node_descs_[entry_idx].node_name + node_descs_[entry_idx].node_name_len >= max_compact_string_size())
    {
        return 0;
    }
    if (node_descs_[entry_idx].counter_type >= PROF_COUNTER_MAX)
    {
        return 0;
    }
    const char* desc_name = &compact_string_[node_descs_[entry_idx].node_name];
    size_t desc_len = node_descs_[entry_idx].node_name_len;
    double cpu_rate = circles_per_ns(node_descs_[entry_idx].counter_type);
    if (opt_name != NULL)
    {
        desc_name = opt_name;
        desc_len = opt_name_len;
    }

    ProfCounter<> cost_single_serialize;
    cost_single_serialize.start();

    int name_blank = (int)desc_len + depth  + depth;
    name_blank = name_blank < 35 ? 35 - name_blank : 0;


#define STRLEN(str) str, strlen(str)
    if (node.cpu.c > 0)
    {
        cost_single_serialize.start();
        buffer.push_char(' ', depth * 2);
        buffer.push_string(STRLEN("|"));
        buffer.push_number((unsigned long long)entry_idx, 3);
        buffer.push_string(STRLEN("| "));
        buffer.push_string(desc_name, desc_len);
        buffer.push_char('-', name_blank);
        buffer.push_string(STRLEN(" |"));


        buffer.push_string(STRLEN("\tcpu*|-- "));
        if (true)
        {
            buffer.push_human_count(node.cpu.c);
            buffer.push_string(STRLEN("c, "));
            buffer.push_human_time((long long)(node.cpu.sum * cpu_rate / node.cpu.c));
            buffer.push_string(STRLEN(", "));
            buffer.push_human_time((long long)(node.cpu.sum * cpu_rate));
        }

        buffer.push_string(STRLEN(" --|*\t\t|-- "));
        if (node.cpu.min_u != LLONG_MAX && node.cpu.max_u > 0)
        {
            buffer.push_human_time((long long)(node.cpu.max_u * cpu_rate));
            buffer.push_string(STRLEN(", "));
            buffer.push_human_time((long long)(node.cpu.min_u * cpu_rate));
        }

        buffer.push_string(STRLEN(" --|  |-- "));
        if (node.cpu.dv > 0 || node.cpu.sm > 0)
        {
            buffer.push_human_time((long long)(node.cpu.dv * cpu_rate / node.cpu.c));
            buffer.push_string(STRLEN(", "));
            buffer.push_human_time((long long)(node.cpu.sm * cpu_rate));
        }

        buffer.push_string(STRLEN(" --||-- "));
        if (node.cpu.h_sm > 0 || node.cpu.l_sm > 0)
        {
            buffer.push_human_time((long long)(node.cpu.h_sm * cpu_rate));
            buffer.push_string(STRLEN(", "));
            buffer.push_human_time((long long)(node.cpu.l_sm * cpu_rate));
        }
        buffer.push_string(STRLEN(" --|"));
        buffer.push_string(STRLEN(PROF_LINE_FEED));
        buffer.closing_string();
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SINGLE_SERIALIZE_COST, cost_single_serialize.cycles());
        if (call_log)
        {
            cost_single_serialize.start();
            call_log(buffer);
            buffer.reset_offset();
            cost_single_serialize.stop_and_save();
            call_cpu_full(INNER_PROF_SINGLE_WRITE_LOG_COST, cost_single_serialize.cycles());
        }
    }

    if (node.mem.c > 0)
    {
        cost_single_serialize.start();
        buffer.push_char(' ', depth * 2);
        buffer.push_string(STRLEN("|"));
        buffer.push_number((unsigned long long)entry_idx, 3);
        buffer.push_string(STRLEN("| "));
        buffer.push_string(desc_name, desc_len);
        buffer.push_char('-', name_blank);
        buffer.push_string(STRLEN(" |"));
 

        buffer.push_string(STRLEN("\tmem*|-- "));
        if (true)
        {
            buffer.push_human_count(node.mem.c);
            buffer.push_string(STRLEN("c, "));
            buffer.push_human_mem(node.mem.sum / node.mem.c);
            buffer.push_string(STRLEN(", "));
            buffer.push_human_mem(node.mem.sum);
        }

        buffer.push_string(STRLEN(" --||-- "));
        if (node.mem.delta > 0)
        {
            buffer.push_human_mem(node.mem.sum - node.mem.delta);
            buffer.push_string(STRLEN(", "));
            buffer.push_human_mem(node.mem.delta);
        }
        buffer.push_string(STRLEN(" --|"));
        buffer.push_string(STRLEN(PROF_LINE_FEED));
        buffer.closing_string();
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SINGLE_SERIALIZE_COST, cost_single_serialize.cycles());
        if (call_log)
        {
            cost_single_serialize.start();
            call_log(buffer);
            buffer.reset_offset();
            cost_single_serialize.stop_and_save();
            call_cpu_full(INNER_PROF_SINGLE_WRITE_LOG_COST, cost_single_serialize.cycles());
        }

    }

    if (node.vm.rss_size + node.vm.vm_size > 0)
    {
        cost_single_serialize.start();
        buffer.push_char(' ', depth * 2);
        buffer.push_string(STRLEN("|"));
        buffer.push_number((unsigned long long)entry_idx, 3);
        buffer.push_string(STRLEN("| "));
        buffer.push_string(desc_name, desc_len);
        buffer.push_char('-', name_blank);
        buffer.push_string(STRLEN(" |"));


        buffer.push_string(STRLEN("\t vm*|-- "));
        if (true)
        {
            buffer.push_human_mem(node.vm.vm_size);
            buffer.push_string(STRLEN("(vm), "));
            buffer.push_human_mem(node.vm.rss_size);
            buffer.push_string(STRLEN("(rss), "));
            buffer.push_human_mem(node.vm.shr_size);
            buffer.push_string(STRLEN("(shr), "));
            buffer.push_human_mem(node.vm.rss_size - node.vm.shr_size);
            buffer.push_string(STRLEN("(uss)"));
        }

        buffer.push_string(STRLEN(" --|"));
        buffer.push_string(STRLEN(PROF_LINE_FEED));
        buffer.closing_string();
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SINGLE_SERIALIZE_COST, cost_single_serialize.cycles());
        if (call_log)
        {
            cost_single_serialize.start();
            call_log(buffer);
            buffer.reset_offset();
            cost_single_serialize.stop_and_save();
            call_cpu_full(INNER_PROF_SINGLE_WRITE_LOG_COST, cost_single_serialize.cycles());
        }
    }

    if (node.user.c > 0)
    {
        cost_single_serialize.start();
        buffer.push_char(' ', depth * 2);
        buffer.push_string(STRLEN("|"));
        buffer.push_number((unsigned long long)entry_idx, 3);
        buffer.push_string(STRLEN("| "));
        buffer.push_string(desc_name, desc_len);
        buffer.push_char('-', name_blank);
        buffer.push_string(STRLEN(" |"));


        buffer.push_string(STRLEN("\tuser*|-- "));
        if (true)
        {
            buffer.push_human_count(node.user.c);
            buffer.push_string(STRLEN("c, "));
            buffer.push_human_count(node.user.sum / node.user.c);
            buffer.push_string(STRLEN(", "));
            buffer.push_human_count(node.user.sum);
        }

        buffer.push_string(STRLEN(" --|"));
        buffer.push_string(STRLEN(PROF_LINE_FEED));
        buffer.closing_string();
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SINGLE_SERIALIZE_COST, cost_single_serialize.cycles());
        if (call_log)
        {
            cost_single_serialize.start();
            call_log(buffer);
            buffer.reset_offset();
            cost_single_serialize.stop_and_save();
            call_cpu_full(INNER_PROF_SINGLE_WRITE_LOG_COST, cost_single_serialize.cycles());
        }
    }
    if (depth > PROF_MAX_DEPTH)
    {
        buffer.push_char(' ', depth * 2);
        buffer.push_string("more node in here ... " PROF_LINE_FEED);
        buffer.closing_string();
        if (call_log)
        {
            call_log(buffer);
            buffer.reset_offset();
        }
        return -4;
    }

    for (int i = node.first_child; i < node.first_child + node.child_count; i++)
    {
        ProfNode& child = nodes_[i];
        if (child.parrent == entry_idx)
        {
            int ret = serialize_root(i, depth + 1, NULL, 0, buffer, call_log);
            if (ret < 0)
            {
                return ret;
            }
        }
    }

    return 0;
}






template<int INST, int RESERVE, int DECLARE>
ProfSerializeBuffer ProfRecord<INST, RESERVE, DECLARE>::serialize_root(int entry_idx, std::function<void(const ProfSerializeBuffer& buffer)> call_log)
{
    ProfSerializeBuffer buffer(serialize_buff_, sizeof(serialize_buff_));
    int ret = serialize_root(entry_idx, 0, NULL, 0, buffer, call_log);
    (void)ret;
    buffer.closing_string();
    return buffer;
}



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::serialize(unsigned int flags, std::function<void(const ProfSerializeBuffer& buffer)> call_log)
{
    if (!call_log && log_func_)
    {
        call_log = log_func_;
    }

    if (!call_log)
    {
        return -1;
    }
    ProfCounter<> cost;
    cost.start();
    ProfSerializeBuffer buffer(serialize_buff_, sizeof(serialize_buff_));

    buffer.reset_offset();
    buffer.push_string(STRLEN(PROF_LINE_FEED));
    buffer.closing_string();
    call_log(buffer);
    buffer.reset_offset();


    buffer.push_char('=', 30);
    buffer.push_char('\t');
    buffer.push_string(desc());
    buffer.push_string(STRLEN(" begin serialize: "));
    buffer.push_now_date();
    buffer.push_char('\t');
    buffer.push_char('=', 30);
    buffer.push_string(STRLEN(PROF_LINE_FEED));
    buffer.push_string(STRLEN("| -- index -- | ---    cpu  ------------ | ----------   hits, avg, sum   ---------- | ---- max, min ---- | ------ dv, sm ------ |  --- hsm, lsm --- | " PROF_LINE_FEED));
    buffer.push_string(STRLEN("| -- index -- | ---    mem  ---------- | ----------   hits, avg, sum   ---------- | ------ last, delta ------ | " PROF_LINE_FEED));
    buffer.push_string(STRLEN("| -- index -- | ---    vm  ------------ | ----------   vm, rss, shr, uss   ------------------ | " ));
    buffer.closing_string();
    call_log(buffer);
    buffer.reset_offset();
    buffer.push_string(STRLEN("| -- index -- | ---    user  ----------- | -----------  hits, avg, sum   ---------- | " PROF_LINE_FEED));
    buffer.closing_string();
    call_log(buffer);

    if (flags & PROF_SER_INNER)
    {
        buffer.reset_offset();
        buffer.push_string(STRLEN(PROF_LINE_FEED));
        buffer.closing_string();
        call_log(buffer);
        buffer.reset_offset();
        for (int i = INNER_PROF_NULL + 1; i < INNER_PROF_MAX; i++)
        {
            int ret = serialize_root(i, 0, NULL, 0, buffer, call_log);
            (void)ret;
        }
    }

    if (flags & PROF_SER_RESERVE)
    {
        buffer.reset_offset();
        buffer.push_string(STRLEN(PROF_LINE_FEED));
        buffer.closing_string();
        buffer.reset_offset();
        for (int i = node_reserve_begin_id(); i < node_reserve_end_id(); i++)
        {
            int ret = serialize_root(i, 0, NULL, 0, buffer, call_log);
            (void)ret;
        }
    }
    
    if (flags & PROF_SER_DELCARE)
    {
        buffer.reset_offset();
        buffer.push_string(STRLEN(PROF_LINE_FEED));
        buffer.closing_string();
        buffer.reset_offset();
        for (int i = node_declare_begin_id(); i < node_delcare_reg_end_id(); )
        {
            int ret = serialize_root(i, 0, NULL, 0, buffer, call_log);
            (void)ret;
            i += nodes_[i].jump_child + 1;
        }
    }

    buffer.reset_offset();
    buffer.push_char('=', 30);
    buffer.push_char('\t');
    buffer.push_string(" end : ");
    buffer.push_now_date();
    buffer.push_char('\t');
    buffer.push_char('=', 30);
    buffer.push_string(STRLEN(PROF_LINE_FEED));
    buffer.closing_string();
    call_log(buffer);
    buffer.reset_offset();
    /*
    buffer.push_char('=', 120);
    buffer.push_string(STRLEN(PROF_LINE_FEED));
    buffer.closing_string();
    call_log(buffer);
    buffer.reset_offset();
    */

    buffer.push_string(STRLEN(PROF_LINE_FEED));
    buffer.closing_string();
    call_log(buffer);
    buffer.reset_offset();

    call_cpu(INNER_PROF_SERIALIZE_COST, cost.stop_and_save().cycles());
    return 0;
}


#endif

/*
* zprof License
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



#ifndef ZPROF_H
#define ZPROF_H

#ifndef PROF_DEFAULT_INST_ID 
#define PROF_DEFAULT_INST_ID 0
#endif

#ifndef PROF_RESERVE_COUNT
#define PROF_RESERVE_COUNT 50
#endif 


#ifndef PROF_DECLARE_COUNT
#define PROF_DECLARE_COUNT 260
#endif 




#define ProfInstType ProfRecord<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT>
#define ProfInst ProfInstType::instance()



template<bool IS_BAT, ProfLevel PROF_LEVEL>
inline void ProfRecordWrap(int idx, long long count, long long cost)
{

}


template<>
inline void ProfRecordWrap<true, PROF_LEVEL_NORMAL>(int idx, long long count, long long cost)
{
    ProfInst.call_cpu(idx, count, cost);
}

template<>
inline void ProfRecordWrap<false, PROF_LEVEL_NORMAL>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.call_cpu(idx, cost);
}
template<>
inline void ProfRecordWrap<true, PROF_LEVEL_FAST>(int idx, long long count, long long cost)
{
    ProfInst.call_cpu_no_sm(idx, count, cost);
}
template<>
inline void ProfRecordWrap<false, PROF_LEVEL_FAST>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.call_cpu_no_sm(idx, cost);
}

template<>
inline void ProfRecordWrap<true, PROF_LEVEL_FULL>(int idx, long long count, long long cost)
{
    ProfInst.call_cpu_full(idx, count, cost);
}
template<>
inline void ProfRecordWrap<false, PROF_LEVEL_FULL>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.call_cpu_full(idx, cost);
}

template<long long COUNT>
struct ProfCountIsGreatOne
{
    static const bool is_bat = COUNT > 1;
};


template <long long COUNT = 1, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoRecord
{
public:
    ProfAutoRecord(int idx)
    {
        idx_ = idx;
        counter_.start();
    }
    ~ProfAutoRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>(idx_, COUNT, counter_.save().cycles());
    }
    ProfCounter<C>& counter() { return counter_; }
private:
    ProfCounter<C> counter_;
    int idx_;
};




template <long long COUNT = 1LL, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoAnonRecord
{
public:
    static const size_t DESC_SIZE = 100;
public:
    ProfAutoAnonRecord(const char* desc)
    {
        strncpy(desc_, desc, DESC_SIZE);
        desc_[DESC_SIZE - 1] = '\0';
        counter_.start();
    }
    ~ProfAutoAnonRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>(ProfInstType::INNER_PROF_NULL, COUNT, counter_.save().cycles());
        ProfSerializeBuffer buffer(ProfInst.serialize_buffer(), ProfInstType::max_serialize_buff_size()); 
        ProfInst.serialize_root(ProfInstType::INNER_PROF_NULL, 0, desc_, strlen(desc_), buffer, NULL);
        ProfInst.reset_node(ProfInstType::INNER_PROF_NULL);
    }

    ProfCounter<C>& counter() { return counter_; }
private:
    ProfCounter<C> counter_;
    char desc_[DESC_SIZE];
};




#ifdef OPEN_ZPROF

#define PROF_REGIST_NODE(id, name, ct, resident, re_reg)  ProfInst.regist_node(id, name, ct, resident, re_reg)
#define PROF_FAST_REGIST_NODE(id)  ProfInst.regist_node(id, #id, PROF_COUNTER_DEFAULT,  false, false)
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.regist_node(id, name, PROF_COUNTER_DEFAULT,  false, false)
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.regist_node(id, #id, PROF_COUNTER_DEFAULT,  true, false)
#define PROF_BIND_CHILD(id, cid)  ProfInst.bind_childs(id, cid)
#define PROF_BIND_MERGE(id, cid) ProfInst.bind_merge(cid, id)
#define PROF_BIND_CHILD_AND_MERGE(id, cid) do {PROF_BIND_CHILD(id, cid); PROF_BIND_MERGE(id, cid); }while(0)
#define PROF_REG_AND_BIND_CHILD(id, cid)  do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_CHILD(id, cid); } while(0)
#define PROF_REG_AND_BIND_MERGE(id, cid) do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_MERGE(id, cid); } while(0)
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) do {PROF_FAST_REGIST_NODE(cid);  PROF_BIND_CHILD_AND_MERGE(id, cid); }while(0)






#define PROF_INIT(desc) ProfInst.init_prof(desc)
#define PROF_INIT_JUMP_COUNT() ProfInst.init_jump_count()
#define PROF_RESET_CHILD(idx) ProfInst.reset_childs(idx)
#define PROF_UPDATE_MERGE() ProfInst.update_merge()
#define PROF_CLEAN_RESERVE() ProfInst.clean_reserve_info()
#define PROF_CLEAN_DECLARE() ProfInst.clean_declare_info()

#define PROF_CALL_CPU_SAMPLE(idx, cost) ProfInst.call_cpu(idx, cost)
#define PROF_CALL_CPU_WRAP(idx, COUNT, cost, PROF_LEVEL)  \
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>((int)(idx), (long long)(COUNT), (long long)cost)
#define PROF_CALL_CPU_DYN_WRAP(idx, count, cost, PROF_LEVEL)  \
        ProfRecordWrap<true, PROF_LEVEL>((int)(idx), (long long)(count), (long long)cost)
#define PROF_CALL_CPU(idx, cost) PROF_CALL_CPU_WRAP((idx), 1, (cost), PROF_LEVEL_NORMAL)
#define PROF_CALL_MEM(idx, count, mem) ProfInst.call_mem(idx, count, mem)
#define PROF_CALL_VM(idx, vm) ProfInst.call_vm(idx, vm)
#define PROF_REFRESH_MEM(idx, count, mem) ProfInst.refresh_mem(idx, count, mem)
#define PROF_CALL_TIMER(idx, stamp) ProfInst.call_timer(idx, stamp)
#define PROF_CALL_USER(idx, count, add) ProfInst.call_user(idx, count, add)


#define PROF_DEFINE_COUNTER(var)  ProfCounter<> var
#define PROF_DEFINE_COUNTER_INIT(tc, start)  ProfCounter<> tc(start)
#define PROF_START_COUNTER(var) var.start()
#define PROF_RESTART_COUNTER(var) var.start()
#define PROF_STOP_AND_SAVE_COUNTER(var) var.stop_and_save()
#define PROF_STOP_AND_RECORD(idx, var) PROF_CALL_CPU_WRAP((idx), 1, (var).stop_and_save().cycles(), PROF_LEVEL_NORMAL)

#define PROF_DEFINE_AUTO_RECORD(var, idx) ProfAutoRecord<> var(idx)
#define PROF_DEFINE_AUTO_ANON_RECORD(var, desc) ProfAutoAnonRecord<> var(desc)
#define PROF_DEFINE_AUTO_MULTI_ANON_RECORD(var, count, desc) ProfAutoAnonRecord<count> var(desc)
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, count, level, ct, desc) ProfAutoAnonRecord<count, level, ct> var(desc)


#define PROF_OUTPUT_DEFAULT_LOG(desc)        ProfSerializeBuffer buffer(ProfInst.serialize_buffer(), ProfInstType::max_serialize_buff_size()); \
                                                              ProfInst.serialize_root(ProfInstType::INNER_PROF_NULL, 0, desc, strlen(desc), buffer, NULL);\
                                                              ProfInst.reset_node(ProfInstType::INNER_PROF_NULL);

#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  do {ProfRecordWrap<true, PROF_LEVEL_FAST>((int)ProfInstType::INNER_PROF_NULL, (long long)(count), (long long)num);  PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) do {PROF_CALL_USER(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) do {PROF_CALL_MEM(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   do {PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_USER(desc, num) do {PROF_CALL_USER(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) do {PROF_CALL_MEM(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SELF_MEM(desc) do{PROF_CALL_VM(ProfInstType::INNER_PROF_NULL, prof_get_mem_use()); PROF_OUTPUT_DEFAULT_LOG(desc);}while(0)



#else
#define PROF_REGIST_NODE(id, name, pt, resident, force)
#define PROF_FAST_REGIST_NODE(id) 
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  
#define PROF_BIND_CHILD(id, cid) 
#define PROF_BIND_MERGE(id, cid) 
#define PROF_BIND_CHILD_AND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD(id, cid)  
#define PROF_REG_AND_BIND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) 

#define PROF_INIT(desc) 
#define PROF_INIT_JUMP_COUNT()
#define PROF_RESET_CHILD(idx) 
#define PROF_UPDATE_MERGE() 
#define PROF_RESET_RESERVE()
#define PROF_RESET_DECLARE() 
#define PROF_RESET_ANON() 

#define PROF_CALL_CPU_SAMPLE(idx, cost) 
#define PROF_CALL_CPU(idx, cost) 
#define PROF_CALL_CPU_WRAP(idx, COUNT, cost, PROF_LEVEL) 
#define PROF_CALL_CPU_DYN_WRAP(idx, count, cost, PROF_LEVEL)
#define PROF_CALL_MEM(idx, count, mem) 
#define PROF_CALL_VM(idx, vm) 
#define PROF_REFRESH_MEM(idx, count, mem) 
#define PROF_CALL_TIMER(idx, stamp) 
#define PROF_CALL_USER(idx, count, add)

#define PROF_DEFINE_COUNTER(var)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  
#define PROF_START_COUNTER(var) 
#define PROF_RESTART_COUNTER(var) 
#define PROF_STOP_AND_SAVE_COUNTER(var) 
#define PROF_STOP_AND_RECORD(idx, var) 

#define PROF_DEFINE_AUTO_RECORD(var, idx) 
#define PROF_DEFINE_AUTO_ANON_RECORD(desc, idx) 
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, count, level, ct, desc) 

#define PROF_OUTPUT_DEFAULT_LOG(desc) 

#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) 
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) 
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   
#define PROF_OUTPUT_SINGLE_USER(desc, num)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) 
#define PROF_OUTPUT_SELF_MEM(desc) 

#endif




#define PROF_SERIALIZE_FN_LOG()    ProfInst.serialize(0xff, NULL)






#endif
#ifdef __GNUG__
#pragma GCC pop_options
#endif
