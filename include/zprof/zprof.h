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

//#define PROF_NO_RDTSC

namespace zprof
{
    // ‰∏çÂêåÁöÑÊó∂ÈíüÁ±ªÂûã   
    enum ClockType
    {
        kClockNULL,
        kClockSystem,
        kClockClock,
        kClockChrono,
        kClockSteadyChrono,
        kClockSystemChrono,
        kClockSystemMS, //wall clock 

        //rdtscÊåá‰ª§‰∏éfenceÁöÑÁªÑÂêà  
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(PROF_NO_RDTSC)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(PROF_NO_RDTSC)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)  && !defined(PROF_NO_RDTSC)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(PROF_NO_RDTSC)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(PROF_NO_RDTSC)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(PROF_NO_RDTSC)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(PROF_NO_RDTSC)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__) && !defined(PROF_NO_RDTSC)
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

#ifdef PROF_NO_RDTSC //apple m1 | some amd chip 
        period = ns_per_second * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)  && defined(__APPLE__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)  //linux 
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
#else
        period = ns_per_second * std::chrono::high_resolution_clock::period().num / std::chrono::high_resolution_clock::period().den;
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




    //clock Â∞ÅË£Ö: Êèê‰æõÁªü‰∏ÄÁöÑ‰ΩøÁî®ÊñπÂºèÂíåËæìÂá∫ 
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
        // ÂêØÂä®ÁªüËÆ° 
        void Start()
        {
            begin_ = GetTick<_C>();
            ticks_ = 0;
        }
        // ËÆ∞ÂΩïelpased ticks  
        ClockBase& Save()
        {
            
            ticks_ = GetTick<_C>() - begin_;
            return *this;
        }

        ClockBase& StopAndSave() { return Save(); }

        // elpased ticks, ÂÖ∂‰ªñÂà´ÂêçÂíåÊó∂Èó¥Âçï‰Ωç 
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

#ifndef ZPROF_REPORT_H_
#define ZPROF_REPORT_H_

#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <array>
#include <limits.h>
#include <chrono>
#include <stdio.h>
#include <cstdarg>
#include <iostream>


namespace zprof
{
    #ifdef WIN32
    #define PROF_LINE_FEED "\r\n"
    #elif (defined __APPLE__)
    #define PROF_LINE_FEED "\r\n"
    #else
    #define PROF_LINE_FEED "\n"
    #endif // WIN32

    static constexpr int kProfNameMaxSize = 50;
    static constexpr int kProfDescMaxSize = 100;
    static constexpr int kProfLineMinSize = 200;
    static constexpr int kProfMaxDepth = 5;

    // ”√”⁄»’÷æ–Œ Ωµƒ±®±Ì∏Ò ΩªØ∑Ω∑®  
    class Report
    {
    public:
        Report() = delete;

        // ∞Û∂®Õ‚≤øƒ⁄¥Ê 
        explicit Report(char* buff, size_t buff_size)
        {
            buff_ = buff;
            buff_len_ = buff_size;
            offset_ = 0;
        }

        // ∞¥ ‰≥ˆ∏Ò Ω“™«Û –Ú¡–ªØ–Ë“™µƒ ˝æ›¿‡–Õ 
        inline Report& PushHumanCount(long long count);
        inline Report& PushHumanTime(long long ns);
        inline Report& PushHumanMem(long long bytes);
        inline Report& PushChar(char ch, int repeat = 1);
        inline Report& PushString(const char* str);
        inline Report& PushString(const char* str, size_t size);
        inline Report& PushNowDate();
        inline Report& PushDate(long long date_ms);
        inline Report& PushNumber(unsigned long long number, int wide = 0);
        inline Report& PushNumber(long long number, int wide = 0);

        // øÏÀŸÃÓ≥‰÷∏∂®≥§∂»◊÷∑˚  
        inline Report& PushIndent(int count);
        inline Report& PushHyphen(int count);

        // ÃÌº” c-style string µƒΩ·Œ≤  
        inline void ClosingString();

        bool IsFull() { return offset_ + 1 >= buff_len_; }  

    public:
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




    inline Report& Report::PushHumanCount(long long count)
    {
        if (buff_len_ <= offset_ + 35)
        {
            return *this;
        }
        if (count > 1000 * 1000)
        {
            PushNumber((unsigned long long)(count / 1000 / 1000));
            PushChar(',');
            PushNumber((unsigned long long)((count / 1000) % 1000), 3);
            PushChar(',');
            PushNumber((unsigned long long)(count % 1000), 3);
            return *this;
        }
        else if (count > 1000)
        {
            PushNumber((unsigned long long)(count / 1000));
            PushChar(',');
            PushNumber((unsigned long long)(count % 1000), 3);
            return *this;
        }
        return PushNumber((unsigned long long)(count));
    }

    inline Report& Report::PushHumanTime(long long ns)
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
            PushString("invalid");
            return *this;
        }
        PushNumber((long long)(ns/ fr));
        buff_[offset_++] = '.';
        PushNumber((unsigned long long)((ns/ mr) % 1000), 3);
        buff_[offset_++] = fc;
        buff_[offset_++] = lc;
        return *this;
    }


    inline Report& Report::PushHumanMem(long long bytes)
    {
        if (buff_len_ <= offset_ + 35)
        {
            return *this;
        }
        if (bytes > 1024 * 1024 * 1024)
        {
            PushNumber((unsigned long long)(bytes / 1024 / 1024 / 1024));
            PushChar('.');
            PushNumber((unsigned long long)((bytes / 1024 / 1024) % 1024)*1000/1024, 3);
            PushChar('G');
            return *this;
        }
        else if (bytes > 1024 * 1024)
        {
            PushNumber((unsigned long long)(bytes / 1024 / 1024));
            PushChar('.');
            PushNumber((unsigned long long)((bytes / 1024) % 1024) * 1000 / 1024, 3);
            PushChar('M');
            return *this;
        }
        else if (bytes > 1024)
        {
            PushNumber((unsigned long long)(bytes / 1024));
            PushChar('.');
            PushNumber((unsigned long long)(bytes % 1024) * 1000 / 1024, 3);
            PushChar('K');
            return *this;
        }
        else
        {
            PushNumber((unsigned long long)bytes);
            PushChar('B');
        }
        return *this;
    }

    inline Report& Report::PushChar(char ch, int repeat)
    {
        while (repeat > 0 && offset_ < buff_len_)
        {
            buff_[offset_++] = ch;
            repeat--;
        }
        return *this;
    }

    inline Report& Report::PushNumber(unsigned long long number, int wide)
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

    inline Report& Report::PushNumber(long long number, int wide)
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
        return PushNumber((unsigned long long)number, wide);
    }

    inline Report& Report::PushString(const char* str)
    {
        return PushString(str, strlen(str));
    }
    inline Report& Report::PushString(const char* str, size_t size)
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

    inline Report& Report::PushDate(long long date_ms)
    {
        time_t ts = date_ms / 1000;
        unsigned int precise = (unsigned int)(date_ms % 1000);

        struct tm tt = { 0 };
#ifdef WIN32
        localtime_s(&tt, &ts);
#else 
        localtime_r(&ts, &tt);
#endif

        PushChar('[');
        PushNumber((unsigned long long)tt.tm_year + 1900, 4);
        PushNumber((unsigned long long)tt.tm_mon + 1, 2);
        PushNumber((unsigned long long)tt.tm_mday, 2);
        PushChar(' ');
        PushNumber((unsigned long long)tt.tm_hour, 2);
        PushChar(':');
        PushNumber((unsigned long long)tt.tm_min, 2);
        PushChar(':');
        PushNumber((unsigned long long)tt.tm_sec, 2);
        PushChar('.');
        PushNumber((unsigned long long)precise, 3);
        PushChar(']');
        return *this;
    }

    inline Report& Report::PushNowDate()
    {
        return PushDate(Clock<>::SystemNowMs());
    }


    inline void Report::ClosingString()
    {
        size_t closed_idx = offset_ >= buff_len_ ? buff_len_ - 1 : offset_;
        buff_[closed_idx] = '\0';
    }

    // “‘ø’∏Ò–Œ ΩÃ·π©ÀıΩ¯÷ß≥÷ 
    inline Report& Report::PushIndent(int count)
    {
        static const char lut[] = "                                                            ";
        constexpr int lut_size = 50;
        static_assert(lut_size < sizeof(lut), "overflow");
        static_assert(lut_size <= kProfNameMaxSize, "meaningless");
        static_assert(lut_size >= kProfMaxDepth*2, "too small");
        if (count > lut_size)
        {
            count = lut_size;
        }
        if (count <= 0)
        {
            return *this;
        }
        if (offset_ + count >= buff_len_)
        {
            return *this;
        }
        memcpy(buff_ + offset_, lut, count);
        offset_ += count;
        return *this;
    }



    inline Report& Report::PushHyphen(int count)
    {
        static const char lut[] = "------------------------------------------------------------";
        constexpr int lut_size = 50;
        static_assert(lut_size < sizeof(lut), "overflow");
        static_assert(lut_size <= kProfLineMinSize, "meaningless");
        static_assert(lut_size >= kProfNameMaxSize, "meaningless");

        if (count > lut_size)
        {
            count = lut_size;
        }
        if (count <= 0)
        {
            return *this;
        }
        if (offset_ + count >= buff_len_)
        {
            return *this;
        }
        memcpy(buff_ + offset_, lut, count);
        offset_ += count;
        return *this;
    }


    // Ã·π©“ª∏ˆºÚµ•µƒæ≤Ã¨∑‚◊∞ 
    class StaticReport : public Report
    {
    public:
        static const int BUFF_SIZE = 350;
        static_assert(BUFF_SIZE > kProfLineMinSize, "");
        StaticReport() :Report(buff_, BUFF_SIZE)
        {
            buff_[0] = '\0';
        }
        ~StaticReport()
        {

        }

    private:
        char buff_[BUFF_SIZE];
    };

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

#ifndef ZPROF_RECORD_H_
#define ZPROF_RECORD_H_

#include <algorithm>
#include <functional>
#include <atomic>




namespace zprof
{
    //∆Ωª¨  
    #define SMOOTH_CYCLES(s_ticks, ticks) (   (s_ticks * 12 + ticks * 4) >> 4   ) 
    #define SMOOTH_CYCLES_WITH_INIT(s_ticks, ticks) ( (s_ticks) == 0 ? (ticks) : SMOOTH_CYCLES(s_ticks, ticks) )  
    
    #define UNWIND_STR(str) str, strlen(str)

    // —πÀıµƒÃıƒø√˚≥§∂»–≈œ¢   
    static constexpr int  kCompactDataUnitSize = 30;
    static constexpr int  kCompactDataBuffMinSize = 150;  

    //µ•∏ˆ◊÷∂Œµƒ ‰≥ˆ∂‘∆Î≥§∂»  
    static constexpr int  kRecordFormatAlignSize = 35;

    enum RecordLevel
    {
        kRecordLevelNormal,
        kRecordLevelFast,
        kRecordLevelFull,
    };


    struct RecordTraits
    {
        int name;
        int name_len;
        int clk;
        bool resident;
    };

    struct RecordMerge
    {
        int to;
        int childs;
        int merged;
    };

    struct RecordShow
    {
        int upper;
        int jumps;
        int child;
        int window;
    };


    struct RecordCPU
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

    struct RecordTimer
    {
        long long last;
    };

    struct RecordMem
    {
        long long c;  
        long long sum;
        long long delta;
        long long t_u;
    };


    struct RecordUser
    {
        long long param1;
        long long param2;
        long long param3;
        long long param4;
    };





    struct RecordNode
    {
        bool active;  
        RecordTraits traits;
        RecordShow show;
        RecordMerge merge;
        RecordCPU cpu; 
        RecordMem mem; 
        RecordTimer timer;
        RecordUser user;
        VMData vm;
    };  

    constexpr static int kNodeSize = sizeof(RecordNode);


    enum OutFlags : unsigned int
    {
        kOutFlagNull,
        kOutFlagInner = 0x1,
        kOutFlagReserve = 0x2,
        kOutFlagDelcare = 0x4,
        kOutFlagAll = 0xffff,
    };

    /*
    #ifdef _FN_LOG_LOG_H_
    static inline void ProfDefaultFNLogFunc(const Report& rp)
    {
        LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
    }
    #endif
    */

    // zprof ∫À–ƒ¿‡:  º«¬º–‘ƒ‹,ƒ⁄¥Ê,”√ªß◊‘∂®“Â ˝æ›µ»  
    // kInst »´æ÷ µ¿˝ID  
    // kReserve ±£¡Ùº«¬ºÃıƒø∑∂Œß(◊‘∂Ø◊¢≤·,÷±Ω” π”√)   
    // kDeclare …˘√˜Ãıƒø∑∂Œß(◊‘––◊¢≤·∫Û π”√)   
    template<int kInst, int kReserve, int kDeclare>
    class Record 
    {
    public:
        using ReportProc = void(*)(const Report& rp);
        enum InnerType
        {
            kInnerNull,
            kInnerInitTs,
            kInnerResetTs,
            kInnerOutputTs,
            kInnerInitCost,
            kInnerMergeCost,
            kInnerReportCost,
            kInnerSerializeCost,
            kInnerOutputCost,
            kInnerClockCost,
            kInnerRecordCost,
            kInnerRecordSmoothCost,
            kInnerRecordFullCost,
            kInnerClockRecordCost,
            kInnerOriginInc,
            kInnerAtomRelax,
            kInnerAtomCost,
            kInnerAtomSeqCost,
            kInnerMax,
        };

        static constexpr int inst_id() { return kInst; }



        static constexpr int reserve_begin_id() { return kInnerMax; }
        static constexpr int reserve_count() { return kReserve; }
        static constexpr int reserve_end_id() { return reserve_begin_id() + reserve_count(); }

        static constexpr int declare_begin_id() { return reserve_end_id(); }
        static constexpr int declare_count() { return kDeclare; }
        static constexpr int declare_end_id() { return declare_begin_id() + declare_count(); }
        inline int declare_window() { return declare_window_; }

        static constexpr int begin_id() { return kInnerNull + 1; }
        static constexpr int count() { return declare_end_id() - 1; }
        static constexpr int end_id() { return begin_id() + count(); }
        static constexpr int max_count() { return count(); }

        static constexpr int compact_data_size() { return kCompactDataUnitSize * (1+end_id()); } //reserve node no name 
        static_assert(end_id() == kInnerMax + reserve_count() + declare_count(), "");


    public:
        static inline Record& Instance()
        {
            static Record inst;
            return inst;
        }

    public:
        Record();

        // ∂˛¥Œ≥ı ºªØ≤¢ÕÍ≥…ª∑æ≥ ˝æ›ÃΩ≤‚  
        int Init(const char* title);

        //◊¢≤·Ãıƒø, ◊¢≤·Ãıƒøµƒ ˝¡ø“™–°”⁄kDeclare  
        int Regist(int idx, const char* name, unsigned int clk, bool resident, bool re_reg);

        // ªÒ»°zprof µ¿˝µƒtitle  
        const char* Title() const { return &compact_data_[title_]; }

        const char* Name(int idx);
        int Rename(int idx, const char* name);

        // ◊¢≤·∫Ûø…“‘∞Û∂®∂‡∏ˆnode÷Æº‰µƒ’π æ≤„º∂πÿœµ  
        int BindChilds(int idx, int child);

        // ’π æ≤„º∂Ã¯µ„Ω·ππ”≈ªØ Ã·∏ﬂ ‰≥ˆ–ß¬     
        int BuildJumpPath();

        // ◊¢≤·∫Ûø…“‘∞Û∂®∂‡∏ˆnodeµƒ ˝æ›∫œ≤¢πÿœµ 
        int BindMerge(int to, int child);  

        //  ˝æ›∫œ≤¢º∆À„   
        void DoMerge();

        // ÷ÿ÷√º«¬ºµƒ ˝æ› 
        PROF_ALWAYS_INLINE  void ResetCpu(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.cpu, 0, sizeof(node.cpu));
            node.cpu.min_u = LLONG_MAX;
        }
        PROF_ALWAYS_INLINE void ResetMem(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.mem, 0, sizeof(node.mem));
        }
        PROF_ALWAYS_INLINE void ResetVm(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.vm, 0, sizeof(node.vm));
        }
        PROF_ALWAYS_INLINE void ResetTimer(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.timer, 0, sizeof(node.timer));
        }
        PROF_ALWAYS_INLINE void ResetUser(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.user, 0, sizeof(node.user));
        }
        PROF_ALWAYS_INLINE void ResetNode(int idx)
        {
            ResetCpu(idx);
            ResetMem(idx);
            ResetVm(idx);
            ResetTimer(idx);
            ResetUser(idx);
        }

        void ResetRangeNode(int first_idx, int end_idx, bool keep_resident = true)
        {
            for (int idx = first_idx; idx < end_idx; idx++)
            {
                if (!keep_resident || !nodes_[idx].traits.resident)
                {
                    ResetNode(idx);
                }
            }
        }

        void ResetReserveNode(bool keep_resident = true)
        {
            ResetRangeNode(reserve_begin_id(), reserve_end_id(), keep_resident);
            RerecordUser(kInnerResetTs, zprof::Clock<>::SystemNowMs());        
        }

        void ResetDeclareNode(bool keep_resident = true)
        {
            ResetRangeNode(declare_begin_id(), declare_end_id(), keep_resident);
            RerecordUser(kInnerResetTs, zprof::Clock<>::SystemNowMs());
        }



        inline void ResetChilds(int idx, int depth = 0);

        // º«¬ºCPUø™œ˙  
        // ’‚¿Ôµƒticks“™¥”zprof::ClockªÒ»°, ≤¢«““™«Û∫Õ◊¢≤·µƒ ±÷”¿‡–Õ“ª÷¬, ∑Ò‘Ú ‰≥ˆ±®∏Ê ±∫Úø…ƒ‹≤˙…˙≤ª’˝»∑µƒŒÔ¿Ì ±º‰ªªÀ„.   
        // cŒ™Õ≥º∆µƒ¥Œ ˝  
        // ticksŒ™∂‘”¶¥Œ ˝µƒ◊‹ø™œ˙  
        PROF_ALWAYS_INLINE void RecordCpu(int idx, long long c, long long ticks)
        {
            long long dis = ticks / c;
            RecordNode& node = nodes_[idx];
            node.cpu.c += c;
            node.cpu.sum += ticks;
            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, ticks);
            node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
            node.cpu.dv += abs(dis - node.cpu.sum/node.cpu.c);
            node.cpu.t_u += ticks;
        }

        PROF_ALWAYS_INLINE void RecordCpu(int idx, long long ticks)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += ticks;
            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, ticks);
            node.cpu.max_u = (node.cpu.max_u < ticks ? ticks : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < ticks ? node.cpu.min_u : ticks);
            node.cpu.dv += abs(ticks - node.cpu.sm);
            node.cpu.t_u += ticks;
        }
        PROF_ALWAYS_INLINE void RecordCpuNoSM(int idx, long long ticks)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += ticks;
            node.cpu.sm = ticks;
            node.cpu.t_u += ticks;
        }
        PROF_ALWAYS_INLINE void RecordCpuNoSM(int idx, long long count, long long ticks)
        {
            long long dis = ticks / count;
            RecordNode& node = nodes_[idx];
            node.cpu.c += count;
            node.cpu.sum += ticks;
            node.cpu.sm = dis;
            node.cpu.t_u += ticks;
        }

        // ’‚∏ˆ∑Ω∑®º«¬ºµƒƒ⁄»›◊ÓœÍœ∏  
        PROF_ALWAYS_INLINE void RecordCpuFull(int idx, long long ticks)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += ticks;
            long long dis = ticks;
            long long avg = node.cpu.sum / node.cpu.c;

            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, ticks);  

            //…œœ¬¡Ω∏ˆÀÆŒªœﬂµƒ∆Ωª¨÷µ  
            node.cpu.h_sm = (dis >= avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
            node.cpu.l_sm = (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));

            node.cpu.dv += abs(dis - node.cpu.sm);
            node.cpu.t_u += ticks;
            node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
        }

        // ¥¯count ˝æ› 
        PROF_ALWAYS_INLINE void RecordCpuFull(int idx, long long c, long long ticks)
        {
        
            RecordNode& node = nodes_[idx];
            node.cpu.c += c;
            node.cpu.sum += ticks;
            long long dis = ticks / c;
            long long avg = node.cpu.sum / node.cpu.c;

            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, ticks);
            node.cpu.h_sm =  (dis > avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
            node.cpu.l_sm =  (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
            node.cpu.dv += abs(dis - node.cpu.sm);
            node.cpu.t_u += ticks;
            node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
        }


        PROF_ALWAYS_INLINE void RecordTimer(int idx, long long stamp)
        {
            RecordNode& node = nodes_[idx];
            if (node.timer.last == 0)
            {
                node.timer.last = stamp;
                return;
            }
            RecordCpuFull(idx, 1, stamp - node.timer.last);
            node.timer.last = stamp;
        }

        PROF_ALWAYS_INLINE void RecordMem(int idx, long long c, long long add)
        {
            RecordNode& node = nodes_[idx];
            node.mem.c += c;
            node.mem.sum += add;
            node.mem.t_u += add;
        }
        PROF_ALWAYS_INLINE void RecordVm(int idx, const VMData& vm)
        {
            nodes_[idx].vm = vm;
        }
        PROF_ALWAYS_INLINE void RecordUser(int idx, long long param1, long long param2 = 0, long long param3 = 0, long long param4 = 0)
        {
            RecordNode& node = nodes_[idx];
            node.user.param1 += param1;
            node.user.param2 += param2;
            node.user.param3 += param3;
            node.user.param4 += param4;
        }
        PROF_ALWAYS_INLINE void RerecordUser(int idx, long long param1, long long param2 = 0, long long param3 = 0, long long param4 = 0)
        {
            RecordNode& node = nodes_[idx];
            node.user.param1 = param1;
            node.user.param2 = param2;
            node.user.param3 = param3;
            node.user.param4 = param4;
        }

        PROF_ALWAYS_INLINE void RerecordMem(int idx, long long c, long long add)
        {
            ResetMem(idx);
            RecordMem(idx, c, add);
        }


        // ≤„º∂µ›πÈ ‰≥ˆÀ˘”–±®∏Ê   
        int OutputCpu(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int OutputMem(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int OutputVm(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int OutputUser(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int RecursiveOutput(int entry_idx, int depth, const char* opt_name, int opt_name_len, Report& rp);


        // ±®∏Ê ‰≥ˆΩ”ø⁄   
        int OutputReport(unsigned int flags = kOutFlagAll);
        int OutputOneRecord(int entry_idx);
        int OutputTempRecord(const char* opt_name, int opt_name_len);
        int OutputTempRecord(const char* opt_name);


    public:
        Report& compact_writer() { return compact_writer_; }
        RecordNode& node(int idx) { return nodes_[idx]; }
        double clock_period(int t) { return  clock_period_[t]; }




     //output interface
    public:
        void SetOutputFunc(ReportProc func) { output_ = func; }
    protected:
        void OutputAndClean(Report& s) { s.ClosingString(); output_(s); s.reset_offset(); }
        static void DefaultOutputFunc(const Report& rp) { printf("%s\n", rp.buff()); }
    public:
        ReportProc output()const { return output_; }
    private:
        ReportProc output_;

    //merge data and interface 
    public:
        std::array<int, end_id()>& merge_leafs() { return merge_leafs_; }
        int merge_leafs_size() { return merge_leafs_size_; }
    private:
        std::array<int, end_id()> merge_leafs_;
        int merge_leafs_size_;



    private:
        int title_;  
        // À˘”–◊÷∑˚¥Æ ˝æ›∂º¥Ê‘⁄“ª∏ˆ ˝◊È÷–≤¢“‘\0Œ™Ω·Œ≤,  π”√◊÷∑˚¥Æµƒµÿ∑Ωº«¬ºµƒ «∂‘”¶◊÷∑˚¥Æ‘⁄∏√ ˝◊Èµƒ∆ ºœ¬±Í  
        // ≤ŒøºELF∑˚∫≈±Ìµƒ…Ëº∆     
        char compact_data_[compact_data_size()];
        Report compact_writer_;
        int unknown_desc_;
        int reserve_desc_;
        int no_name_space_;
        int no_name_space_len_;

    private:
        RecordNode nodes_[end_id()];
        int declare_window_;
        double clock_period_[kClockMAX];
    };

    template<int kInst, int kReserve, int kDeclare>
    Record<kInst, kReserve, kDeclare>::Record() : compact_writer_(compact_data_, compact_data_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        merge_leafs_size_ = 0;
        memset(clock_period_, 0, sizeof(clock_period_));
        declare_window_ = declare_begin_id();

        output_ = &Record::DefaultOutputFunc;  //set default log;

        static_assert(compact_data_size() > kCompactDataBuffMinSize, "");
        compact_data_[0] = '\0';
        unknown_desc_ = 0;
        compact_writer_.PushString("unknown");
        compact_writer_.PushChar('\0');
        reserve_desc_ = (int)compact_writer_.offset();
        compact_writer_.PushString("reserve");
        compact_writer_.PushChar('\0');
        no_name_space_ = (int)compact_writer_.offset();
        compact_writer_.PushString("null(name empty or over buffers)");
        no_name_space_len_ = (int)(compact_writer_.offset() - no_name_space_);
        compact_writer_.PushChar('\0');
        title_ = 0;

    };



    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::Init(const char* title)
    {
        if (title == NULL || compact_writer_.IsFull())
        {
            title_ = 0;
        }
        else
        {
            title_ = (int)compact_writer_.offset();
            compact_writer_.PushString(title);
            compact_writer_.PushChar('\0');
            compact_writer_.ClosingString();
        }
        zprof::Clock<> clk;
        clk.Start();

        // ªÒ»°À˘”– ±÷”µƒ∆µ¬ ªªÀ„–≈œ¢  
        // ‘À–– ± ‰≥ˆ±®∏Ê ±÷±Ω”Ω¯––÷µœ‡≥Àº∆À„º¥ø…ªÒµ√ƒ…√ÎŒ™µ•Œªµƒ ±º‰    
        clock_period_[kClockNULL] = 0;
        clock_period_[kClockSystem] = GetClockPeriod<kClockSystem>();
        clock_period_[kClockClock] = GetClockPeriod<kClockClock>();
        clock_period_[kClockChrono] = GetClockPeriod<kClockChrono>();
        clock_period_[kClockSteadyChrono] = GetClockPeriod<kClockSteadyChrono>();
        clock_period_[kClockSystemChrono] = GetClockPeriod<kClockSystemChrono>();
        clock_period_[kClockSystemMS] = GetClockPeriod<kClockSystemMS>();
        clock_period_[kClockPureRDTSC] = GetClockPeriod<kClockPureRDTSC>();
        clock_period_[kClockVolatileRDTSC] = GetClockPeriod<kClockPureRDTSC>();
        clock_period_[kClockFenceRDTSC] = GetClockPeriod<kClockPureRDTSC>();
        clock_period_[kClockMFenceRDTSC] = GetClockPeriod<kClockPureRDTSC>();
        clock_period_[kClockLockRDTSC] = GetClockPeriod<kClockPureRDTSC>();
        clock_period_[kClockRDTSCP] = GetClockPeriod<kClockPureRDTSC>();
        clock_period_[kClockBTBFenceRDTSC] = GetClockPeriod<kClockPureRDTSC>();
        clock_period_[kClockBTBMFenceRDTSC] = GetClockPeriod<kClockPureRDTSC>();

        clock_period_[kClockNULL] = GetClockPeriod<zprof::kClockDefatultLevel >();

        for (int i = begin_id(); i < reserve_end_id(); i++)
        {
            Regist(i, "reserve", zprof::kClockDefatultLevel, false, false);
        }

        Regist(kInnerNull, "kInnerNull", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerInitTs, "kInnerInitTs", kClockSystemMS, true, true);
        Regist(kInnerResetTs, "kInnerResetTs", kClockSystemMS, true, true);
        Regist(kInnerOutputTs, "kInnerOutputTs", kClockSystemMS, true, true);
        Regist(kInnerInitCost, "kInnerInitCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerMergeCost, "kInnerMergeCost", zprof::kClockDefatultLevel, true, true);

        Regist(kInnerReportCost, "kInnerReportCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerSerializeCost, "kInnerSerializeCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerOutputCost, "kInnerOutputCost", zprof::kClockDefatultLevel, true, true);
    
        Regist(kInnerClockCost, "kInnerClockCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerRecordCost, "kInnerRecordCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerRecordSmoothCost, "kInnerRecordSmoothCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerRecordFullCost, "kInnerRecordFullCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerClockRecordCost, "kInnerClockRecordCost", zprof::kClockDefatultLevel, true, true);

        Regist(kInnerOriginInc, "kInnerOriginInc", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerAtomRelax, "kInnerAtomRelax", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerAtomCost, "kInnerAtomCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerAtomSeqCost, "kInnerAtomSeqCost", zprof::kClockDefatultLevel, true, true);


        if (true)
        {
            RerecordUser(kInnerInitTs, zprof::Clock<>::SystemNowMs());
            RerecordUser(kInnerResetTs, zprof::Clock<>::SystemNowMs());
            RerecordUser(kInnerOutputTs, zprof::Clock<>::SystemNowMs());
        }

        if (true)
        {
            RecordVm(kInnerInitCost, GetSelfMem());
            RecordMem(kInnerInitCost, max_count(), sizeof(*this));
        }

        if (true)
        {
            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                zprof::Clock<> test_cost;
                test_cost.Start();
                test_cost.StopAndSave();
                RecordCpu(kInnerNull, test_cost.cost());
            }
            RecordCpu(kInnerClockRecordCost, 1000, clk.StopAndSave().cost());

            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                clk.Save();
            }
            RecordCpu(kInnerClockCost, 1000, clk.StopAndSave().cost());

            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                RecordCpuNoSM(kInnerNull, clk.StopAndSave().cost());
            }
            RecordCpu(kInnerRecordCost, 1000, clk.StopAndSave().cost());

            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                RecordCpu(kInnerNull, 1, clk.StopAndSave().cost());
            }
            RecordCpu(kInnerRecordSmoothCost, 1000, clk.StopAndSave().cost());

            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                RecordCpuFull(kInnerNull, 1, clk.StopAndSave().cost());
            }
            RecordCpu(kInnerRecordFullCost, 1000, clk.StopAndSave().cost());


            std::atomic<long long> atomll_test(0);
            volatile long long origin_feetch_add_test = 0;
            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                origin_feetch_add_test++;
            }
            RecordCpu(kInnerOriginInc, 1000, clk.StopAndSave().cost());

            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_relaxed);
            }
            RecordCpu(kInnerAtomRelax, 1000, clk.StopAndSave().cost());

            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test++;
            }
            RecordCpu(kInnerAtomCost, 1000, clk.StopAndSave().cost());

        
            clk.Start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_seq_cst);
            }
            RecordCpu(kInnerAtomSeqCost, 1000, clk.StopAndSave().cost());

            ResetNode(kInnerNull);
        }

        RecordCpu(kInnerInitCost, clk.StopAndSave().cost());

        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::BuildJumpPath()
    {
        for (int i = declare_begin_id(); i < declare_end_id(); )
        {
            int next_upper_id = i + 1;
            while (next_upper_id < declare_end_id())
            {
                //’“µΩœ¬“ª∏ˆ∂•≤„Ω⁄µ„  
                if (nodes_[next_upper_id].show.upper == 0)
                {
                    break;
                }
                next_upper_id++;
            }

            // ∑«∂•≤„Ω⁄µ„◊‹ «÷∏œÚœ¬“ª∏ˆ∂•≤„Ω⁄µ„ ºı…Ÿ±È¿˙≈–∂®ø™œ˙  
            // ƒ¨»œ÷∏œÚœ¬“ª∏ˆ º¥ π≤ª÷¥––Ã¯µ„”≈ªØ“≤ «¬ﬂº≠’˝»∑µƒ   
            for (int j = i; j < next_upper_id; j++)
            {
                nodes_[j].show.jumps = next_upper_id - j - 1;
            }
            i = next_upper_id;
        }
        return 0;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::Regist(int idx, const char* name, unsigned int clk, bool resident, bool re_reg)
    {
        if (idx >= end_id() )
        {
            return -1;
        }
        if (name == NULL)
        {
            return -3;
        }
    
    
        RecordNode& node = nodes_[idx];


        if (!re_reg && node.active)
        {
            return 0;
        }

        memset(&node, 0, sizeof(node));
        Rename(idx, name);
        nodes_[idx].traits.clk = clk;
        nodes_[idx].traits.resident = resident;
        node.active = true;
        node.cpu.min_u = LLONG_MAX;

        if (idx >= declare_begin_id() && idx < declare_end_id() && idx + 1 > declare_window_)
        {
            declare_window_ = idx + 1;
        }

        return 0;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::Rename(int idx, const char* name)
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


        nodes_[idx].traits.name = (int)compact_writer_.offset();// node name is "" when compact rp full 
        compact_writer_.PushString(name);
        compact_writer_.PushChar('\0');
        compact_writer_.ClosingString();
        nodes_[idx].traits.name_len = (int)strlen(&compact_data_[nodes_[idx].traits.name]);
        if (nodes_[idx].traits.name_len == 0)
        {
            nodes_[idx].traits.name = no_name_space_;
            nodes_[idx].traits.name_len = no_name_space_len_;
        }
        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    const char* Record<kInst, kReserve, kDeclare>::Name(int idx)
    {
        if (idx < begin_id() || idx >= end_id())
        {
            return "";
        }
        RecordTraits& traits = nodes_[idx].traits;
        if (traits.name >= compact_data_size())
        {
            return "";
        }
        return &compact_data_[traits.name];
    };


    template<int kInst, int kReserve, int kDeclare>
    void Record<kInst, kReserve, kDeclare>::ResetChilds(int idx, int depth)
    {
        if (idx < begin_id() || idx >= end_id())
        {
            return ;
        }
        RecordNode& node = nodes_[idx];
        ResetCpu(idx);
        ResetMem(idx);
        ResetTimer(idx);
        ResetUser(idx);
        if (depth > kProfMaxDepth)
        {
            return;
        }
        for (int i = node.show.child; i < node.show.child + node.show.window; i++)
        {
            RecordNode& child = nodes_[i];
            if (child.show.upper == idx)
            {
               ResetChilds(i, depth + 1);
            }
        }
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::BindChilds(int idx, int cidx)
    {
        if (idx < begin_id() || idx >= end_id() || cidx < begin_id() || cidx >= end_id())
        {
            return -1;
        }

        if (idx == cidx)
        {
            return -2;
        }

        RecordNode& node = nodes_[idx];
        RecordNode& child = nodes_[cidx];
        if (!node.active || !child.active)
        {
            return -3; //Regist method has memset all info ; 
        }

        // ¥∞ø⁄≤ﬂ¬‘: 
        // µ•∏ˆΩ⁄µ„µƒÀ˘”–◊”Ω⁄µ„“ª∞„æ€ºØ‘⁄“ª∏ˆ–°µƒ∑∂Œßƒ⁄, ◊Ó”≈«Èøˆœ¬¡¨–¯∑÷≤º    
        // Õ®π˝child+window»∑∂®◊Ó¥Û◊Ó–°∑∂Œß, πÊ±‹µÙ π”√list‘Ï≥…∂ÓÕ‚µƒ¥Ê¥¢ø™œ˙∫Õ–‘ƒ‹¿À∑—  

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



    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::BindMerge(int to, int child)
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
        RecordNode& node = nodes_[child];
        RecordNode& to_node = nodes_[to];
        if (!node.active || !to_node.active)
        {
            return -3; //Regist method has memset all info ; 
        }

        // duplicate bind   
        if (node.merge.to != 0)
        {
            return -4;
        }

        to_node.merge.childs++;
        node.merge.to = to;

        // ∑«“∂◊”Ω⁄µ„ 
        if (node.merge.childs > 0)
        {
            return 0;
        }

        // ∫œ≤¢µƒƒø±ÍΩ⁄µ„»Áπ˚“≤¥Ê‘⁄œÚ…œ∫œ≤¢ ƒ«√¥“™¥”µ±«∞µƒ“∂◊”Ω⁄µ„÷–Ãﬁ≥˝    
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


        // º”»ÎµΩ“∂◊”Ω⁄µ„¡–±Ì
        merge_leafs_[merge_leafs_size_++] = child;
        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    void Record<kInst, kReserve, kDeclare>::DoMerge()
    {
        Clock<> clk;
        clk.Start();
        // À˘”–¥Ê‘⁄œÚ…œ∫œ≤¢ ˝æ›µƒ“∂◊”Ω⁄µ„æ˘÷¥––“ª¥Œ ˝æ›µƒ∫œ≤¢ 
        for (int i = 0; i < merge_leafs_size_; i++)
        {
            int leaf_id = merge_leafs_[i];
            RecordNode& leaf = nodes_[leaf_id];
            RecordNode* node = NULL;
            long long append_cpu = 0;
            long long append_mem = 0;
            int id = 0;
            node = &nodes_[leaf.merge.to];
            append_cpu = leaf.cpu.t_u;
            append_mem = leaf.mem.t_u;
            id = leaf.merge.to;
            leaf.cpu.t_u = 0;
            leaf.mem.t_u = 0;

            // 1-N≤„∏∏º∂   
            do
            {
                node->cpu.t_u += append_cpu;
                node->mem.t_u += append_mem;
                node->merge.merged++;

                // ∑«“∂◊”Ω⁄µ„÷ª”–µ±«∞À˘”–◊”“∂◊”Ω⁄µ„∫œ≤¢ÕÍ≥…∫Û≤≈ƒ‹ºÃ–¯œÚ…œ∫œ≤¢ 
                if (node->merge.merged >= node->merge.childs)
                {
                    node->merge.merged = 0;
                    append_cpu = node->cpu.t_u;
                    append_mem = node->mem.t_u;
                    if (append_cpu > 0)
                    {
                        RecordCpuFull(id, append_cpu);
                    }
                    if (append_mem > 0)
                    {
                        RecordMem(id, 1, append_mem);
                    }
                    node->cpu.t_u = 0;
                    node->mem.t_u = 0;
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
        RecordCpu(kInnerMergeCost, clk.StopAndSave().cost());
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputCpu(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding)
    {
        if (name == NULL  || name_len + name_padding > kProfDescMaxSize)
        {
            return -10;
        }
        double cpu_rate = clock_period(node.traits.clk);
        zprof::Clock<> single_line_cost;
        single_line_cost.Start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushHyphen(name_padding);
        rp.PushString(UNWIND_STR(" |"));

        rp.PushString(UNWIND_STR("\tcpu*|-- "));
        if (true)
        {
            rp.PushHumanCount(node.cpu.c);
            rp.PushString(UNWIND_STR("c, "));
            rp.PushHumanTime((long long)(node.cpu.sum * cpu_rate / node.cpu.c));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.sum * cpu_rate));
        }


        if (node.cpu.min_u != LLONG_MAX && node.cpu.max_u > 0)
        {
            rp.PushString(UNWIND_STR(" --|\tmax-min:|-- "));
            rp.PushHumanTime((long long)(node.cpu.max_u * cpu_rate));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.min_u * cpu_rate));
        }


        if (node.cpu.dv > 0 || node.cpu.sm > 0)
        {
            rp.PushString(UNWIND_STR(" --|\tdv-sm:|-- "));
            rp.PushHumanTime((long long)(node.cpu.dv * cpu_rate / node.cpu.c));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.sm * cpu_rate));
        }


        if (node.cpu.h_sm > 0 || node.cpu.l_sm > 0)
        {
            rp.PushString(UNWIND_STR(" --|\th-l:|-- "));
            rp.PushHumanTime((long long)(node.cpu.h_sm * cpu_rate));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.l_sm * cpu_rate));
        }
        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());

        single_line_cost.Start();
        OutputAndClean(rp);
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());

        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputMem(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding)
    {
        if (name == NULL || name_len + name_padding > kProfDescMaxSize)
        {
            return -20;
        }
        zprof::Clock<> single_line_cost;
        single_line_cost.Start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushHyphen(name_padding);
        rp.PushString(UNWIND_STR(" |"));

        rp.PushString(UNWIND_STR("\tmem*|-- "));
        if (true)
        {
            rp.PushHumanCount(node.mem.c);
            rp.PushString(UNWIND_STR("c, "));
            rp.PushHumanMem(node.mem.sum / node.mem.c);
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanMem(node.mem.sum);
        }

        rp.PushString(UNWIND_STR(" --||-- "));
        if (node.mem.delta > 0)
        {
            rp.PushHumanMem(node.mem.sum - node.mem.delta);
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanMem(node.mem.delta);
        }
        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());


        single_line_cost.Start();
        OutputAndClean(rp);
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());
        return 0;
    }
    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputVm(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding)
    {
        if (name == NULL || name_len + name_padding > kProfDescMaxSize)
        {
            return -30;
        }
        zprof::Clock<> single_line_cost;
        single_line_cost.Start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushHyphen(name_padding);
        rp.PushString(UNWIND_STR(" |"));


        rp.PushString(UNWIND_STR("\t vm*|-- "));
        if (true)
        {
            rp.PushHumanMem(node.vm.vm_size);
            rp.PushString(UNWIND_STR("(vm), "));
            rp.PushHumanMem(node.vm.rss_size);
            rp.PushString(UNWIND_STR("(rss), "));
            rp.PushHumanMem(node.vm.shr_size);
            rp.PushString(UNWIND_STR("(shr), "));
            rp.PushHumanMem(node.vm.rss_size - node.vm.shr_size);
            rp.PushString(UNWIND_STR("(uss)"));
        }

        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());

        single_line_cost.Start();
        OutputAndClean(rp);
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());
        return 0;
    }
    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputUser(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding)
    {
        if (name == NULL || name_len + name_padding > kProfDescMaxSize)
        {
            return -40;
        }
        zprof::Clock<> single_line_cost;
        single_line_cost.Start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushHyphen(name_padding);
        rp.PushString(UNWIND_STR(" |"));


        rp.PushString(UNWIND_STR("\tuser*|-- "));
        if (true)
        {
            rp.PushHumanCount(node.user.param1);
            rp.PushString(UNWIND_STR(" \t/ "));
            rp.PushHumanCount(node.user.param2);
            rp.PushString(UNWIND_STR(" \t/ "));
            rp.PushHumanCount(node.user.param3);
            rp.PushString(UNWIND_STR(" \t/ "));
            rp.PushHumanCount(node.user.param4);
        }

        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());

        single_line_cost.Start();
        OutputAndClean(rp);
        single_line_cost.StopAndSave();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());
        return 0;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::RecursiveOutput(int entry_idx, int depth, const char* opt_name, int opt_name_len, Report& rp)
    {
        if (entry_idx >= end_id())
        {
            return -1;
        }

        if (rp.buff_len() <= kProfLineMinSize)
        {
            return -2;
        }

        if (output_ == nullptr)
        {
            return -3;
        }

        RecordNode& node = nodes_[entry_idx];

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
        if (node.traits.clk >= kClockMAX)
        {
            return 0;
        }


    
        zprof::Clock<> single_line_cost;

        const char* name = &compact_data_[node.traits.name];
        int name_len = node.traits.name_len;
        
        if (opt_name != NULL)
        {
            name = opt_name;
            name_len = opt_name_len;
        }

        int name_padding = (int)name_len + depth  + depth;
        name_padding = name_padding < kRecordFormatAlignSize ? kRecordFormatAlignSize - name_padding : 0;

        if (name_len + name_padding > kProfDescMaxSize)
        {
            return -5;
        }

        rp.reset_offset();


        if (node.cpu.c > 0)
        {
            OutputCpu(node, rp, entry_idx, depth, name, name_len, name_padding);
        }

        if (node.mem.c > 0)
        {
            OutputMem(node, rp, entry_idx, depth, name, name_len, name_padding);
        }

        if (node.vm.rss_size + node.vm.vm_size > 0)
        {
            OutputVm(node, rp, entry_idx, depth, name, name_len, name_padding);
        }

        if (node.user.param1 != 0 || node.user.param2 != 0 || node.user.param3 != 0 || node.user.param4 != 0)
        {
            OutputUser(node, rp, entry_idx, depth, name, name_len, name_padding);
        }

        if (depth > kProfMaxDepth)
        {
            rp.PushIndent(depth * 2);
            OutputAndClean(rp);
            return -4;
        }

        //µ›πÈ ‰≥ˆÀ˘”–◊”±Ì
        for (int i = node.show.child; i < node.show.child + node.show.window; i++)
        {
            RecordNode& child = nodes_[i];
            if (child.show.upper == entry_idx)
            {
                int ret = RecursiveOutput(i, depth + 1, NULL, 0, rp);
                if (ret < 0)
                {
                    return ret;
                }
            }
        }
        return 0;
    }






    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputOneRecord(int entry_idx)
    {
        StaticReport rp;
        int ret = RecursiveOutput(entry_idx, 0, NULL, 0, rp);
        (void)ret;
        return ret;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputTempRecord(const char* opt_name, int opt_name_len)
    {
        StaticReport rp;
        int ret = RecursiveOutput(0, 0, opt_name, opt_name_len, rp);
        ResetNode(0);//reset  
        return ret;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputTempRecord(const char* opt_name)
    {
        return OutputTempRecord(opt_name, (int)strlen(opt_name));
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputReport(unsigned int flags)
    {
        if (output_ == nullptr)
        {
            return -1;
        }
        RerecordUser(kInnerOutputTs, zprof::Clock<>::SystemNowMs());

        zprof::Clock<> clk;
        clk.Start();
        StaticReport rp;

        rp.reset_offset();
        OutputAndClean(rp);


        rp.PushChar('=', 30);
        rp.PushChar(' ');
        rp.PushString(Title());
        rp.PushString(UNWIND_STR(" output report at: "));
        rp.PushNowDate();
        rp.PushString(UNWIND_STR(" dist start time:["));
        rp.PushHumanTime((Clock<>::SystemNowMs() - nodes_[kInnerInitTs].user.param1)*1000*1000);
        rp.PushString(UNWIND_STR("] dist reset time:["));
        rp.PushHumanTime((Clock<>::SystemNowMs() - nodes_[kInnerResetTs].user.param1) * 1000 * 1000);
        rp.PushChar(']');
        rp.PushChar(' ');

        rp.PushChar('=', 30);
        OutputAndClean(rp);

        rp.PushString(UNWIND_STR("| -- index -- | ---    cpu  ------------ | ----------   hits, avg, sum   ---------- | ---- max, min ---- | ------ dv, sm ------ |  --- hsm, lsm --- | "));
        OutputAndClean(rp);
        rp.PushString(UNWIND_STR("| -- index -- | ---    mem  ---------- | ----------   hits, avg, sum   ---------- | ------ last, delta ------ | "));
        OutputAndClean(rp);
        rp.PushString(UNWIND_STR("| -- index -- | ---    vm  ------------ | ----------   vm, rss, shr, uss   ------------------ | " ));
        OutputAndClean(rp);

        rp.PushString(UNWIND_STR("| -- index -- | ---    user  ----------- | -----------  hits, avg, sum   ---------- | "));
        OutputAndClean(rp);

        if (flags & kOutFlagInner)
        {
            rp.PushString(UNWIND_STR(PROF_LINE_FEED));
            for (int i = kInnerNull + 1; i < kInnerMax; i++)
            {
                int ret = RecursiveOutput(i, 0, NULL, 0, rp);
                (void)ret;
            }
        }

        if (flags & kOutFlagReserve)
        {
            rp.PushString(UNWIND_STR(PROF_LINE_FEED));
            for (int i = reserve_begin_id(); i < reserve_end_id(); i++)
            {
                int ret = RecursiveOutput(i, 0, NULL, 0, rp);
                (void)ret;
            }
        }
    
        if (flags & kOutFlagDelcare)
        {
            rp.PushString(UNWIND_STR(PROF_LINE_FEED));
            for (int i = declare_begin_id(); i < declare_window(); )
            {
                int ret = RecursiveOutput(i, 0, NULL, 0, rp);
                (void)ret;
                i += nodes_[i].show.jumps + 1;
            }
        }

        rp.reset_offset();
        rp.PushChar('=', 30);
        rp.PushChar('\t');
        rp.PushString(" end : ");
        rp.PushNowDate();
        rp.PushChar('\t');
        rp.PushChar('=', 30);
        OutputAndClean(rp);
        OutputAndClean(rp);

        RecordCpu(kInnerReportCost, clk.StopAndSave().cost());
        return 0;
    }

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



#ifndef ZPROF_H_
#define ZPROF_H_

// ƒ¨»œµƒ»´æ÷ µ¿˝ID   
#ifndef PROF_DEFAULT_INST_ID 
#define PROF_DEFAULT_INST_ID 0
#endif

// PROF±£¡ÙÃıƒø ”√”⁄º«¬º“ª–©Õ®”√µƒª∑æ≥–‘ƒ‹ ˝æ›   
#ifndef PROF_RESERVE_COUNT
#define PROF_RESERVE_COUNT 50
#endif 

// PROF÷˜“™Ãıƒø –Ë“™œ»◊¢≤·√˚◊÷∫Õ≤„º∂πÿ¡™πÿœµ  
#ifndef PROF_DECLARE_COUNT
#define PROF_DECLARE_COUNT 260
#endif 




// ƒ¨»œµƒ»´æ÷ µ¿˝∂®“Â »Á–Ë¿©’πø…∏¸ªªkInst ID π”√∂ÓÕ‚µƒ»´æ÷ µ¿˝      
using ProfInstType = zprof::Record<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT>;
#define ProfInst ProfInstType::Instance()


// ∞¸◊∞∫Ø ˝ ∏˘æ›ƒ£∞Ê≤Œ ˝‘⁄±‡“ÎΩ◊∂Œ÷±Ω” π”√≤ªÕ¨µƒ»Îø⁄  ¥”∂¯ºı…Ÿ≥£º˚ π”√≥°æ∞œ¬µƒ‘À–– ±≈–∂œœ˚∫ƒ.  
template<bool kIsBat, zprof::RecordLevel kLevel>
inline void ProfRecordWrap(int idx, long long count, long long ticks)
{

}

// º«¬º”√∫Í∂®“Â  
template<>
inline void ProfRecordWrap<true, zprof::kRecordLevelNormal>(int idx, long long count, long long ticks)
{
    ProfInst.RecordCpu(idx, count, ticks);
}

template<>
inline void ProfRecordWrap<false, zprof::kRecordLevelNormal>(int idx, long long count, long long ticks)
{
    (void)count;
    ProfInst.RecordCpu(idx, ticks);
}
template<>
inline void ProfRecordWrap<true, zprof::kRecordLevelFast>(int idx, long long count, long long ticks)
{
    ProfInst.RecordCpuNoSM(idx, count, ticks);
}
template<>
inline void ProfRecordWrap<false, zprof::kRecordLevelFast>(int idx, long long count, long long ticks)
{
    (void)count;
    ProfInst.RecordCpuNoSM(idx, ticks);
}

template<>
inline void ProfRecordWrap<true, zprof::kRecordLevelFull>(int idx, long long count, long long ticks)
{
    ProfInst.RecordCpuFull(idx, count, ticks);
}
template<>
inline void ProfRecordWrap<false, zprof::kRecordLevelFull>(int idx, long long count, long long ticks)
{
    (void)count;
    ProfInst.RecordCpuFull(idx, ticks);
}

template<long long kCount>
struct ProfCountIsGreatOne
{
    static const bool is_bat = kCount > 1;
};


// RAII–°∫Ø ˝  
// ”√”⁄øÏÀŸº«¬º<◊¢≤·Ãıƒø>µƒ–‘ƒ‹–≈œ¢  
template <long long kCount = 1, zprof::RecordLevel kLevel = zprof::kRecordLevelNormal,
    zprof::ClockType C = zprof::kClockDefatultLevel>
class ProfAutoRecord
{
public:
    //idxŒ™<◊¢≤·Ãıƒø>µƒID  
    ProfAutoRecord(int idx)
    {
        idx_ = idx;
        clock_.Start();
    }
    ~ProfAutoRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<kCount>::is_bat, kLevel>(idx_, kCount, clock_.Save().cost());
    }
    zprof::Clock<C>& clock() { return clock_; }
private:
    zprof::Clock<C> clock_;
    int idx_;
};

// RAII–°∫Ø ˝  
// ”√”⁄øÏÀŸº«¬º<◊¢≤·Ãıƒø>µƒ–‘ƒ‹–≈œ¢ ¥¯’Ô∂œ–≈œ¢
template<class AutoRecord>
class ProfDiagnostic
{
public:
    //idxŒ™<◊¢≤·Ãıƒø>µƒID  
    ProfDiagnostic(int idx, long long watchdog, std::function<void(long long)> dog) :record_(idx)
    {
        watchdog_ = watchdog;
        dog_ = dog;
    }
    ~ProfDiagnostic()
    {
        if (watchdog_ <= 0 || dog_ == nullptr)
        {
            return;
        }
        long long ns = record_.clock().Save().cost_ns();
        if (ns > watchdog_)
        {
            dog_(ns);
        }
    }
private:
    AutoRecord record_;
    long long watchdog_;
    std::function<void(long long)> dog_;
};



// RAII–°∫Ø ˝  
// “ª¥Œ–‘º«¬º≤¢÷±Ω” ‰≥ˆµΩ»’÷æ ≤ª–Ë“™Ã·«∞◊¢≤·»Œ∫ŒÃıƒø  
// ’˚ÃÂ–‘ƒ‹”∞œÏ“™…‘Œ¢∏ﬂ”⁄<◊¢≤·Ãıƒø>  µ´œ˚∫ƒ≤ø∑÷≤¢≤ª”∞œÏº«¬º±æ…Ì.  π”√‘⁄≥£º˚µƒ“ª¥Œ–‘¡˜≥ÃªÚ’ﬂdemo≥°æ∞÷–.    
template <zprof::RecordLevel kLevel = zprof::kRecordLevelNormal,
    zprof::ClockType C = zprof::kClockDefatultLevel>
class ProfAutoAnonRecord
{
public:
    explicit ProfAutoAnonRecord(const char* desc, long long cnt = 1)
    {
        strncpy(desc_, desc, zprof::kProfNameMaxSize);
        desc_[zprof::kProfNameMaxSize - 1] = '\0';
        cnt_ = cnt;
        clock_.Start();
    }
    ~ProfAutoAnonRecord()
    {
        //ProfCountIsGreatOne
        if (cnt_ == 1)
        {
            ProfRecordWrap<false, kLevel>(ProfInstType::kInnerNull, cnt_, clock_.Save().cost());
        }
        else
        {
            ProfRecordWrap<true, kLevel>(ProfInstType::kInnerNull, cnt_, clock_.Save().cost());
        }
        ProfInst.OutputTempRecord(desc_);
    }

    zprof::Clock<C>& clock() { return clock_; }
    long long cnt() const { return cnt_; }
    void set_cnt(long long cnt) { cnt_ = cnt; }
private:
    long long cnt_;
    zprof::Clock<C> clock_;
    char desc_[zprof::kProfNameMaxSize];
};



// Ω”ø⁄»Îø⁄   
// ◊˜Œ™profileπ§æﬂ, “‘∫Íµƒ–Œ ΩÃ·π©Ω”ø⁄, ø…‘⁄±‡“Îª∑æ≥÷–ÀÊ ±πÿ±’.   
//  µº …˙≤˙ª∑æ≥÷– ∏√π§æﬂ∂‘–‘ƒ‹µƒ’˚ÃÂ”∞œÏ∑«≥£–° ø…‘⁄…˙≤˙ª∑æ≥’˝≥£ø™∆Ù  

#ifdef OPEN_ZPROF   

//--------
// Ãıƒø◊¢≤·∫Õπÿœµ∞Û∂®   
// -------
// ◊¢≤·Ãıƒø  
#define PROF_REGIST_NODE(id, name, ct, resident, re_reg)  ProfInst.Regist(id, name, ct, resident, re_reg)  

// øÏÀŸ◊¢≤·Ãıƒø: Ã·π©ƒ¨»œº∆ ±∑Ω Ω, ƒ¨»œ∏√Ãıƒø≤ªø™∆Ù≥£◊§ƒ£ Ω, “ªµ©µ˜”√clearœ‡πÿΩ”ø⁄∏√Ãıƒøº«¬ºµƒ–≈œ¢ª·±ª«Â¡„.  ƒ¨»œ∏√ÃıƒøŒ¥±ª◊¢≤·π˝ µ±«∞Œ™–¬◊¢≤·  
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.Regist(id, name, zprof::kClockDefatultLevel,  false, false)

// øÏÀŸ◊¢≤·Ãıƒø: Õ¨…œ, √˚◊÷“≤ƒ¨»œÃ·π© º¥ID◊‘…Ì    
#define PROF_FAST_REGIST_NODE(id)  PROF_FAST_REGIST_NODE_ALIAS(id, #id)

// øÏÀŸ◊¢≤·Ãıƒø: Õ¨…œ µ´ «Œ™≥£◊§Ãıƒø 
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.Regist(id, #id, zprof::kClockDefatultLevel,  true, false)  

// ∞Û∂®’π æ≤„º∂(∏∏◊”)πÿœµ  
#define PROF_BIND_CHILD(id, cid)  ProfInst.BindChilds(id, cid) 

// ∞Û∂®∫œ≤¢≤„º∂(cid->id)πÿœµ  ∫œ≤¢πÿœµ÷–∞¥’’∫œ≤¢∑ΩœÚ ∫œ≤¢µƒƒø±Í‘⁄«∞, “™À—ºØµƒ‘⁄∫Û≤¢±£≥÷¡¨–¯ ø…“‘ªÒµ√–‘ƒ‹…œµƒÃ¯µ„”≈ªØ(“≤∑˚∫œœﬂ–‘ÀºŒ¨)    
#define PROF_BIND_MERGE(id, cid) ProfInst.BindMerge(id, cid)   

// Õ®≥£∫œ≤¢πÿœµ∫Õ’π æ≤„º∂πÿœµ“ª÷¬ ’‚¿ÔÕ¨ ±∞Û∂®¡Ω’ﬂ  
#define PROF_BIND_CHILD_AND_MERGE(id, cid) do {PROF_BIND_CHILD(id, cid); PROF_BIND_MERGE(id, cid); }while(0)

// ◊¢≤·◊”Ãıƒø≤¢∞Û∂®’π æ≤„º∂πÿœµ    
#define PROF_REG_AND_BIND_CHILD(id, cid)  do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_CHILD(id, cid); } while(0)   
// ◊¢≤·◊”Ãıƒø≤¢∞Û∂®∫œ≤¢≤„º∂πÿœµ  
#define PROF_REG_AND_BindMerge(id, cid) do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_MERGE(id, cid); } while(0)  
// ◊¢≤·◊”Ãıƒø≤¢Õ¨ ±∞Û∂®’π æ≤„º∂∫Õ∫œ≤¢≤„º∂  
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) do {PROF_FAST_REGIST_NODE(cid);  PROF_BIND_CHILD_AND_MERGE(id, cid); }while(0)


//--------
// PROF∆ÙÕ£∫Õ ‰≥ˆ     
// -------

// ≥ı ºªØ»´æ÷ µ¿˝≤¢∆Ù∂Ø∏√ µ¿˝  
#define PROF_INIT(Title) ProfInst.Init(Title)   

// [option] ∂‘◊¢≤·∫√µƒÃıƒøΩ¯––Ã¯µ„”≈ªØ; ≤ª÷¥––‘Ú≤ªªÒµ√”≈ªØ  
// ∑≈‘⁄◊¢≤·ÕÍÀ˘”–Ãıƒø∫Û÷¥––, ∑Ò‘Ú”≈ªØ÷ªƒ‹∏≤∏«÷¥–– ±“—æ≠◊¢≤·µƒÃıƒø(»´¡ø∏≤–¥–ÕππΩ®Ã¯µ„, Œﬁ∏±◊˜”√)  
#define PROF_BUILD_JUMP_PATH() ProfInst.BuildJumpPath()  

// ◊¢≤· ‰≥ˆ ƒ¨»œ π”√printf  
#define PROF_SET_OUTPUT(out_func) ProfInst.SetOutputFunc(out_func)

// ÷ÿ÷√(«Â¡„)idxÃıƒø“‘º∞µ›πÈ÷ÿ÷√∆‰À˘”–◊”Ãıƒø  
#define PROF_RESET_CHILD(idx) ProfInst.ResetChilds(idx)  

// ÷¥–––‘ƒ‹ ˝æ›µƒ≤„º∂∫œ≤¢ 
// ∫œ≤¢≤„º∂Ω¯––¡À±‚∆Ω—πÀı 
#define PROF_DO_MERGE() ProfInst.DoMerge()  

// «Â¡„<±£¡ÙÃıƒø>–≈œ¢(≥£◊§Ãıƒø≥˝Õ‚)  
#define PROF_RESET_RESERVE(...) ProfInst.ResetReserveNode(__VA_ARGS__)  
// «Â¡„<◊¢≤·Ãıƒø>–≈œ¢(≥£◊§Ãıƒø≥˝Õ‚)  
#define PROF_RESET_DECLARE(...) ProfInst.ResetDeclareNode(__VA_ARGS__)  


//--------
// PROFº«¬º       
// Õ®≥£ÕÍ’˚µƒº∆ ±+º«¬º‘ºŒ™10ns~20ns 
// -------

// º«¬º–‘ƒ‹œ˚∫ƒ–≈œ¢ ∆Ωæ˘∫ƒ ±‘ºŒ™4ns    
#define PROF_RECORD_CPU_SAMPLE(idx, ticks) ProfInst.RecordCpu(idx, ticks)   

// º«¬º–‘ƒ‹œ˚∫ƒ–≈œ¢(–Ø¥¯◊‹∫ƒ ±∫Õ÷¥––¥Œ ˝) ∆Ωæ˘∫ƒ ±‘ºŒ™6ns      
// kCountŒ™≥£ ˝ ticksŒ™◊‹∫ƒ ±, ∏˘æ›º«¬ºµ»º∂—°‘Ò–‘¥Ê¥¢ ∆Ωª¨ ˝æ›, ∂∂∂Ø∆´≤Ó µ»     RecordLevel:kRecordLevelNormal  
#define PROF_RECORD_CPU_WRAP(idx, kCount, ticks, kLevel)  \
        ProfRecordWrap<ProfCountIsGreatOne<kCount>::is_bat, kLevel>((int)(idx), (long long)(kCount), (long long)ticks)  
// º«¬º–‘ƒ‹œ˚∫ƒ–≈œ¢: Õ¨…œ, µ´count∑«≥£ ˝  
#define PROF_RECORD_CPU_DYN_WRAP(idx, count, ticks, kLevel)  \
        ProfRecordWrap<true, kLevel>((int)(idx), (long long)(count), (long long)ticks)

// Õ¨PROF_RECORD_CPU_SAMPLE  
#define PROF_RECORD_CPU(idx, ticks) PROF_RECORD_CPU_WRAP((idx), 1, (ticks), zprof::kRecordLevelNormal)

// º«¬ºƒ⁄¥Ê◊÷Ω⁄ ˝    
//  ‰≥ˆ»’÷æ ± Ω¯––ø…∂¡–‘¥¶¿Ì ¥¯k,m,gµ»µ•Œª  
#define PROF_RECORD_MEM(idx, count, mem) ProfInst.RecordMem(idx, count, mem)  

// º«¬ºœµÕ≥ƒ⁄¥Ê–≈œ¢ ∞¸∫¨vm, rssµ»  
#define PROF_RECORD_VM(idx, vm) ProfInst.RecordVm(idx, vm)
#define PROF_RERECORD_MEM(idx, count, mem) ProfInst.RerecordMem(idx, count, mem)

// º«¬º∂® ±∆˜ ±»ΩœÃÿ ‚.  ∏˘æ›µ˜”√µƒ«∞∫Ûº‰∏ÙΩ¯––¿€º”  
#define PROF_RECORD_TIMER(idx, stamp) ProfInst.RecordTimer(idx, stamp)  

// º«¬º”√ªß◊‘∂®“Â–≈œ¢ √ª”–∂ÓÕ‚¥¶¿Ì   
#define PROF_RECORD_USER(idx, a1, ...) ProfInst.RecordUser(idx, a1, ##__VA_ARGS__)
#define PROF_RERECORD_USER(idx, a1, ...) ProfInst.RecordUser(idx, a1, ##__VA_ARGS__)


// ------- ÷∂Øº∆ ±∆˜-----------
// ∂®“Â“ª∏ˆº∆ ±∆˜  
#define PROF_DEFINE_COUNTER(var)  zprof::Clock<> var

// ∂®“Â“ª∏ˆ¥¯∆ º ±º‰¥¡µƒº∆ ±∆˜(Õ®≥£≥°æ∞∫‹…Ÿ”√’‚∏ˆ)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  zprof::Clock<> tc(start)  

// …Ë÷√µ±«∞ ±º‰Œ™ ∂® ±∆˜ø™ º ±º‰    
#define PROF_START_COUNTER(var) var.Start()   

// ÷ÿ–¬…Ë÷√µ±«∞ ±º‰Œ™ ∂® ±∆˜ø™ º ±º‰    
#define PROF_RESTART_COUNTER(var) var.Start()   

// …Ë÷√µ±«∞ ±º‰Œ™∂® ±∆˜Ω· ¯ ±º‰  
#define PROF_STOP_AND_SAVE_COUNTER(var) var.StopAndSave()  

// …Ë÷√µ±«∞ ±º‰Œ™∂® ±∆˜Ω· ¯ ±º‰ ≤¢–¥»Îidx∂‘”¶µƒÃıƒø÷–  
#define PROF_STOP_AND_RECORD(idx, var) PROF_RECORD_CPU_WRAP((idx), 1, (var).StopAndSave().cost(), zprof::kRecordLevelNormal)



// -------◊‘∂Øº∆ ±∆˜(raii∞¸◊∞, ∂®“Â ±º«¬ºø™ º ±º‰, œ˙ªŸ ±∫Ú–¥»Îº«¬ºÃıƒø)-----------
#define PROF_DEFINE_AUTO_RECORD(var, idx) ProfAutoRecord<> var(idx)   


// -------◊‘∂Øº∆ ±∆˜(raii∞¸◊∞, ∂®“Â ±º«¬ºø™ º ±º‰, œ˙ªŸ ±÷±Ω” ‰≥ˆ–‘ƒ‹–≈œ¢µΩ»’÷æ)-----------
#define PROF_DEFINE_AUTO_ANON_RECORD(var, desc) ProfAutoAnonRecord<> var(desc)
// -------◊‘∂Øº∆ ±∆˜(raii∞¸◊∞, ∂®“Â ±º«¬ºø™ º ±º‰, œ˙ªŸ ±÷±Ω” ‰≥ˆ–‘ƒ‹–≈œ¢µΩ»’÷æ)-----------
#define PROF_DEFINE_AUTO_MULTI_ANON_RECORD(var, count, desc) ProfAutoAnonRecord<> var(desc, count)
// -------◊‘∂Øº∆ ±∆˜(raii∞¸◊∞, ∂®“Â ±º«¬ºø™ º ±º‰, œ˙ªŸ ±÷±Ω” ‰≥ˆ–‘ƒ‹–≈œ¢µΩ»’÷æ)-----------
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, level, ct, desc) ProfAutoAnonRecord<count, level, ct> var(desc, count)



//  π”√Ãÿ ‚Ãıƒø<0>Ω¯––“ª¥Œ–‘ ‰≥ˆ  
// ”√”⁄¡¢øÃ ‰≥ˆ–‘ƒ‹–≈œ¢∂¯≤ª «◊ﬂ±®∏Ê ‰≥ˆ  
#define PROF_OUTPUT_TEMP_RECORD(desc)        do {ProfInst.OutputTempRecord(desc, (int)strlen(desc));}while(0)  

// ¡¢øÃ ‰≥ˆ“ªÃı–≈œ¢  
#define PROF_OUTPUT_RECORD(idx)        do {ProfInst.OutputOneRecord(idx);}while(0)

//  ‰≥ˆÕÍ’˚±®∏Ê (kOutFlagAll)   
#define PROF_OUTPUT_REPORT(...)    ProfInst.OutputReport(__VA_ARGS__)

// ∆‰À˚¡¢º¥ ‰≥ˆ
#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  \
    do {ProfRecordWrap<true, zprof::kRecordLevelFast>((int)ProfInstType::kInnerNull, (long long)(count), (long long)num);  PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, ...) do {PROF_RECORD_USER(ProfInstType::kInnerNull, ##__VA_ARGS__);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) do {PROF_RECORD_MEM(ProfInstType::kInnerNull, count, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   do {PROF_RECORD_CPU(ProfInstType::kInnerNull, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_USER(desc, num) do {PROF_RECORD_USER(ProfInstType::kInnerNull, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) do {PROF_RECORD_MEM(ProfInstType::kInnerNull, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)

//  ‰≥ˆµ±«∞Ω¯≥Ãµƒvm/rss–≈œ¢ 
#define PROF_OUTPUT_SELF_MEM(desc) do{PROF_RECORD_VM(ProfInstType::kInnerNull, zprof::GetSelfMem()); PROF_OUTPUT_TEMP_RECORD(desc);}while(0)
#define PROF_OUTPUT_SYS_MEM(desc) do{PROF_RECORD_VM(ProfInstType::kInnerNull, zprof::GetSysMem()); PROF_OUTPUT_TEMP_RECORD(desc);}while(0)


#else
#define PROF_REGIST_NODE(id, name, pt, resident, force)
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  
#define PROF_FAST_REGIST_NODE(id) 

#define PROF_FAST_REGIST_RESIDENT_NODE(id)  
#define PROF_BIND_CHILD(id, cid) 
#define PROF_BIND_MERGE(id, cid) 
#define PROF_BIND_CHILD_AND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD(id, cid)  
#define PROF_REG_AND_BindMerge(id, cid) 
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) 

#define PROF_INIT(Title) 
#define PROF_BUILD_JUMP_PATH()
#define PROF_SET_OUTPUT(log_fun) 

#define PROF_RESET_RESERVE()
#define PROF_RESET_DECLARE() 
#define PROF_RESET_ANON() 
#define PROF_RESET_CHILD(idx) 
#define PROF_DO_MERGE() 


#define PROF_RECORD_CPU_SAMPLE(idx, ticks) 
#define PROF_RECORD_CPU(idx, ticks) 
#define PROF_RECORD_CPU_WRAP(idx, kCount, ticks, kLevel) 
#define PROF_RECORD_CPU_DYN_WRAP(idx, count, ticks, kLevel)
#define PROF_RECORD_MEM(idx, count, mem) 
#define PROF_RERECORD_MEM(idx, count, mem) 
#define PROF_RECORD_VM(idx, vm) 
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


// ºÊ»›≥ı º√¸√˚∑Ω∞∏   
template<zprof::ClockType T = zprof::Clock<>::C>
using ProfCounter = zprof::Clock<T>;

using ProfSerializer = zprof::Report;



#endif
#ifdef __GNUG__
#pragma GCC pop_options
#endif
