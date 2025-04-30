
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


#ifndef ZPROF_CLOCK_H_
#define ZPROF_CLOCK_H_

#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <array>
#include <limits.h>
#include <chrono>
#include <string.h>
#include <string>
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
#include <mach/mach.h>
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

namespace zprof
{
    // 不同的时钟类型   
    enum ClockType
    {
        kClockNULL,
        kClockSystem,
        kClockClock,
        kClockChrono,
        kClockSteadyChrono,
        kClockSystemChrono,
        kClockSystemMS, //wall clock 

        //rdtsc指令与fence的组合  
        kClockPureRDTSC,
        kClockVolatileRDTSC,
        kClockFenceRDTSC,
        kClockMFenceRDTSC,
        kClockLockRDTSC,
        kClockRDTSCP,
        kClockBTBFenceRDTSC,
        kClockBTBMFenceRDTSC,

        kClockMAX,
    };


    struct VMData
    {
        //don't use u64 or long long; 
        unsigned long long vm_size;
        unsigned long long rss_size;
        unsigned long long shr_size;
    };

    template<ClockType _C>
    inline long long GetTick()
    {
        return 0;
    }

    template<>
    inline long long GetTick<kClockFenceRDTSC>()
    {
#ifdef WIN32
        _mm_lfence();
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long GetTick<kClockBTBFenceRDTSC>()
    {
#ifdef WIN32
        long long ret = 0;
        _mm_lfence();
        ret = (long long)__rdtsc();
        _mm_lfence();
        return ret;
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("lfence;rdtsc;lfence" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }


    template<>
    inline long long GetTick<kClockVolatileRDTSC>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)  && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi) ::);
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long GetTick<kClockPureRDTSC>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));  //pure need __volatile__ too  . 
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long GetTick<kClockLockRDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__("lock addq $0, 0(%%rsp); rdtsc" : "=a"(lo), "=d"(hi)::"memory");
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }


    template<>
    inline long long GetTick<kClockMFenceRDTSC>()
    {
#ifdef WIN32
        long long ret = 0;
        _mm_mfence();
        ret = (long long)__rdtsc();
        _mm_mfence();
        return ret;
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("mfence;rdtsc;mfence" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long GetTick<kClockBTBMFenceRDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (long long)__rdtsc();
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("mfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }

    template<>
    inline long long GetTick<kClockRDTSCP>()
    {
#ifdef WIN32
        unsigned int ui = 0;
        return (long long)__rdtscp(&ui);
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(__APPLE__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"memory");
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
#endif
    }


    template<>
    inline long long GetTick<kClockClock>()
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
    inline long long GetTick<kClockSystem>()
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
    inline long long GetTick<kClockChrono>()
    {
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long GetTick<kClockSteadyChrono>()
    {
        return std::chrono::steady_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long GetTick<kClockSystemChrono>()
    {
        return std::chrono::system_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long GetTick<kClockSystemMS>()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }



    inline VMData GetSelfMem()
    {
        VMData vm = { 0ULL, 0ULL, 0ULL };
#ifdef WIN32
        HANDLE hproc = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hproc, &pmc, sizeof(pmc)))
        {
            CloseHandle(hproc);// ignore  
            vm.vm_size = (unsigned long long)pmc.WorkingSetSize;
            vm.rss_size = (unsigned long long)pmc.WorkingSetSize;
        }
#elif defined(__APPLE__)
        mach_task_basic_info info;
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        kern_return_t kr = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count);
        if (kr != KERN_SUCCESS)
        {
            return vm;
        }
        vm.rss_size = info.resident_size;
        vm.vm_size = info.resident_size_max;
        vm.shr_size = 0;
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

    inline VMData GetSysMem()
    {
        VMData vm = { 0ULL, 0ULL, 0ULL };
#ifdef WIN32
        MEMORYSTATUS state = { 0 };
        GlobalMemoryStatus(&state);

        vm.vm_size = (unsigned long long)state.dwTotalPhys;
        vm.rss_size = (unsigned long long)(state.dwTotalPhys - state.dwAvailPhys);
#elif defined(__APPLE__)

        size_t len = sizeof(vm.vm_size);
        sysctlbyname("hw.memsize", &vm.vm_size, &len, NULL, 0);
        len = sizeof(vm.shr_size);
        sysctlbyname("hw.pagesize", &vm.shr_size, &len, NULL, 0);
        len = sizeof(vm.rss_size);
        sysctlbyname("hw.page_free_count", &vm.rss_size, &len, NULL, 0);    
        vm.rss_size = vm.vm_size - vm.shr_size * vm.rss_size; 
        vm.shr_size = 0;

#else
        const char* file = "/proc/meminfo";
        FILE* fp = fopen(file, "r");
        if (fp != NULL)
        {
            char line_buff[256];
            while (fgets(line_buff, sizeof(line_buff), fp) != NULL)
            {
                std::string line = line_buff;
                //std::transform(line.begin(), line.end(), line.begin(), ::toupper);
                if (line.compare(0, strlen("MemTotal:"), "MemTotal:", strlen("MemTotal:")) == 0)
                {
                    int ret = sscanf(line.c_str() + strlen("MemTotal:"), "%lld", &vm.vm_size);
                    if (ret == 1)
                    {
                        vm.vm_size *= 1024;
                    }
                }

                if (line.compare(0, strlen("MemFree:"), "MemFree:", strlen("MemFree:")) == 0)
                {
                    int ret = sscanf(line.c_str() + strlen("MemFree:"), "%lld", &vm.rss_size);
                    if (ret == 1)
                    {
                        vm.rss_size *= 1024;
                    }
                }

                if (line.compare(0, strlen("Cached:"), "Cached:", strlen("Cached:")) == 0)
                {
                    int ret = sscanf(line.c_str() + strlen("Cached:"), "%lld", &vm.shr_size);
                    if (ret == 1)
                    {
                        vm.shr_size *= 1024;
                    }
                }
                if (vm.rss_size != 0 && vm.shr_size != 0 && vm.vm_size != 0)
                {
                    vm.rss_size = vm.vm_size - (vm.rss_size + vm.shr_size);
                    vm.shr_size = 0;
                    break;
                }

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

    // U: nanosecond   
    inline double GetCpuPeriod()
    {
        double period = 0;
        double ns_per_second = 1000 * 1000 * 1000;
//#ifdef __APPLE__
#ifdef __APPLE__
#if defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
        int mib[2];
        unsigned int freq;
        size_t len;
        mib[0] = CTL_HW;
        mib[1] = HW_CPU_FREQ;
        len = sizeof(freq);
        sysctl(mib, 2, &freq, &len, NULL, 0);
        if (freq == 0) //error
        {
            return 0;
        }
        period = ns_per_second / freq;
        //all apple use chrono  
        period = ns_per_second * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
#else
        period = ns_per_second * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
#endif
#elif (defined WIN32)
        SYSTEM_INFO si = { 0 };
        GetSystemInfo(&si);
        std::array< PROF_PROCESSOR_POWER_INFORMATION, 128> pppi;
        DWORD dwSize = sizeof(PROF_PROCESSOR_POWER_INFORMATION) * si.dwNumberOfProcessors;
        memset(&pppi[0], 0, dwSize);
        long ret = CallNtPowerInformation(ProcessorInformation, NULL, 0, &pppi[0], dwSize);
        if (ret != 0 || pppi[0].MaxMhz <= 0)
        {
            return 0;
        }
        period = pppi[0].MaxMhz;
        period *= 1000 * 1000;  //mhz --> hz 
        period = ns_per_second / period;
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

                int ret = sscanf(p, "%lf", &period);
                if (ret <= 0)
                {
                    period = 0;
                    break;
                }
                break;
            }
        }
        fclose(fp);
        period *= 1000 * 1000; //mhz --> hz 
        if (period < 1)
        {
            return 0;
        }
        period = ns_per_second / period;

#endif // 
        return period;
    }





    template<ClockType _C>
    inline double GetClockPeriod()
    {
        return 1.0;  //1 ns
    }

    template<>
    inline double GetClockPeriod<kClockFenceRDTSC>()
    {
        const static double period = GetCpuPeriod();
        return period;
    }
    template<>
    inline double GetClockPeriod<kClockBTBFenceRDTSC>()
    {
        return GetClockPeriod<kClockFenceRDTSC>();
    }

    template<>
    inline double GetClockPeriod<kClockVolatileRDTSC>()
    {
        return GetClockPeriod<kClockFenceRDTSC>();
    }

    template<>
    inline double GetClockPeriod<kClockPureRDTSC>()
    {
        return GetClockPeriod<kClockFenceRDTSC>();
    }

    template<>
    inline double GetClockPeriod<kClockLockRDTSC>()
    {
        return GetClockPeriod<kClockFenceRDTSC>();
    }

    template<>
    inline double GetClockPeriod<kClockMFenceRDTSC>()
    {
        return GetClockPeriod<kClockFenceRDTSC>();
    }

    template<>
    inline double GetClockPeriod<kClockBTBMFenceRDTSC>()
    {
        return GetClockPeriod<kClockFenceRDTSC>();
    }

    template<>
    inline double GetClockPeriod<kClockRDTSCP>()
    {
        return GetClockPeriod<kClockFenceRDTSC>();
    }

    template<>
    inline double GetClockPeriod<kClockClock>()
    {
#ifdef WIN32
        static double period = 0.0;
        if (period == 0.0)
        {
            LARGE_INTEGER win_freq;
            win_freq.QuadPart = 0;
            QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
            if (win_freq.QuadPart == 0)
            {
                win_freq.QuadPart = 1;
            }
            period = 1000.0 * 1000.0 * 1000.0 / win_freq.QuadPart;
        }
        return period;
#else
        return 1.0; //1 ns
#endif
    }

    template<>
    inline double GetClockPeriod<kClockSystem>()
    {
        return 1.0; //1 ns
    }

    template<>
    inline double GetClockPeriod<kClockChrono>()
    {
        return 1000.0 * 1000.0 * 1000.0 * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
    }

    template<>
    inline double GetClockPeriod<kClockSteadyChrono>()
    {
        return 1000.0 * 1000.0 * 1000.0 * std::chrono::steady_clock::period().num / std::chrono::steady_clock::period().den;
    }

    template<>
    inline double GetClockPeriod<kClockSystemChrono>()
    {
        return 1000.0 * 1000.0 * 1000.0 * std::chrono::system_clock::period().num / std::chrono::system_clock::period().den;
    }

    template<>
    inline double GetClockPeriod<kClockSystemMS>()
    {
        return 1000.0 * 1000.0 * 1000.0 / 1000;
    }




    //clock 封装: 提供统一的使用方式和输出 
    template<ClockType _C = kClockVolatileRDTSC>
    class ClockBase
    {
    public:
        static constexpr ClockType C = _C;

    private:
        long long begin_;
        long long ticks_;
    public:
        long long get_begin() const { return begin_; }
        long long get_ticks() const { return ticks_; }
        long long get_end() const { return begin_ + ticks_; }

        void set_begin(long long val) { begin_ = val; }
        void set_ticks(long long ticks) { ticks_ = ticks; }
    public:
        ClockBase()
        {
            begin_ = 0;
            ticks_ = 0;
        }
        ClockBase(long long start_clock)
        {
            begin_ = start_clock;
            ticks_ = 0;
        }
        ClockBase(const ClockBase& c)
        {
            begin_ = c.begin_;
            ticks_ = c.ticks_;
        }
        // 启动统计 
        void Start()
        {
            begin_ = GetTick<_C>();
            ticks_ = 0;
        }
        // 记录elpased ticks  
        ClockBase& Save()
        {
            
            ticks_ = GetTick<_C>() - begin_;
            return *this;
        }

        ClockBase& StopAndSave() { return Save(); }

        // elpased ticks, 其他别名和时间单位 
        long long ticks()const { return ticks_; }
        long long cycles()const { return ticks_; }
        long long cost()const { return ticks_; }
        long long cost_ns()const { return (long long)(ticks_ * GetClockPeriod<_C>()); }
        long long cost_ms()const { return cost_ns() / 1000 / 1000; }
        double cost_s() const { return (double)cost_ns() / (1000.0 * 1000.0 * 1000.0); }

        //utils  
    public:
        static long long Now() { return GetTick<_C>(); }
        static long long SystemNowNs() { return GetTick<kClockSystem>(); }
        static long long SystemNowUs() { return GetTick<kClockSystem>() / 1000; }
        static long long SystemNowMs() { return GetTick<kClockSystem>() / 1000 / 1000; }
        static double SystemNowS() { return GetTick<kClockSystem>() / 1000 / 1000 / 1000;}
        static VMData GetSelfMem() { return GetSelfMem(); }
        static VMData GetSysMem() { return GetSysMem(); }
    };

    template<ClockType _C = ClockBase<>::C>
    using Clock = ClockBase<_C>;
    using VMData = VMData;
    constexpr static zprof::ClockType kClockDefatultLevel = Clock<>::C;



}








#endif
