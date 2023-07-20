
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


#ifndef ZPROF_CLOCK_H
#define ZPROF_CLOCK_H

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

    enum ClockType
    {
        CLOCK_NULL,
        CLOCK_SYS,
        CLOCK_CLOCK,
        CLOCK_CHRONO,
        CLOCK_CHRONO_STEADY,
        CLOCK_CHRONO_SYS,
        CLOCK_CHRONO_SYS_MS, //降低到ms精度 用于record时自由输入来自其他计时来源的数据  

        CLOCK_RDTSC_PURE,
        CLOCK_RDTSC_NOFENCE,
        CLOCK_RDTSC,
        CLOCK_RDTSC_BTB,
        CLOCK_RDTSCP,
        CLOCK_RDTSC_MFENCE,
        CLOCK_RDTSC_MFENCE_BTB,

        CLOCK_RDTSC_LOCK,
        CLOCK_MAX,
    };
#ifndef CLOCK_DEFAULT
#define CLOCK_DEFAULT CLOCK_RDTSC_NOFENCE
#endif 


    struct ProfVM
    {
        unsigned long long vm_size;
        unsigned long long rss_size;
        unsigned long long shr_size;
    };









    template<ClockType T>
    PROF_ALWAYS_INLINE long long get_tick()
    {
        return 0;
    }

    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSC>()
    {
#ifdef WIN32
        _mm_lfence();
        return (long long)__rdtsc();
#else
        unsigned int lo, hi;
        __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) ::);
        uint64_t val = ((uint64_t)hi << 32) | lo;
        return (long long)val;
#endif
    }

    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSC_BTB>()
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
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSC_NOFENCE>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#else
        unsigned long hi, lo;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi) ::);
        uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
        return (long long)val;
#endif
    }

    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSC_PURE>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#else
        unsigned long hi, lo;
        __asm__("rdtsc" : "=a"(lo), "=d"(hi));
        uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
        return (long long)val;
#endif
    }

    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSC_LOCK>()
    {
#ifdef WIN32
        _mm_mfence();
        return (long long)__rdtsc();
#else
        unsigned long hi, lo;
        __asm__("lock addq $0, 0(%%rsp); rdtsc" : "=a"(lo), "=d"(hi)::"memory");
        uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
        return (long long)val;
#endif
    }


    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSC_MFENCE>()
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
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSC_MFENCE_BTB>()
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
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_RDTSCP>()
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
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_CLOCK>()
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
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_SYS>()
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
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_CHRONO>()
    {
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
    }

    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_CHRONO_STEADY>()
    {
        return std::chrono::steady_clock().now().time_since_epoch().count();
    }

    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_CHRONO_SYS>()
    {
        return std::chrono::system_clock().now().time_since_epoch().count();
    }

    template<>
    PROF_ALWAYS_INLINE long long get_tick<CLOCK_CHRONO_SYS_MS>()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    inline ProfVM get_self_mem()
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


    inline ProfVM get_sys_mem()
    {
        ProfVM vm = { 0ULL, 0ULL, 0ULL };
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
    inline double get_cpu_mhz()
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




    template<ClockType T>
    PROF_ALWAYS_INLINE double get_frequency()
    {
        return 1.0;
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSC>()
    {
        static double frequency_per_ns = get_cpu_mhz() * 1000.0 * 1000.0 / 1000.0 / 1000.0 / 1000.0;
        return frequency_per_ns;
    }
    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSC_BTB>()
    {
        return get_frequency<CLOCK_RDTSC>();
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSC_NOFENCE>()
    {
        return get_frequency<CLOCK_RDTSC>();
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSC_PURE>()
    {
        return get_frequency<CLOCK_RDTSC>();
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSC_LOCK>()
    {
        return get_frequency<CLOCK_RDTSC>();
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSC_MFENCE>()
    {
        return get_frequency<CLOCK_RDTSC>();
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSC_MFENCE_BTB>()
    {
        return get_frequency<CLOCK_RDTSC>();
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_RDTSCP>()
    {
        return get_frequency<CLOCK_RDTSC>();
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_CLOCK>()
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
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_SYS>()
    {
        return 1.0;
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_CHRONO>()
    {
        static double chrono_frequency = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_CHRONO_STEADY>()
    {
        static double chrono_frequency = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_CHRONO_SYS>()
    {
        static double chrono_frequency = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }
    template<>
    PROF_ALWAYS_INLINE double get_frequency<CLOCK_CHRONO_SYS_MS>()
    {
        static double chrono_frequency = 1.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }
    template<ClockType T>
    PROF_ALWAYS_INLINE double get_inverse_frequency()
    {
        static double inverse_frequency_per_ns = 1.0 / (get_frequency<T>() <= 0.0 ? 1.0 : get_frequency<T>());
        return inverse_frequency_per_ns;
    }











    template<ClockType T = CLOCK_DEFAULT>
    class Clock
    {
    public:
        Clock()
        {
            start_tick_ = 0;
            ticks_ = 0;
        }
        Clock(long long val)
        {
            start_tick_ = val;
            ticks_ = 0;
        }
        void start()
        {
            start_tick_ = get_tick<T>();
            ticks_ = 0;
        }

        Clock& save()
        {
            ticks_ = get_tick<T>() - start_tick_;
            return *this;
        }

        Clock& stop_and_save() { return save(); }

        long long start_val() const { return start_tick_; }
        long long stop_val() const { return start_tick_ + ticks_; }
        
        long long cycles() const { return ticks_; }
        long long duration_ticks() const { return ticks_; }
        long long duration_ns() const { return (long long)(ticks_ * get_inverse_frequency<T>()); }
        double duration_second() const { return (double)duration_ns() / (1000.0 * 1000.0 * 1000.0); }

        void set_start_val(long long val) { start_tick_ = val; }
        void set_ticks_val(long long cycles) { ticks_ = cycles; }
    private:
        long long start_tick_;
        long long ticks_;
    };








}








#endif
