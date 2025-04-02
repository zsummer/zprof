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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__("rdtsc" : "=a"(lo), "=d"(hi));
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
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
#elif defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
        struct timeval tm;
        gettimeofday(&tm, nullptr);
        return tm.tv_sec * 1000 * 1000 * 1000 + tm.tv_usec * 1000;
#else
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
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
    inline double GetCpuFreq()
    {
        double mhz = 1;
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
        mhz = freq;
        mhz /= 1000.0 * 1000.0;
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
#endif // 
        return mhz;
    }



    // 获取时钟的频率 以纳秒为单位 决定时钟的真实精度.    
    // 例如CPU是2.5G 则RDTSC型时钟 频率是2.5/ns  
    template<ClockType _C>
    inline double GetFrequency()
    {
        return 1.0;
    }

    template<>
    inline double GetFrequency<kClockFenceRDTSC>()
    {
        const static double frequency_per_ns = GetCpuFreq() * 1000.0 * 1000.0 / 1000.0 / 1000.0 / 1000.0;
        return frequency_per_ns;
    }
    template<>
    inline double GetFrequency<kClockBTBFenceRDTSC>()
    {
        return GetFrequency<kClockFenceRDTSC>();
    }

    template<>
    inline double GetFrequency<kClockVolatileRDTSC>()
    {
        return GetFrequency<kClockFenceRDTSC>();
    }

    template<>
    inline double GetFrequency<kClockPureRDTSC>()
    {
        return GetFrequency<kClockFenceRDTSC>();
    }

    template<>
    inline double GetFrequency<kClockLockRDTSC>()
    {
        return GetFrequency<kClockFenceRDTSC>();
    }

    template<>
    inline double GetFrequency<kClockMFenceRDTSC>()
    {
        return GetFrequency<kClockFenceRDTSC>();
    }

    template<>
    inline double GetFrequency<kClockBTBMFenceRDTSC>()
    {
        return GetFrequency<kClockFenceRDTSC>();
    }

    template<>
    inline double GetFrequency<kClockRDTSCP>()
    {
        return GetFrequency<kClockFenceRDTSC>();
    }

    template<>
    inline double GetFrequency<kClockClock>()
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
    inline double GetFrequency<kClockSystem>()
    {
        return 1.0;
    }

    template<>
    inline double GetFrequency<kClockChrono>()
    {
        const static double chrono_frequency = 
            std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count() 
            / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    inline double GetFrequency<kClockSteadyChrono>()
    {
        const static double chrono_frequency = 
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)).count() 
            / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    inline double GetFrequency<kClockSystemChrono>()
    {
        const static double chrono_frequency = 
            std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(1)).count() 
            / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    inline double GetFrequency<kClockSystemMS>()
    {
        static double chrono_frequency = 1.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    // 获取频率(纳秒单位)的倒数   
    // 例如CPU 2.5G  RDTSC型时钟的倒数为0.4 代表每次tick的间隔时间是0.4ns 
    template<ClockType _C>
    inline double GetInverseFrequency()
    {
        const static double inverse_frequency_per_ns = 1.0 / (GetFrequency<_C>() <= 0.0 ? 1.0 : GetFrequency<_C>());
        return inverse_frequency_per_ns;
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
        long long cost_ns()const { return (long long)(ticks_ * GetInverseFrequency<_C>()); }
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

    // 用于日志形式的报表格式化方法  
    class Report
    {
    public:
        Report() = delete;

        // 绑定外部内存 
        explicit Report(char* buff, size_t buff_size)
        {
            buff_ = buff;
            buff_len_ = buff_size;
            offset_ = 0;
        }

        // 按输出格式要求 序列化需要的数据类型 
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

        // 快速填充指定长度字符  
        inline Report& PushIndent(int count);
        inline Report& PushHyphen(int count);

        // 添加 c-style string 的结尾  
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

    // 以空格形式提供缩进支持 
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


    // 提供一个简单的静态封装 
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
    //平滑  
    #define SMOOTH_CYCLES(s_ticks, ticks) (   (s_ticks * 12 + ticks * 4) >> 4   ) 
    #define SMOOTH_CYCLES_WITH_INIT(s_ticks, ticks) ( (s_ticks) == 0 ? (ticks) : SMOOTH_CYCLES(s_ticks, ticks) )  
    
    #define UNWIND_STR(str) str, strlen(str)

    // 压缩的条目名长度信息   
    static constexpr int  kCompactDataUnitSize = 30;
    static constexpr int  kCompactDataBuffMinSize = 150;  

    //单个字段的输出对齐长度  
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

    // zprof 核心类:  记录性能,内存,用户自定义数据等  
    // kInst 全局实例ID  
    // kReserve 保留记录条目范围(自动注册,直接使用)   
    // kDeclare 声明条目范围(自行注册后使用)   
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

        // 二次初始化并完成环境数据探测  
        int Init(const char* title);

        //注册条目, 注册条目的数量要小于kDeclare  
        int Regist(int idx, const char* name, unsigned int clk, bool resident, bool re_reg);

        // 获取zprof实例的title  
        const char* Title() const { return &compact_data_[title_]; }

        const char* Name(int idx);
        int Rename(int idx, const char* name);

        // 注册后可以绑定多个node之间的展示层级关系  
        int BindChilds(int idx, int child);

        // 展示层级跳点结构优化 提高输出效率    
        int BuildJumpPath();

        // 注册后可以绑定多个node的数据合并关系 
        int BindMerge(int to, int child);  

        // 数据合并计算   
        void DoMerge();

        // 重置记录的数据 
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

        // 记录CPU开销  
        // 这里的ticks要从zprof::Clock获取, 并且要求和注册的时钟类型一致, 否则输出报告时候可能产生不正确的物理时间换算.   
        // c为统计的次数  
        // ticks为对应次数的总开销  
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

        // 这个方法记录的内容最详细  
        PROF_ALWAYS_INLINE void RecordCpuFull(int idx, long long ticks)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += ticks;
            long long dis = ticks;
            long long avg = node.cpu.sum / node.cpu.c;

            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, ticks);  

            //上下两个水位线的平滑值  
            node.cpu.h_sm = (dis >= avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
            node.cpu.l_sm = (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));

            node.cpu.dv += abs(dis - node.cpu.sm);
            node.cpu.t_u += ticks;
            node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
        }

        // 带count数据 
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


        // 层级递归输出所有报告   
        int OutputCpu(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int OutputMem(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int OutputVm(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int OutputUser(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_padding);
        int RecursiveOutput(int entry_idx, int depth, const char* opt_name, int opt_name_len, Report& rp);


        // 报告输出接口   
        int OutputReport(unsigned int flags = kOutFlagAll);
        int OutputOneRecord(int entry_idx);
        int OutputTempRecord(const char* opt_name, int opt_name_len);
        int OutputTempRecord(const char* opt_name);


    public:
        Report& compact_writer() { return compact_writer_; }
        RecordNode& node(int idx) { return nodes_[idx]; }
        double particle_for_ns(int t) { return  particle_for_ns_[t]; }




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
        // 所有字符串数据都存在一个数组中并以\0为结尾, 使用字符串的地方记录的是对应字符串在该数组的起始下标  
        // 参考ELF符号表的设计     
        char compact_data_[compact_data_size()];
        Report compact_writer_;
        int unknown_desc_;
        int reserve_desc_;
        int no_name_space_;
        int no_name_space_len_;

    private:
        RecordNode nodes_[end_id()];
        int declare_window_;
        double particle_for_ns_[kClockMAX];
    };

    template<int kInst, int kReserve, int kDeclare>
    Record<kInst, kReserve, kDeclare>::Record() : compact_writer_(compact_data_, compact_data_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        merge_leafs_size_ = 0;
        memset(particle_for_ns_, 0, sizeof(particle_for_ns_));
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

        // 获取所有时钟的频率换算信息  
        // 运行时输出报告时直接进行值相乘计算即可获得纳秒为单位的时间    
        particle_for_ns_[kClockNULL] = 0;
        particle_for_ns_[kClockSystem] = GetInverseFrequency<kClockSystem>();
        particle_for_ns_[kClockClock] = GetInverseFrequency<kClockClock>();
        particle_for_ns_[kClockChrono] = GetInverseFrequency<kClockChrono>();
        particle_for_ns_[kClockSteadyChrono] = GetInverseFrequency<kClockSteadyChrono>();
        particle_for_ns_[kClockSystemChrono] = GetInverseFrequency<kClockSystemChrono>();
        particle_for_ns_[kClockSystemMS] = GetInverseFrequency<kClockSystemMS>();
        particle_for_ns_[kClockPureRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockVolatileRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockMFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockLockRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockRDTSCP] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockBTBFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockBTBMFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();

        particle_for_ns_[kClockNULL] = GetInverseFrequency<zprof::kClockDefatultLevel >();

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
                //找到下一个顶层节点  
                if (nodes_[next_upper_id].show.upper == 0)
                {
                    break;
                }
                next_upper_id++;
            }

            // 非顶层节点总是指向下一个顶层节点 减少遍历判定开销  
            // 默认指向下一个 即使不执行跳点优化也是逻辑正确的   
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

        // 窗口策略: 
        // 单个节点的所有子节点一般聚集在一个小的范围内, 最优情况下连续分布    
        // 通过child+window确定最大最小范围, 规避掉使用list造成额外的存储开销和性能浪费  

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

        // 非叶子节点 
        if (node.merge.childs > 0)
        {
            return 0;
        }

        // 合并的目标节点如果也存在向上合并 那么要从当前的叶子节点中剔除    
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


        // 加入到叶子节点列表
        merge_leafs_[merge_leafs_size_++] = child;
        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    void Record<kInst, kReserve, kDeclare>::DoMerge()
    {
        Clock<> clk;
        clk.Start();
        // 所有存在向上合并数据的叶子节点均执行一次数据的合并 
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

            // 1-N层父级   
            do
            {
                node->cpu.t_u += append_cpu;
                node->mem.t_u += append_mem;
                node->merge.merged++;

                // 非叶子节点只有当前所有子叶子节点合并完成后才能继续向上合并 
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
        double cpu_rate = particle_for_ns(node.traits.clk);
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

        //递归输出所有子表
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

// 默认的全局实例ID   
#ifndef PROF_DEFAULT_INST_ID 
#define PROF_DEFAULT_INST_ID 0
#endif

// PROF保留条目 用于记录一些通用的环境性能数据   
#ifndef PROF_RESERVE_COUNT
#define PROF_RESERVE_COUNT 50
#endif 

// PROF主要条目 需要先注册名字和层级关联关系  
#ifndef PROF_DECLARE_COUNT
#define PROF_DECLARE_COUNT 260
#endif 




// 默认的全局实例定义 如需扩展可更换kInst ID使用额外的全局实例      
using ProfInstType = zprof::Record<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT>;
#define ProfInst ProfInstType::Instance()


// 包装函数 根据模版参数在编译阶段直接使用不同的入口  从而减少常见使用场景下的运行时判断消耗.  
template<bool kIsBat, zprof::RecordLevel kLevel>
inline void ProfRecordWrap(int idx, long long count, long long ticks)
{

}

// 记录用宏定义  
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


// RAII小函数  
// 用于快速记录<注册条目>的性能信息  
template <long long kCount = 1, zprof::RecordLevel kLevel = zprof::kRecordLevelNormal,
    zprof::ClockType C = zprof::kClockDefatultLevel>
class ProfAutoRecord
{
public:
    //idx为<注册条目>的ID  
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

// RAII小函数  
// 用于快速记录<注册条目>的性能信息 带诊断信息
template<class AutoRecord>
class ProfDiagnostic
{
public:
    //idx为<注册条目>的ID  
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



// RAII小函数  
// 一次性记录并直接输出到日志 不需要提前注册任何条目  
// 整体性能影响要稍微高于<注册条目>  但消耗部分并不影响记录本身. 使用在常见的一次性流程或者demo场景中.    
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



// 接口入口   
// 作为profile工具, 以宏的形式提供接口, 可在编译环境中随时关闭.   
// 实际生产环境中 该工具对性能的整体影响非常小 可在生产环境正常开启  

#ifdef OPEN_ZPROF   

//--------
// 条目注册和关系绑定   
// -------
// 注册条目  
#define PROF_REGIST_NODE(id, name, ct, resident, re_reg)  ProfInst.Regist(id, name, ct, resident, re_reg)  

// 快速注册条目: 提供默认计时方式, 默认该条目不开启常驻模式, 一旦调用clear相关接口该条目记录的信息会被清零.  默认该条目未被注册过 当前为新注册  
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.Regist(id, name, zprof::kClockDefatultLevel,  false, false)

// 快速注册条目: 同上, 名字也默认提供 即ID自身    
#define PROF_FAST_REGIST_NODE(id)  PROF_FAST_REGIST_NODE_ALIAS(id, #id)

// 快速注册条目: 同上 但是为常驻条目 
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.Regist(id, #id, zprof::kClockDefatultLevel,  true, false)  

// 绑定展示层级(父子)关系  
#define PROF_BIND_CHILD(id, cid)  ProfInst.BindChilds(id, cid) 

// 绑定合并层级(cid->id)关系  合并关系中按照合并方向 合并的目标在前, 要搜集的在后并保持连续 可以获得性能上的跳点优化(也符合线性思维)    
#define PROF_BIND_MERGE(id, cid) ProfInst.BindMerge(id, cid)   

// 通常合并关系和展示层级关系一致 这里同时绑定两者  
#define PROF_BIND_CHILD_AND_MERGE(id, cid) do {PROF_BIND_CHILD(id, cid); PROF_BIND_MERGE(id, cid); }while(0)

// 注册子条目并绑定展示层级关系    
#define PROF_REG_AND_BIND_CHILD(id, cid)  do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_CHILD(id, cid); } while(0)   
// 注册子条目并绑定合并层级关系  
#define PROF_REG_AND_BindMerge(id, cid) do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_MERGE(id, cid); } while(0)  
// 注册子条目并同时绑定展示层级和合并层级  
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) do {PROF_FAST_REGIST_NODE(cid);  PROF_BIND_CHILD_AND_MERGE(id, cid); }while(0)


//--------
// PROF启停和输出     
// -------

// 初始化全局实例并启动该实例  
#define PROF_INIT(Title) ProfInst.Init(Title)   

// [option] 对注册好的条目进行跳点优化; 不执行则不获得优化  
// 放在注册完所有条目后执行, 否则优化只能覆盖执行时已经注册的条目(全量覆写型构建跳点, 无副作用)  
#define PROF_BUILD_JUMP_PATH() ProfInst.BuildJumpPath()  

// 注册输出 默认使用printf  
#define PROF_SET_OUTPUT(out_func) ProfInst.SetOutputFunc(out_func)

// 重置(清零)idx条目以及递归重置其所有子条目  
#define PROF_RESET_CHILD(idx) ProfInst.ResetChilds(idx)  

// 执行性能数据的层级合并 
// 合并层级进行了扁平压缩 
#define PROF_DO_MERGE() ProfInst.DoMerge()  

// 清零<保留条目>信息(常驻条目除外)  
#define PROF_RESET_RESERVE(...) ProfInst.ResetReserveNode(__VA_ARGS__)  
// 清零<注册条目>信息(常驻条目除外)  
#define PROF_RESET_DECLARE(...) ProfInst.ResetDeclareNode(__VA_ARGS__)  


//--------
// PROF记录       
// 通常完整的计时+记录约为10ns~20ns 
// -------

// 记录性能消耗信息 平均耗时约为4ns    
#define PROF_RECORD_CPU_SAMPLE(idx, ticks) ProfInst.RecordCpu(idx, ticks)   

// 记录性能消耗信息(携带总耗时和执行次数) 平均耗时约为6ns      
// kCount为常数 ticks为总耗时, 根据记录等级选择性存储 平滑数据, 抖动偏差 等     RecordLevel:kRecordLevelNormal  
#define PROF_RECORD_CPU_WRAP(idx, kCount, ticks, kLevel)  \
        ProfRecordWrap<ProfCountIsGreatOne<kCount>::is_bat, kLevel>((int)(idx), (long long)(kCount), (long long)ticks)  
// 记录性能消耗信息: 同上, 但count非常数  
#define PROF_RECORD_CPU_DYN_WRAP(idx, count, ticks, kLevel)  \
        ProfRecordWrap<true, kLevel>((int)(idx), (long long)(count), (long long)ticks)

// 同PROF_RECORD_CPU_SAMPLE  
#define PROF_RECORD_CPU(idx, ticks) PROF_RECORD_CPU_WRAP((idx), 1, (ticks), zprof::kRecordLevelNormal)

// 记录内存字节数    
// 输出日志时 进行可读性处理 带k,m,g等单位  
#define PROF_RECORD_MEM(idx, count, mem) ProfInst.RecordMem(idx, count, mem)  

// 记录系统内存信息 包含vm, rss等  
#define PROF_RECORD_VM(idx, vm) ProfInst.RecordVm(idx, vm)
#define PROF_RERECORD_MEM(idx, count, mem) ProfInst.RerecordMem(idx, count, mem)

// 记录定时器 比较特殊.  根据调用的前后间隔进行累加  
#define PROF_RECORD_TIMER(idx, stamp) ProfInst.RecordTimer(idx, stamp)  

// 记录用户自定义信息 没有额外处理   
#define PROF_RECORD_USER(idx, a1, ...) ProfInst.RecordUser(idx, a1, ##__VA_ARGS__)
#define PROF_RERECORD_USER(idx, a1, ...) ProfInst.RecordUser(idx, a1, ##__VA_ARGS__)


// -------手动计时器-----------
// 定义一个计时器  
#define PROF_DEFINE_COUNTER(var)  zprof::Clock<> var

// 定义一个带起始时间戳的计时器(通常场景很少用这个)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  zprof::Clock<> tc(start)  

// 设置当前时间为 定时器开始时间    
#define PROF_START_COUNTER(var) var.Start()   

// 重新设置当前时间为 定时器开始时间    
#define PROF_RESTART_COUNTER(var) var.Start()   

// 设置当前时间为定时器结束时间  
#define PROF_STOP_AND_SAVE_COUNTER(var) var.StopAndSave()  

// 设置当前时间为定时器结束时间 并写入idx对应的条目中  
#define PROF_STOP_AND_RECORD(idx, var) PROF_RECORD_CPU_WRAP((idx), 1, (var).StopAndSave().cost(), zprof::kRecordLevelNormal)



// -------自动计时器(raii包装, 定义时记录开始时间, 销毁时候写入记录条目)-----------
#define PROF_DEFINE_AUTO_RECORD(var, idx) ProfAutoRecord<> var(idx)   


// -------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ANON_RECORD(var, desc) ProfAutoAnonRecord<> var(desc)
// -------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_MULTI_ANON_RECORD(var, count, desc) ProfAutoAnonRecord<> var(desc, count)
// -------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, level, ct, desc) ProfAutoAnonRecord<count, level, ct> var(desc, count)



// 使用特殊条目<0>进行一次性输出  
// 用于立刻输出性能信息而不是走报告输出  
#define PROF_OUTPUT_TEMP_RECORD(desc)        do {ProfInst.OutputTempRecord(desc, (int)strlen(desc));}while(0)  

// 立刻输出一条信息  
#define PROF_OUTPUT_RECORD(idx)        do {ProfInst.OutputOneRecord(idx);}while(0)

// 输出完整报告 (kOutFlagAll)   
#define PROF_OUTPUT_REPORT(...)    ProfInst.OutputReport(__VA_ARGS__)

// 其他立即输出
#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  \
    do {ProfRecordWrap<true, zprof::kRecordLevelFast>((int)ProfInstType::kInnerNull, (long long)(count), (long long)num);  PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, ...) do {PROF_RECORD_USER(ProfInstType::kInnerNull, ##__VA_ARGS__);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) do {PROF_RECORD_MEM(ProfInstType::kInnerNull, count, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   do {PROF_RECORD_CPU(ProfInstType::kInnerNull, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_USER(desc, num) do {PROF_RECORD_USER(ProfInstType::kInnerNull, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) do {PROF_RECORD_MEM(ProfInstType::kInnerNull, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)

// 输出当前进程的vm/rss信息 
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


// 兼容初始命名方案   
template<zprof::ClockType T = zprof::Clock<>::C>
using ProfCounter = zprof::Clock<T>;

using ProfSerializer = zprof::Report;



#endif
#ifdef __GNUG__
#pragma GCC pop_options
#endif
