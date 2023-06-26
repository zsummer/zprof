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
    PROF_CONNTER_CHRONO_STEADY,
    PROF_CONNTER_CHRONO_SYS,

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
PROF_ALWAYS_INLINE long long prof_get_time_cycle()
{
    return 0;
}

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_PURE>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_LOCK>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE_BTB>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_RDTSCP>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_CLOCK>()
{
#if (defined WIN32)
    LARGE_INTEGER win_freq;
    win_freq.QuadPart = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&win_freq);
    return win_freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
#endif
}

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_COUNTER_SYS>()
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
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_CONNTER_CHRONO>()
{
    return std::chrono::high_resolution_clock().now().time_since_epoch().count();
}

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_CONNTER_CHRONO_STEADY>()
{
    return std::chrono::steady_clock().now().time_since_epoch().count();
}

template<>
PROF_ALWAYS_INLINE long long prof_get_time_cycle<PROF_CONNTER_CHRONO_SYS>()
{
    return std::chrono::system_clock().now().time_since_epoch().count();
}


inline ProfVM prof_get_mem_use()
{
    ProfVM vm = { 0ULL, 0ULL, 0ULL };
#ifdef WIN32
    HANDLE hproc = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hproc, &pmc, sizeof(pmc)))
    {
        CloseHandle(hproc);// ignore  
        vm.vm_size = (unsigned long long)pmc.WorkingSetSize;
        vm.rss_size = (unsigned long long)pmc.WorkingSetSize;
    }
#else
    const char* file = "/proc/self/statm";
    FILE* fp = fopen(file, "r");
    if (fp != NULL)
    {
        char line_buff[256];
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
    }
#endif
    return vm;
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
inline double prof_get_cpu_mhz()
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
PROF_ALWAYS_INLINE double prof_get_time_frequency()
{
    return 1.0;
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC>()
{
    static double frequency_per_ns = prof_get_cpu_mhz() * 1000.0 * 1000.0 / 1000.0 / 1000.0 / 1000.0;
    return frequency_per_ns;
}
template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_BTB>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_NOFENCE>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_PURE>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_LOCK>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_MFENCE>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSC_MFENCE_BTB>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_RDTSCP>()
{
    return prof_get_time_frequency<PROF_COUNTER_RDTSC>();
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_CLOCK>()
{
#ifdef WIN32
    double frequency_per_ns = 0;
    LARGE_INTEGER win_freq;
    win_freq.QuadPart = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
    frequency_per_ns = win_freq.QuadPart / 1000.0 / 1000.0 / 1000.0;
    return frequency_per_ns;
#else
    return 1.0;
#endif
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_COUNTER_SYS>()
{
    return 1.0;
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_CONNTER_CHRONO>()
{
    static double chrono_frequency = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
    return chrono_frequency;
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_CONNTER_CHRONO_STEADY>()
{
    static double chrono_frequency = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
    return chrono_frequency;
}

template<>
PROF_ALWAYS_INLINE double prof_get_time_frequency<PROF_CONNTER_CHRONO_SYS>()
{
    static double chrono_frequency = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
    return chrono_frequency;
}

template<ProfCounterType T>
PROF_ALWAYS_INLINE double prof_get_time_inverse_frequency()
{
    static double inverse_frequency_per_ns = 1.0 / (prof_get_time_frequency<T>() <= 0.0 ? 1.0 : prof_get_time_frequency<T>());
    return inverse_frequency_per_ns;
}











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

    void set_start_val(long long val) { start_val_ = val; }
    void set_cycles_val(long long cycles) { cycles_ = cycles; }
private:
    long long start_val_;
    long long cycles_;
};















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


#define PROF_NAME_MAX_SIZE 50  
#define PROF_DESC_MAX_SIZE 100
#define PROF_LINE_MIN_SIZE 200
#define PROF_MAX_DEPTH 5

class ProfSerializer
{
public:
    ProfSerializer() = delete;
    explicit ProfSerializer(char* buff, size_t buff_size)
    {
        buff_ = buff;
        buff_len_ = buff_size;
        offset_ = 0;
    }


    inline ProfSerializer& push_human_count(long long count);
    inline ProfSerializer& push_human_time(long long ns);
    inline ProfSerializer& push_human_mem(long long bytes);
    inline ProfSerializer& push_char(char ch, int repeat = 1);
    inline ProfSerializer& push_string(const char* str);
    inline ProfSerializer& push_string(const char* str, size_t size);
    inline ProfSerializer& push_now_date();
    inline ProfSerializer& push_number(unsigned long long number, int wide = 0);
    inline ProfSerializer& push_number(long long number, int wide = 0);

    inline ProfSerializer& push_indent(int count);
    inline ProfSerializer& push_blank(int count);


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




inline ProfSerializer& ProfSerializer::push_human_count(long long count)
{
    if (buff_len_ <= offset_ + 35)
    {
        return *this;
    }
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

inline ProfSerializer& ProfSerializer::push_human_time(long long ns)
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


inline ProfSerializer& ProfSerializer::push_human_mem(long long bytes)
{
    if (buff_len_ <= offset_ + 35)
    {
        return *this;
    }
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

inline ProfSerializer& ProfSerializer::push_char(char ch, int repeat)
{
    while (repeat > 0 && offset_ < buff_len_)
    {
        buff_[offset_++] = ch;
        repeat--;
    }
    return *this;
}

inline ProfSerializer& ProfSerializer::push_number(unsigned long long number, int wide)
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

inline ProfSerializer& ProfSerializer::push_number(long long number, int wide)
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

inline ProfSerializer& ProfSerializer::push_string(const char* str)
{
    return push_string(str, strlen(str));
}
inline ProfSerializer& ProfSerializer::push_string(const char* str, size_t size)
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
inline ProfSerializer& ProfSerializer::push_now_date()
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


inline void ProfSerializer::closing_string()
{
    size_t closed_id = offset_ >= buff_len_ ? buff_len_ - 1 : offset_;
    buff_[closed_id] = '\0';
}

inline ProfSerializer& ProfSerializer::push_indent(int count)
{
    static const char* const pi = "                                                            ";
    constexpr int pi_size = 50;
    static_assert(pi_size >= PROF_NAME_MAX_SIZE, "");
    static_assert(pi_size >= PROF_MAX_DEPTH*2, "indent is two blank");
    if (count > pi_size)
    {
        count = pi_size;
    }
    if (count <= 0)
    {
        return *this;
    }
    if (offset_ + count >= buff_len_)
    {
        return *this;
    }
    memcpy(buff_ + offset_, pi, count);
    offset_ += count;
    return *this;
}



inline ProfSerializer& ProfSerializer::push_blank(int count)
{
    static const char* const pi = "------------------------------------------------------------";
    constexpr int pi_size = 50;
    static_assert(pi_size >= PROF_NAME_MAX_SIZE, "");
    if (count > pi_size)
    {
        count = pi_size;
    }
    if (count <= 0)
    {
        return *this;
    }
    if (offset_ + count >= buff_len_)
    {
        return *this;
    }
    memcpy(buff_ + offset_, pi, count);
    offset_ += count;
    return *this;
}

class ProfStackSerializer : public ProfSerializer
{
public:
    static const int BUFF_SIZE = 350;
    static_assert(BUFF_SIZE > PROF_LINE_MIN_SIZE, "");
    ProfStackSerializer() :ProfSerializer(buff_, BUFF_SIZE)
    {
        buff_[0] = '\0';
    }
    ~ProfStackSerializer()
    {

    }

private:
    char buff_[BUFF_SIZE];//1k  
};



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






#define SMOOTH_CYCLES(s_cost, cost) (   (s_cost * 12 + cost * 4) >> 4   ) 
#define SMOOTH_CYCLES_WITH_INIT(s_cost, cost) ( (s_cost) == 0 ? (cost) : SMOOTH_CYCLES(s_cost, cost) )

enum ProfLevel
{
    PROF_LEVEL_NORMAL,
    PROF_LEVEL_FAST,
    PROF_LEVEL_FULL,
};


struct ProfTraits
{
    int name;
    int name_len;
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



struct ProfMerge
{
    int to;
    int childs;
    int merged;
};

struct ProfShow
{
    int upper;
    int jumps;
    int child;
    int window;
};

struct ProfNode
{
    bool active;  
    ProfTraits traits;
    ProfShow show;
    ProfMerge merge;
    ProfCPU cpu; 
    ProfMEM mem; 
    ProfTimer timer;
    ProfUser user;
    ProfVM vm;
};  

enum ProfOutputFlags : unsigned int
{
    PROF_OUTPUT_FLAG_NULL,
    PROF_OUTPUT_FLAG_INNER = 0x1,
    PROF_OUTPUT_FLAG_RESERVE = 0x2,
    PROF_OUTPUT_FLAG_DELCARE = 0x4,
    PROF_OUTPUT_FLAG_ALL = 0xffff,
};

/*
#ifdef _FN_LOG_LOG_H_
static inline void ProfDefaultFNLogFunc(const ProfSerializer& serializer)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(serializer.buff(), (int)serializer.offset());
}
#endif
*/

template<int INST, int RESERVE, int DECLARE>
class ProfRecord 
{
public:
    using Output = void(*)(const ProfSerializer& serializer);
    enum InnerType
    {
        INNER_PROF_NULL,
        INNER_PROF_INIT_COST,
        INNER_PROF_MERGE_COST,
        INNER_PROF_REPORT_COST,
        INNER_PROF_SERIALIZE_COST,
        INNER_PROF_OUTPUT_COST,
        INNER_PROF_MEM_INFO_COST,
        INNER_PROF_COUNTER_COST,
        INNER_PROF_RECORD_COST,
        INNER_PROF_RECORD_SM_COST,
        INNER_PROF_RECORD_FULL_COST,
        INNER_PROF_COUNTER_RECORD_COST,
        INNER_PROF_ORIGIN_INC,
        INNER_PROF_ATOM_RELEAX,
        INNER_PROF_ATOM_COST,
        INNER_PROF_ATOM_SEQ_COST,
        INNER_PROF_MAX,
    };

    static constexpr int inst_id() { return INST; }



    static constexpr int reserve_begin_id() { return INNER_PROF_MAX; }
    static constexpr int reserve_count() { return RESERVE; }
    static constexpr int reserve_end_id() { return reserve_begin_id() + reserve_count(); }

    static constexpr int declare_begin_id() { return reserve_end_id(); }
    static constexpr int declare_count() { return DECLARE; }
    static constexpr int declare_end_id() { return declare_begin_id() + declare_count(); }
    inline int declare_window() { return declare_window_; }

    static constexpr int begin_id() { return INNER_PROF_NULL + 1; }
    static constexpr int count() { return declare_end_id() - 1; }
    static constexpr int end_id() { return begin_id() + count(); }
    static constexpr int max_count() { return count(); }

    static constexpr int compact_data_size() { return 30 * (1+end_id()); } //reserve node no name 
    static_assert(end_id() == INNER_PROF_MAX + reserve_count() + declare_count(), "");

public:
    long long init_timestamp_;
    long long last_timestamp_;
public:
    static inline ProfRecord& instance()
    {
        static ProfRecord inst;
        return inst;
    }

public:
    ProfRecord();
    int init(const char* title);
    int regist(int idx, const char* name, unsigned int counter, bool resident, bool re_reg);
    const char* title() const { return &compact_data_[title_]; }

    const char* name(int idx);
    int rename(int idx, const char* name);


    int bind_childs(int idx, int child);
    int build_jump_path();

    int bind_merge(int to, int child);
    void do_merge();

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

    void reset_range_node(int first_idx, int end_idx, bool keep_resident = true)
    {
        for (int idx = first_idx; idx < end_idx; idx++)
        {
            if (!keep_resident || !nodes_[idx].traits.resident)
            {
                reset_node(idx);
            }
        }
    }

    void reset_reserve_node(bool keep_resident = true)
    {
        reset_range_node(reserve_begin_id(), reserve_end_id(), keep_resident);
        last_timestamp_ = time(NULL);
    }

    void reset_declare_node(bool keep_resident = true)
    {
        reset_range_node(declare_begin_id(), declare_end_id(), keep_resident);
        last_timestamp_ = time(NULL);
    }



    inline void reset_childs(int idx, int depth = 0);

    PROF_ALWAYS_INLINE void record_cpu(int idx, long long c, long long cost)
    {
        long long dis = cost / c;
        ProfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
        node.cpu.dv += abs(dis - node.cpu.sum/node.cpu.c);
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void record_cpu(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.max_u = (node.cpu.max_u < cost ? cost : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < cost ? node.cpu.min_u : cost);
        node.cpu.dv += abs(cost - node.cpu.sm);
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void record_cpu_no_sm(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = cost;
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void record_cpu_no_sm(int idx, long long count, long long cost)
    {
        long long dis = cost / count;
        ProfNode& node = nodes_[idx];
        node.cpu.c += count;
        node.cpu.sum += cost;
        node.cpu.sm = dis;
        node.cpu.t_u += cost;
    }

    PROF_ALWAYS_INLINE void record_cpu_full(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        long long dis = cost;
        long long avg = node.cpu.sum / node.cpu.c;

        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.h_sm = (dis >= avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
        node.cpu.l_sm = (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_u += cost;
        node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
    }

    PROF_ALWAYS_INLINE void record_cpu_full(int idx, long long c, long long cost)
    {
        
        ProfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        long long dis = cost / c;
        long long avg = node.cpu.sum / node.cpu.c;

        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.h_sm =  (dis > avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
        node.cpu.l_sm =  (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_u += cost;
        node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
    }


    PROF_ALWAYS_INLINE void record_timer(int idx, long long stamp)
    {
        ProfNode& node = nodes_[idx];
        if (node.timer.last == 0)
        {
            node.timer.last = stamp;
            return;
        }
        record_cpu_full(idx, 1, stamp - node.timer.last);
        node.timer.last = stamp;
    }

    PROF_ALWAYS_INLINE void record_mem(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.mem.c += c;
        node.mem.sum += add;
        node.mem.t_u += add;
    }
    PROF_ALWAYS_INLINE void record_vm(int idx, const ProfVM& vm)
    {
        nodes_[idx].vm = vm;
    }
    PROF_ALWAYS_INLINE void record_user(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.user.c += c;
        node.user.sum += add;
        node.user.t_u += add;
    }

    PROF_ALWAYS_INLINE void overwrite_mem(int idx, long long c, long long add)
    {
        reset_mem(idx);
        record_mem(idx, c, add);
    }


    

    //递归展开  
    int recursive_output(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, ProfSerializer& serializer);
    

    //完整报告  
    int output_report(unsigned int flags = PROF_OUTPUT_FLAG_ALL);
    int output_one_record(int entry_idx);
    int output_temp_record(const char* opt_name, size_t opt_name_len);
    int output_temp_record(const char* opt_name);


public:
    ProfSerializer& compact_writer() { return compact_writer_; }
    ProfNode& node(int idx) { return nodes_[idx]; }
    
    double particle_for_ns(int t) { return  particle_for_ns_[t == PROF_COUNTER_NULL ? PROF_COUNTER_DEFAULT : t]; }




 //output interface
public:
    void set_output(Output func) { output_ = func; }
protected:
    void output_and_clean(ProfSerializer& s) { s.closing_string(); output_(s); s.reset_offset(); }
    static void default_output(const ProfSerializer& serializer) { printf("%s\n", serializer.buff()); }
private:
    Output output_;

//merge data and interface 
public:
    std::array<int, end_id()>& merge_leafs() { return merge_leafs_; }
    int merge_leafs_size() { return merge_leafs_size_; }
private:
    std::array<int, end_id()> merge_leafs_;
    int merge_leafs_size_;



private:
    int title_;
    char compact_data_[compact_data_size()];
    ProfSerializer compact_writer_;
    int unknown_desc_;
    int reserve_desc_;
    int no_name_space_;
    int no_name_space_len_;

private:
    ProfNode nodes_[end_id()];
    int declare_window_;
    double particle_for_ns_[PROF_COUNTER_MAX];
};

template<int INST, int RESERVE, int DECLARE>
ProfRecord<INST, RESERVE, DECLARE>::ProfRecord() : compact_writer_(compact_data_, compact_data_size())
{
    memset(nodes_, 0, sizeof(nodes_));
    merge_leafs_size_ = 0;
    memset(particle_for_ns_, 0, sizeof(particle_for_ns_));
    declare_window_ = declare_begin_id();

    output_ = &ProfRecord::default_output;  //set default log;

    init_timestamp_ = 0;
    last_timestamp_ = 0;
    static_assert(compact_data_size() > 150, "");
    unknown_desc_ = 0;
    compact_writer_.push_string("unknown");
    compact_writer_.push_char('\0');
    reserve_desc_ = (int)compact_writer_.offset();
    compact_writer_.push_string("reserve");
    compact_writer_.push_char('\0');
    no_name_space_ = (int)compact_writer_.offset();
    compact_writer_.push_string("null(name empty or over buffers)");
    no_name_space_len_ = (int)(compact_writer_.offset() - no_name_space_);
    compact_writer_.push_char('\0');
    title_ = 0;

};



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::init(const char* title)
{
    if (title == NULL || compact_writer_.is_full())
    {
        title_ = 0;
    }
    else
    {
        title_ = (int)compact_writer_.offset();
        compact_writer_.push_string("title");
        compact_writer_.push_char('\0');
        compact_writer_.closing_string();
    }
    ProfCounter<> counter;
    counter.start();

    last_timestamp_ = time(NULL);
    init_timestamp_ = time(NULL);

    particle_for_ns_[PROF_COUNTER_NULL] = 0;
    particle_for_ns_[PROF_COUNTER_SYS] = prof_get_time_inverse_frequency<PROF_COUNTER_SYS>();
    particle_for_ns_[PROF_COUNTER_CLOCK] = prof_get_time_inverse_frequency<PROF_COUNTER_CLOCK>();
    particle_for_ns_[PROF_CONNTER_CHRONO] = prof_get_time_inverse_frequency<PROF_CONNTER_CHRONO>();
    particle_for_ns_[PROF_CONNTER_CHRONO_STEADY] = prof_get_time_inverse_frequency<PROF_CONNTER_CHRONO_STEADY>();
    particle_for_ns_[PROF_CONNTER_CHRONO_SYS] = prof_get_time_inverse_frequency<PROF_CONNTER_CHRONO_SYS>();
    particle_for_ns_[PROF_COUNTER_RDTSC] = prof_get_time_inverse_frequency<PROF_COUNTER_RDTSC>();
    particle_for_ns_[PROF_COUNTER_RDTSC_BTB] = particle_for_ns_[PROF_COUNTER_RDTSC];
    particle_for_ns_[PROF_COUNTER_RDTSCP] = particle_for_ns_[PROF_COUNTER_RDTSC];
    particle_for_ns_[PROF_COUNTER_RDTSC_MFENCE] = particle_for_ns_[PROF_COUNTER_RDTSC];
    particle_for_ns_[PROF_COUNTER_RDTSC_MFENCE_BTB] = particle_for_ns_[PROF_COUNTER_RDTSC];
    particle_for_ns_[PROF_COUNTER_RDTSC_NOFENCE] = particle_for_ns_[PROF_COUNTER_RDTSC];
    particle_for_ns_[PROF_COUNTER_RDTSC_PURE] = particle_for_ns_[PROF_COUNTER_RDTSC];
    particle_for_ns_[PROF_COUNTER_RDTSC_LOCK] = particle_for_ns_[PROF_COUNTER_RDTSC];
    particle_for_ns_[PROF_COUNTER_NULL] = particle_for_ns_[PROF_COUNTER_DEFAULT];

    for (int i = begin_id(); i < reserve_end_id(); i++)
    {
        regist(i, "reserve", PROF_COUNTER_DEFAULT, false, false);
    }

    regist(INNER_PROF_NULL, "INNER_PROF_NULL", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_INIT_COST, "INIT_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_MERGE_COST, "MERGE_COST", PROF_COUNTER_DEFAULT, true, true);

    regist(INNER_PROF_REPORT_COST, "REPORT_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_SERIALIZE_COST, "SERIALIZE_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_OUTPUT_COST, "OUTPUT_COST", PROF_COUNTER_DEFAULT, true, true);
    
    regist(INNER_PROF_MEM_INFO_COST, "MEM_INFO_COST", PROF_COUNTER_DEFAULT, true, true);

    regist(INNER_PROF_COUNTER_COST, "COUNTER_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_RECORD_COST, "RECORD_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_RECORD_SM_COST, "RECORD_SM_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_RECORD_FULL_COST, "RECORD_FULL_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_COUNTER_RECORD_COST, "COUNTER_RECORD_COST", PROF_COUNTER_DEFAULT, true, true);

    regist(INNER_PROF_ORIGIN_INC, "ORIGIN_INC", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_ATOM_RELEAX, "ATOM_RELEAX", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_ATOM_COST, "ATOM_COST", PROF_COUNTER_DEFAULT, true, true);
    regist(INNER_PROF_ATOM_SEQ_COST, "ATOM_SEQ_COST", PROF_COUNTER_DEFAULT, true, true);




    if (true)
    {
        ProfCounter<> self_mem_cost;
        self_mem_cost.start();
        record_vm(INNER_PROF_MEM_INFO_COST, prof_get_mem_use());
        record_cpu(INNER_PROF_MEM_INFO_COST, self_mem_cost.stop_and_save().cycles());
        record_mem(INNER_PROF_MEM_INFO_COST, 1, sizeof(*this));
        record_user(INNER_PROF_MEM_INFO_COST, 1, max_count());
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
            record_cpu(INNER_PROF_NULL, test_cost.cycles());
        }
        record_cpu(INNER_PROF_COUNTER_RECORD_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            cost.save();
        }
        record_cpu(INNER_PROF_COUNTER_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            record_cpu_no_sm(INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        record_cpu(INNER_PROF_RECORD_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            record_cpu(INNER_PROF_NULL, 1, cost.stop_and_save().cycles());
        }
        record_cpu(INNER_PROF_RECORD_SM_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            record_cpu_full(INNER_PROF_NULL, 1, cost.stop_and_save().cycles());
        }
        record_cpu(INNER_PROF_RECORD_FULL_COST, 1000, cost.stop_and_save().cycles());


        std::atomic<long long> atomll_test(0);
        volatile long long origin_feetch_add_test = 0;
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            origin_feetch_add_test++;
        }
        record_cpu(INNER_PROF_ORIGIN_INC, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_relaxed);
        }
        record_cpu(INNER_PROF_ATOM_RELEAX, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test++;
        }
        record_cpu(INNER_PROF_ATOM_COST, 1000, cost.stop_and_save().cycles());

        
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_seq_cst);
        }
        record_cpu(INNER_PROF_ATOM_SEQ_COST, 1000, cost.stop_and_save().cycles());

        reset_node(INNER_PROF_NULL);
    }

    record_cpu(INNER_PROF_INIT_COST, counter.stop_and_save().cycles());

    return 0;
}


template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::build_jump_path()
{
    for (int i = declare_begin_id(); i < declare_end_id(); )
    {
        int next_upper_id = i + 1;
        while (next_upper_id < declare_end_id())
        {
            if (nodes_[next_upper_id].show.upper == 0)
            {
                break;
            }
            next_upper_id++;
        }
        for (int j = i; j < next_upper_id; j++)
        {
            nodes_[j].show.jumps = next_upper_id - j - 1;
        }
        i = next_upper_id;
    }
    return 0;
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::regist(int idx, const char* name, unsigned int counter_type, bool resident, bool re_reg)
{
    if (idx >= end_id() )
    {
        return -1;
    }
    if (name == NULL)
    {
        return -3;
    }
    
    
    ProfNode& node = nodes_[idx];


    if (!re_reg && node.active)
    {
        return 0;
    }

    memset(&node, 0, sizeof(node));
    rename(idx, name);
    nodes_[idx].traits.counter_type = counter_type;
    nodes_[idx].traits.resident = resident;
    node.active = true;
    node.cpu.min_u = LLONG_MAX;

    if (idx >= declare_begin_id() && idx < declare_end_id() && idx + 1 > declare_window_)
    {
        declare_window_ = idx + 1;
    }

    return 0;
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::rename(int idx, const char* name)
{
    if (idx < begin_id() || idx >= end_id() )
    {
        return -1;
    }
    if (name == NULL)
    {
        return -3;
    }
    if (strcmp(name, "reserve") == 0)
    {
        nodes_[idx].traits.name = reserve_desc_;
        nodes_[idx].traits.name_len = 7;
        return 0;
    }


    nodes_[idx].traits.name = (int)compact_writer_.offset();// node name is "" when compact serializer full 
    compact_writer_.push_string(name);
    compact_writer_.push_char('\0');
    compact_writer_.closing_string();
    nodes_[idx].traits.name_len = (int)strlen(&compact_data_[nodes_[idx].traits.name]);
    if (nodes_[idx].traits.name_len == 0)
    {
        nodes_[idx].traits.name = no_name_space_;
        nodes_[idx].traits.name_len = no_name_space_len_;
    }
    return 0;
}


template<int INST, int RESERVE, int DECLARE>
const char* ProfRecord<INST, RESERVE, DECLARE>::name(int idx)
{
    if (idx < begin_id() || idx >= end_id())
    {
        return "";
    }
    ProfTraits& traits = nodes_[idx].traits;
    if (traits.name >= compact_data_size())
    {
        return "";
    }
    return &compact_data_[traits.name];
};


template<int INST, int RESERVE, int DECLARE>
void ProfRecord<INST, RESERVE, DECLARE>::reset_childs(int idx, int depth)
{
    if (idx < begin_id() || idx >= end_id())
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
    for (int i = node.show.child; i < node.show.child + node.show.window; i++)
    {
        ProfNode& child = nodes_[i];
        if (child.show.upper == idx)
        {
           reset_childs(i, depth + 1);
        }
    }
}


template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::bind_childs(int idx, int cidx)
{
    if (idx < begin_id() || idx >= end_id() || cidx < begin_id() || cidx >= end_id())
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
    if (node.show.child == 0)
    {
        node.show.child = cidx;
        node.show.window = 1;
    }
    else
    {
        if (cidx < node.show.child)
        {
            node.show.window += node.show.child - cidx;
            node.show.child = cidx;
        }
        else if (cidx >= node.show.child + node.show.window)
        {
            node.show.window = cidx - node.show.child + 1;
        }
    }

    child.show.upper = idx;
    return 0;
}



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::bind_merge(int to, int child)
{
    if (child < begin_id() || child >= end_id() || to < begin_id() || to >= end_id())
    {
        return -1;
    }

    if (child == to)
    {
        return -2;
    }
    if (merge_leafs_size_ >= end_id())
    {
        return -3;
    }
    ProfNode& node = nodes_[child];
    ProfNode& to_node = nodes_[to];
    if (!node.active || !to_node.active)
    {
        return -3; //regist method has memset all info ; 
    }

    //change merge to;  
    if (node.merge.to != 0)
    {
        return -4;
    }

    to_node.merge.childs++;
    node.merge.to = to;

    if (node.merge.childs > 0)
    {
        return 0;
    }

    if (to_node.merge.to != 0)
    {
        for (int i = 0; i < merge_leafs_size_; i++)
        {
            if (merge_leafs_[i] == to)
            {
                merge_leafs_[i] = child;
                return 0;
            }
        }
    }



    merge_leafs_[merge_leafs_size_++] = child;
    return 0;
}


template<int INST, int RESERVE, int DECLARE>
void ProfRecord<INST, RESERVE, DECLARE>::do_merge()
{
    ProfCounter<PROF_COUNTER_DEFAULT> cost;
    cost.start();
    for (int i = 0; i < merge_leafs_size_; i++)
    {
        int leaf_id = merge_leafs_[i];
        ProfNode& leaf = nodes_[leaf_id];
        ProfNode* node = NULL;
        long long append_cpu = 0;
        long long append_mem = 0;
        long long append_user = 0;
        int id = 0;
        node = &nodes_[leaf.merge.to];
        append_cpu = leaf.cpu.t_u;
        append_mem = leaf.mem.t_u;
        append_user = leaf.user.t_u;
        id = leaf.merge.to;
        leaf.cpu.t_u = 0;
        leaf.mem.t_u = 0;
        leaf.user.t_u = 0;
        do
        {
            node->cpu.t_u += append_cpu;
            node->mem.t_u += append_mem;
            node->user.t_u += append_user;
            node->merge.merged++;
            if (node->merge.merged >= node->merge.childs)
            {
                node->merge.merged = 0;
                append_cpu = node->cpu.t_u;
                append_mem = node->mem.t_u;
                append_user = node->user.t_u;
                if (append_cpu > 0)
                {
                    record_cpu_full(id, append_cpu);
                }
                if (append_mem > 0)
                {
                    record_mem(id, 1, append_mem);
                }
                if (append_user > 0)
                {
                    record_user(id, 1, append_user);
                }
                node->cpu.t_u = 0;
                node->mem.t_u = 0;
                node->user.t_u = 0;
                if (node->merge.to == 0)
                {
                    break;
                }
                id = node->merge.to;
                node = &nodes_[node->merge.to];
                continue;
            }
            break;
        } while (true);
    }
    record_cpu(INNER_PROF_MERGE_COST, cost.stop_and_save().cycles());
}




template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::recursive_output(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, ProfSerializer& serializer)
{
    if (entry_idx >= end_id())
    {
        return -1;
    }

    if (serializer.buff_len() <= PROF_LINE_MIN_SIZE)
    {
        return -2;
    }

    if (output_ == nullptr)
    {
        return -3;
    }

    ProfNode& node = nodes_[entry_idx];

    if (depth == 0 && node.show.upper)
    {
        return 0;
    }
    if (!node.active)
    {
        return 0;
    }
    if (node.traits.name + node.traits.name_len >= compact_data_size())
    {
        return 0;
    }
    if (node.traits.counter_type >= PROF_COUNTER_MAX)
    {
        return 0;
    }


    
    ProfCounter<> cost_single_serialize;

    const char* name = &compact_data_[node.traits.name];
    size_t name_len = node.traits.name_len;
    double cpu_rate = particle_for_ns(node.traits.counter_type);
    if (opt_name != NULL)
    {
        name = opt_name;
        name_len = opt_name_len;
    }

    int name_blank = (int)name_len + depth  + depth;
    name_blank = name_blank < 35 ? 35 - name_blank : 0;

    if (name_len + name_blank > PROF_DESC_MAX_SIZE)
    {
        return -5;
    }

    serializer.reset_offset();

#define STRLEN(str) str, strlen(str)
    if (node.cpu.c > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(name, name_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));

        serializer.push_string(STRLEN("\tcpu*|-- "));
        if (true)
        {
            serializer.push_human_count(node.cpu.c);
            serializer.push_string(STRLEN("c, "));
            serializer.push_human_time((long long)(node.cpu.sum * cpu_rate / node.cpu.c));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.sum * cpu_rate));
        }

        
        if (node.cpu.min_u != LLONG_MAX && node.cpu.max_u > 0)
        {
            serializer.push_string(STRLEN(" --|\tmax-min:|-- "));
            serializer.push_human_time((long long)(node.cpu.max_u * cpu_rate));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.min_u * cpu_rate));
        }

        
        if (node.cpu.dv > 0 || node.cpu.sm > 0)
        {
            serializer.push_string(STRLEN(" --|\tdv-sm:|-- "));
            serializer.push_human_time((long long)(node.cpu.dv * cpu_rate / node.cpu.c));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.sm * cpu_rate));
        }

        
        if (node.cpu.h_sm > 0 || node.cpu.l_sm > 0)
        {
            serializer.push_string(STRLEN(" --|\th-l:|-- "));
            serializer.push_human_time((long long)(node.cpu.h_sm * cpu_rate));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.l_sm * cpu_rate));
        }
        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());

        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());

    }

    if (node.mem.c > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(name, name_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));

        serializer.push_string(STRLEN("\tmem*|-- "));
        if (true)
        {
            serializer.push_human_count(node.mem.c);
            serializer.push_string(STRLEN("c, "));
            serializer.push_human_mem(node.mem.sum / node.mem.c);
            serializer.push_string(STRLEN(", "));
            serializer.push_human_mem(node.mem.sum);
        }

        serializer.push_string(STRLEN(" --||-- "));
        if (node.mem.delta > 0)
        {
            serializer.push_human_mem(node.mem.sum - node.mem.delta);
            serializer.push_string(STRLEN(", "));
            serializer.push_human_mem(node.mem.delta);
        }
        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());


        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());
    }

    if (node.vm.rss_size + node.vm.vm_size > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(name, name_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));


        serializer.push_string(STRLEN("\t vm*|-- "));
        if (true)
        {
            serializer.push_human_mem(node.vm.vm_size);
            serializer.push_string(STRLEN("(vm), "));
            serializer.push_human_mem(node.vm.rss_size);
            serializer.push_string(STRLEN("(rss), "));
            serializer.push_human_mem(node.vm.shr_size);
            serializer.push_string(STRLEN("(shr), "));
            serializer.push_human_mem(node.vm.rss_size - node.vm.shr_size);
            serializer.push_string(STRLEN("(uss)"));
        }

        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());

        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());
    }

    if (node.user.c > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(name, name_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));


        serializer.push_string(STRLEN("\tuser*|-- "));
        if (true)
        {
            serializer.push_human_count(node.user.c);
            serializer.push_string(STRLEN("c, "));
            serializer.push_human_count(node.user.sum / node.user.c);
            serializer.push_string(STRLEN(", "));
            serializer.push_human_count(node.user.sum);
        }

        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());

        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        record_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());
    }

    if (depth > PROF_MAX_DEPTH)
    {
        serializer.push_indent(depth * 2);
        output_and_clean(serializer);
        return -4;
    }

    for (int i = node.show.child; i < node.show.child + node.show.window; i++)
    {
        ProfNode& child = nodes_[i];
        if (child.show.upper == entry_idx)
        {
            int ret = recursive_output(i, depth + 1, NULL, 0, serializer);
            if (ret < 0)
            {
                return ret;
            }
        }
    }

    return 0;
}






template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::output_one_record(int entry_idx)
{
    ProfStackSerializer serializer;
    int ret = recursive_output(entry_idx, 0, NULL, 0, serializer);
    (void)ret;
    return ret;
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::output_temp_record(const char* opt_name, size_t opt_name_len)
{
    ProfStackSerializer serializer;
    int ret = recursive_output(0, 0, opt_name, opt_name_len, serializer);
    reset_node(0);//reset  
    return ret;
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::output_temp_record(const char* opt_name)
{
    return output_temp_record(opt_name, strlen(opt_name));
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::output_report(unsigned int flags)
{
    if (output_ == nullptr)
    {
        return -1;
    }
    ProfCounter<> cost;
    cost.start();
    ProfStackSerializer serializer;

    serializer.reset_offset();
    output_and_clean(serializer);


    serializer.push_char('=', 30);
    serializer.push_char('\t');
    serializer.push_string(title());
    serializer.push_string(STRLEN(" begin output: "));
    serializer.push_now_date();
    serializer.push_char('\t');
    serializer.push_char('=', 30);
    output_and_clean(serializer);

    serializer.push_string(STRLEN("| -- index -- | ---    cpu  ------------ | ----------   hits, avg, sum   ---------- | ---- max, min ---- | ------ dv, sm ------ |  --- hsm, lsm --- | "));
    output_and_clean(serializer);
    serializer.push_string(STRLEN("| -- index -- | ---    mem  ---------- | ----------   hits, avg, sum   ---------- | ------ last, delta ------ | "));
    output_and_clean(serializer);
    serializer.push_string(STRLEN("| -- index -- | ---    vm  ------------ | ----------   vm, rss, shr, uss   ------------------ | " ));
    output_and_clean(serializer);

    serializer.push_string(STRLEN("| -- index -- | ---    user  ----------- | -----------  hits, avg, sum   ---------- | "));
    output_and_clean(serializer);

    if (flags & PROF_OUTPUT_FLAG_INNER)
    {
        serializer.push_string(STRLEN(PROF_LINE_FEED));
        for (int i = INNER_PROF_NULL + 1; i < INNER_PROF_MAX; i++)
        {
            int ret = recursive_output(i, 0, NULL, 0, serializer);
            (void)ret;
        }
    }

    if (flags & PROF_OUTPUT_FLAG_RESERVE)
    {
        serializer.push_string(STRLEN(PROF_LINE_FEED));
        for (int i = reserve_begin_id(); i < reserve_end_id(); i++)
        {
            int ret = recursive_output(i, 0, NULL, 0, serializer);
            (void)ret;
        }
    }
    
    if (flags & PROF_OUTPUT_FLAG_DELCARE)
    {
        serializer.push_string(STRLEN(PROF_LINE_FEED));
        for (int i = declare_begin_id(); i < declare_window(); )
        {
            int ret = recursive_output(i, 0, NULL, 0, serializer);
            (void)ret;
            i += nodes_[i].show.jumps + 1;
        }
    }

    serializer.reset_offset();
    serializer.push_char('=', 30);
    serializer.push_char('\t');
    serializer.push_string(" end : ");
    serializer.push_now_date();
    serializer.push_char('\t');
    serializer.push_char('=', 30);
    output_and_clean(serializer);
    output_and_clean(serializer);

    record_cpu(INNER_PROF_REPORT_COST, cost.stop_and_save().cycles());
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

//默认的全局实例ID   
#ifndef PROF_DEFAULT_INST_ID 
#define PROF_DEFAULT_INST_ID 0
#endif

//PROF保留条目 用于记录一些通用的环境性能数据   
#ifndef PROF_RESERVE_COUNT
#define PROF_RESERVE_COUNT 50
#endif 

//PROF主要条目 需要先注册名字和层级关联关系  
#ifndef PROF_DECLARE_COUNT
#define PROF_DECLARE_COUNT 260
#endif 




//默认的全局实例定义 如需扩展可更换INST ID使用额外的全局实例      
#define ProfInstType ProfRecord<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT>
#define ProfInst ProfInstType::instance()


//包装函数 根据模版参数在编译阶段直接使用不同的入口  从而减少常见使用场景下的运行时判断消耗.  
template<bool IS_BAT, ProfLevel PROF_LEVEL>
inline void ProfRecordWrap(int idx, long long count, long long cost)
{

}

template<>
inline void ProfRecordWrap<true, PROF_LEVEL_NORMAL>(int idx, long long count, long long cost)
{
    ProfInst.record_cpu(idx, count, cost);
}

template<>
inline void ProfRecordWrap<false, PROF_LEVEL_NORMAL>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.record_cpu(idx, cost);
}
template<>
inline void ProfRecordWrap<true, PROF_LEVEL_FAST>(int idx, long long count, long long cost)
{
    ProfInst.record_cpu_no_sm(idx, count, cost);
}
template<>
inline void ProfRecordWrap<false, PROF_LEVEL_FAST>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.record_cpu_no_sm(idx, cost);
}

template<>
inline void ProfRecordWrap<true, PROF_LEVEL_FULL>(int idx, long long count, long long cost)
{
    ProfInst.record_cpu_full(idx, count, cost);
}
template<>
inline void ProfRecordWrap<false, PROF_LEVEL_FULL>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.record_cpu_full(idx, cost);
}

template<long long COUNT>
struct ProfCountIsGreatOne
{
    static const bool is_bat = COUNT > 1;
};


//RAII小函数  
//用于快速记录<注册条目>的性能信息  
template <long long COUNT = 1, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoRecord
{
public:
    //idx为<注册条目>的ID  
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



//RAII小函数  
//一次性记录并直接输出到日志 不需要提前注册任何条目  
//整体性能影响要稍微高于<注册条目>  但消耗部分并不影响记录本身. 使用在常见的一次性流程或者demo场景中.    
template <long long COUNT = 1LL, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoAnonRecord
{
public:
    ProfAutoAnonRecord(const char* desc)
    {
        strncpy(desc_, desc, PROF_NAME_MAX_SIZE);
        desc_[PROF_NAME_MAX_SIZE - 1] = '\0';
        counter_.start();
    }
    ~ProfAutoAnonRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>(ProfInstType::INNER_PROF_NULL, COUNT, counter_.save().cycles());
        ProfInst.output_temp_record(desc_);
    }

    ProfCounter<C>& counter() { return counter_; }
private:
    ProfCounter<C> counter_;
    char desc_[PROF_NAME_MAX_SIZE];
};



//接口入口   
//作为profile工具, 以宏的形式提供接口, 可在编译环境中随时关闭.   
//实际生产环境中 该工具对性能的整体影响非常小 可在生产环境正常开启  

#ifdef OPEN_ZPROF   

//--------
// 条目注册和关系绑定   
// -------
//注册条目  
#define PROF_REGIST_NODE(id, name, ct, resident, re_reg)  ProfInst.regist(id, name, ct, resident, re_reg)  

//快速注册条目: 提供默认计时方式, 默认该条目不开启常驻模式, 一旦调用clear相关接口该条目记录的信息会被清零.  默认该条目未被注册过 当前为新注册  
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.regist(id, name, PROF_COUNTER_DEFAULT,  false, false)

//快速注册条目: 同上, 名字也默认提供 即ID自身    
#define PROF_FAST_REGIST_NODE(id)  PROF_FAST_REGIST_NODE_ALIAS(id, #id)

//快速注册条目: 同上 但是为常驻条目 
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.regist(id, #id, PROF_COUNTER_DEFAULT,  true, false)  

//绑定展示层级(父子)关系  
#define PROF_BIND_CHILD(id, cid)  ProfInst.bind_childs(id, cid) 

//绑定合并层级(cid->id)关系  合并关系中按照合并方向 合并的目标在前, 要搜集的在后并保持连续 可以获得性能上的跳点优化(也符合线性思维)    
#define PROF_BIND_MERGE(id, cid) ProfInst.bind_merge(id, cid)   

//通常合并关系和展示层级关系一致 这里同时绑定两者  
#define PROF_BIND_CHILD_AND_MERGE(id, cid) do {PROF_BIND_CHILD(id, cid); PROF_BIND_MERGE(id, cid); }while(0)

//注册子条目并绑定展示层级关系    
#define PROF_REG_AND_BIND_CHILD(id, cid)  do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_CHILD(id, cid); } while(0)   
//注册子条目并绑定合并层级关系  
#define PROF_REG_AND_BIND_MERGE(id, cid) do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_MERGE(id, cid); } while(0)  
//注册子条目并同时绑定展示层级和合并层级  
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) do {PROF_FAST_REGIST_NODE(cid);  PROF_BIND_CHILD_AND_MERGE(id, cid); }while(0)


//--------
// PROF启停和输出     
// -------

//初始化全局实例并启动该实例  
#define PROF_INIT(title) ProfInst.init(title)   

//[option] 对注册好的条目进行跳点优化; 不执行则不获得优化  
//放在注册完所有条目后执行, 否则优化只能覆盖执行时已经注册的条目(全量覆写型构建跳点, 无副作用)  
#define PROF_BUILD_JUMP_PATH() ProfInst.build_jump_path()  

//注册输出 默认使用printf  
#define PROF_SET_OUTPUT(out_func) ProfInst.set_output(out_func)

//重置(清零)idx条目以及递归重置其所有子条目  
#define PROF_RESET_CHILD(idx) ProfInst.reset_childs(idx)  

//执行性能数据的层级合并 
//合并层级进行了扁平压缩 
#define PROF_DO_MERGE() ProfInst.do_merge()  

//清零<保留条目>信息(常驻条目除外)  
#define PROF_RESET_RESERVE(...) ProfInst.reset_reserve_node(__VA_ARGS__)  
//清零<注册条目>信息(常驻条目除外)  
#define PROF_RESET_DECLARE(...) ProfInst.reset_declare_node(__VA_ARGS__)  


//--------
// PROF记录       
// 通常完整的计时+记录约为10ns~20ns 
// -------

//记录性能消耗信息 平均耗时约为4ns    
#define PROF_RECORD_CPU_SAMPLE(idx, cost) ProfInst.record_cpu(idx, cost)   

//记录性能消耗信息(携带总耗时和执行次数) 平均耗时约为6ns      
//COUNT为常数 cost为总耗时, 根据记录等级选择性存储 平滑数据, 抖动偏差 等     ProfLevel:PROF_LEVEL_NORMAL  
#define PROF_RECORD_CPU_WRAP(idx, COUNT, cost, PROF_LEVEL)  \
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>((int)(idx), (long long)(COUNT), (long long)cost)  
//记录性能消耗信息: 同上, 但count非常数  
#define PROF_RECORD_CPU_DYN_WRAP(idx, count, cost, PROF_LEVEL)  \
        ProfRecordWrap<true, PROF_LEVEL>((int)(idx), (long long)(count), (long long)cost)

//同PROF_RECORD_CPU_SAMPLE  
#define PROF_RECORD_CPU(idx, cost) PROF_RECORD_CPU_WRAP((idx), 1, (cost), PROF_LEVEL_NORMAL)

//记录内存字节数    
//输出日志时 进行可读性处理 带k,m,g等单位  
#define PROF_RECORD_MEM(idx, count, mem) ProfInst.record_mem(idx, count, mem)  

//记录系统内存信息 包含vm, rss等  
#define PROF_RECORD_VM(idx, vm) ProfInst.record_vm(idx, vm)
#define PROF_OVERWRITE_MEM(idx, count, mem) ProfInst.overwrite_mem(idx, count, mem)

//记录定时器 比较特殊.  根据调用的前后间隔进行累加  
#define PROF_RECORD_TIMER(idx, stamp) ProfInst.record_timer(idx, stamp)  

//记录用户自定义信息 没有额外处理   
#define PROF_RECORD_USER(idx, count, add) ProfInst.record_user(idx, count, add)


//-------手动计时器-----------
//定义一个计时器  
#define PROF_DEFINE_COUNTER(var)  ProfCounter<> var

//定义一个带起始时间戳的计时器(通常场景很少用这个)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  ProfCounter<> tc(start)  

//设置当前时间为 定时器开始时间    
#define PROF_START_COUNTER(var) var.start()   

//重新设置当前时间为 定时器开始时间    
#define PROF_RESTART_COUNTER(var) var.start()   

//设置当前时间为定时器结束时间  
#define PROF_STOP_AND_SAVE_COUNTER(var) var.stop_and_save()  

//设置当前时间为定时器结束时间 并写入idx对应的条目中  
#define PROF_STOP_AND_RECORD(idx, var) PROF_RECORD_CPU_WRAP((idx), 1, (var).stop_and_save().cycles(), PROF_LEVEL_NORMAL)



//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时候写入记录条目)-----------
#define PROF_DEFINE_AUTO_RECORD(var, idx) ProfAutoRecord<> var(idx)   


//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ANON_RECORD(var, desc) ProfAutoAnonRecord<> var(desc)
//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_MULTI_ANON_RECORD(var, count, desc) ProfAutoAnonRecord<count> var(desc)
//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, count, level, ct, desc) ProfAutoAnonRecord<count, level, ct> var(desc)



//使用特殊条目<0>进行一次性输出  
//用于立刻输出性能信息而不是走报告输出  
#define PROF_OUTPUT_TEMP_RECORD(desc)        do {ProfInst.output_temp_record(desc, strlen(desc));}while(0)  

//立刻输出一条信息  
#define PROF_OUTPUT_RECORD(idx)        do {ProfInst.output_one_record(idx);}while(0)

//输出完整报告 (PROF_OUTPUT_FLAG_ALL)   
#define PROF_OUTPUT_REPORT(...)    ProfInst.output_report(__VA_ARGS__)

//其他立即输出
#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  do {ProfRecordWrap<true, PROF_LEVEL_FAST>((int)ProfInstType::INNER_PROF_NULL, (long long)(count), (long long)num);  PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) do {PROF_RECORD_USER(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) do {PROF_RECORD_MEM(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   do {PROF_RECORD_CPU(ProfInstType::INNER_PROF_NULL, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_USER(desc, num) do {PROF_RECORD_USER(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) do {PROF_RECORD_MEM(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)

//输出当前进程的vm/rss信息 
#define PROF_OUTPUT_SELF_MEM(desc) do{PROF_RECORD_VM(ProfInstType::INNER_PROF_NULL, prof_get_mem_use()); PROF_OUTPUT_TEMP_RECORD(desc);}while(0)


#else
#define PROF_REGIST_NODE(id, name, pt, resident, force)
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  
#define PROF_FAST_REGIST_NODE(id) 

#define PROF_FAST_REGIST_RESIDENT_NODE(id)  
#define PROF_BIND_CHILD(id, cid) 
#define PROF_BIND_MERGE(id, cid) 
#define PROF_BIND_CHILD_AND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD(id, cid)  
#define PROF_REG_AND_BIND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) 

#define PROF_INIT(title) 
#define PROF_BUILD_JUMP_PATH()
#define PROF_SET_OUTPUT(log_fun) 

#define PROF_RESET_RESERVE()
#define PROF_RESET_DECLARE() 
#define PROF_RESET_ANON() 
#define PROF_RESET_CHILD(idx) 
#define PROF_DO_MERGE() 


#define PROF_RECORD_CPU_SAMPLE(idx, cost) 
#define PROF_RECORD_CPU(idx, cost) 
#define PROF_RECORD_CPU_WRAP(idx, COUNT, cost, PROF_LEVEL) 
#define PROF_RECORD_CPU_DYN_WRAP(idx, count, cost, PROF_LEVEL)
#define PROF_RECORD_MEM(idx, count, mem) 
#define PROF_RECORD_VM(idx, vm) 
#define PROF_OVERWRITE_MEM(idx, count, mem) 
#define PROF_RECORD_TIMER(idx, stamp) 
#define PROF_RECORD_USER(idx, count, add)

#define PROF_DEFINE_COUNTER(var)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  
#define PROF_START_COUNTER(var) 
#define PROF_RESTART_COUNTER(var) 
#define PROF_STOP_AND_SAVE_COUNTER(var) 
#define PROF_STOP_AND_RECORD(idx, var) 

#define PROF_DEFINE_AUTO_RECORD(var, idx) 
#define PROF_DEFINE_AUTO_ANON_RECORD(desc, idx) 
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, count, level, ct, desc) 

#define PROF_OUTPUT_TEMP_RECORD(desc) 

#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) 
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) 
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   
#define PROF_OUTPUT_SINGLE_USER(desc, num)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) 
#define PROF_OUTPUT_SELF_MEM(desc) 

#define PROF_OUTPUT_REPORT()

#endif











#endif
#ifdef __GNUG__
#pragma GCC pop_options
#endif
