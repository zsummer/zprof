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

    enum clock_type
    {
        T_CLOCK_NULL,
        T_CLOCK_SYS,
        T_CLOCK_CLOCK,
        T_CLOCK_CHRONO,
        T_CLOCK_STEADY_CHRONO,
        T_CLOCK_SYS_CHRONO,
        T_CLOCK_SYS_MS, //wall clock 

        T_CLOCK_PURE_RDTSC,
        T_CLOCK_VOLATILE_RDTSC,
        T_CLOCK_FENCE_RDTSC,
        T_CLOCK_MFENCE_RDTSC,
        T_CLOCK_LOCK_RDTSC,
        T_CLOCK_RDTSCP,
        T_CLOCK_BTB_FENCE_RDTSC,
        T_CLOCK_BTB_MFENCE_RDTSC,

        T_CLOCK_MAX,
    };


    struct vmdata
    {
        //don't use u64 or long long; uint64_t maybe is long ;
        unsigned long long vm_size;
        unsigned long long rss_size;
        unsigned long long shr_size;
    };

    template<clock_type _C>
    inline long long get_tick()
    {
        return 0;
    }

    template<>
    inline long long get_tick<T_CLOCK_FENCE_RDTSC>()
    {
#ifdef WIN32
        _mm_lfence();
        return (long long)__rdtsc();
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#endif
    }

    template<>
    inline long long get_tick<T_CLOCK_BTB_FENCE_RDTSC>()
    {
#ifdef WIN32
        long long ret = 0;
        _mm_lfence();
        ret = (long long)__rdtsc();
        _mm_lfence();
        return ret;
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("lfence;rdtsc;lfence" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#endif
    }


    template<>
    inline long long get_tick<T_CLOCK_VOLATILE_RDTSC>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi) ::);
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#endif
    }

    template<>
    inline long long get_tick<T_CLOCK_PURE_RDTSC>()
    {
#ifdef WIN32
        return (long long)__rdtsc();
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__("rdtsc" : "=a"(lo), "=d"(hi));
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#endif
    }

    template<>
    inline long long get_tick<T_CLOCK_LOCK_RDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (long long)__rdtsc();
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__("lock addq $0, 0(%%rsp); rdtsc" : "=a"(lo), "=d"(hi)::"memory");
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#endif
    }


    template<>
    inline long long get_tick<T_CLOCK_MFENCE_RDTSC>()
    {
#ifdef WIN32
        long long ret = 0;
        _mm_mfence();
        ret = (long long)__rdtsc();
        _mm_mfence();
        return ret;
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("mfence;rdtsc;mfence" : "=a" (lo), "=d" (hi) ::);
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#endif
    }

    template<>
    inline long long get_tick<T_CLOCK_BTB_MFENCE_RDTSC>()
    {
#ifdef WIN32
        _mm_mfence();
        return (long long)__rdtsc();
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("mfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
        unsigned long long val = ((unsigned long long)hi << 32) | lo;
        return (long long)val;
#endif
    }

    template<>
    inline long long get_tick<T_CLOCK_RDTSCP>()
    {
#ifdef WIN32
        unsigned int ui = 0;
        return (long long)__rdtscp(&ui);
#else
        unsigned int lo = 0;
        unsigned int hi = 0;
        __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi)::"memory");
        unsigned long long val = (((unsigned long long)hi) << 32 | ((unsigned long long)lo));
        return (long long)val;
#endif
    }


    template<>
    inline long long get_tick<T_CLOCK_CLOCK>()
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
    inline long long get_tick<T_CLOCK_SYS>()
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
    inline long long get_tick<T_CLOCK_CHRONO>()
    {
        return std::chrono::high_resolution_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long get_tick<T_CLOCK_STEADY_CHRONO>()
    {
        return std::chrono::steady_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long get_tick<T_CLOCK_SYS_CHRONO>()
    {
        return std::chrono::system_clock().now().time_since_epoch().count();
    }

    template<>
    inline long long get_tick<T_CLOCK_SYS_MS>()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }



    inline vmdata get_self_mem()
    {
        vmdata vm = { 0ULL, 0ULL, 0ULL };
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

    inline vmdata get_sys_mem()
    {
        vmdata vm = { 0ULL, 0ULL, 0ULL };
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
    inline double get_cpu_freq()
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




    template<clock_type _C>
    inline double get_frequency()
    {
        return 1.0;
    }

    template<>
    inline double get_frequency<T_CLOCK_FENCE_RDTSC>()
    {
        const static double frequency_per_ns = get_cpu_freq() * 1000.0 * 1000.0 / 1000.0 / 1000.0 / 1000.0;
        return frequency_per_ns;
    }
    template<>
    inline double get_frequency<T_CLOCK_BTB_FENCE_RDTSC>()
    {
        return get_frequency<T_CLOCK_FENCE_RDTSC>();
    }

    template<>
    inline double get_frequency<T_CLOCK_VOLATILE_RDTSC>()
    {
        return get_frequency<T_CLOCK_FENCE_RDTSC>();
    }

    template<>
    inline double get_frequency<T_CLOCK_PURE_RDTSC>()
    {
        return get_frequency<T_CLOCK_FENCE_RDTSC>();
    }

    template<>
    inline double get_frequency<T_CLOCK_LOCK_RDTSC>()
    {
        return get_frequency<T_CLOCK_FENCE_RDTSC>();
    }

    template<>
    inline double get_frequency<T_CLOCK_MFENCE_RDTSC>()
    {
        return get_frequency<T_CLOCK_FENCE_RDTSC>();
    }

    template<>
    inline double get_frequency<T_CLOCK_BTB_MFENCE_RDTSC>()
    {
        return get_frequency<T_CLOCK_FENCE_RDTSC>();
    }

    template<>
    inline double get_frequency<T_CLOCK_RDTSCP>()
    {
        return get_frequency<T_CLOCK_FENCE_RDTSC>();
    }

    template<>
    inline double get_frequency<T_CLOCK_CLOCK>()
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
    inline double get_frequency<T_CLOCK_SYS>()
    {
        return 1.0;
    }

    template<>
    inline double get_frequency<T_CLOCK_CHRONO>()
    {
        const static double chrono_frequency = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    inline double get_frequency<T_CLOCK_STEADY_CHRONO>()
    {
        const static double chrono_frequency = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    inline double get_frequency<T_CLOCK_SYS_CHRONO>()
    {
        const static double chrono_frequency = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(1)).count() / 1000.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<>
    inline double get_frequency<T_CLOCK_SYS_MS>()
    {
        static double chrono_frequency = 1.0 / 1000.0 / 1000.0;
        return chrono_frequency;
    }

    template<clock_type _C>
    inline double get_inverse_frequency()
    {
        const static double inverse_frequency_per_ns = 1.0 / (get_frequency<_C>() <= 0.0 ? 1.0 : get_frequency<_C>());
        return inverse_frequency_per_ns;
    }




    template<clock_type _C = T_CLOCK_VOLATILE_RDTSC>
    class zclock_base
    {
    public:
        static constexpr clock_type C = _C;

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
        zclock_base()
        {
            begin_ = 0;
            ticks_ = 0;
        }
        zclock_base(long long start_clock)
        {
            begin_ = start_clock;
            ticks_ = 0;
        }
        zclock_base(const zclock_base& c)
        {
            begin_ = c.begin_;
            ticks_ = c.ticks_;
        }
        void start()
        {
            begin_ = get_tick<_C>();
            ticks_ = 0;
        }

        zclock_base& save()
        {
            ticks_ = get_tick<_C>() - begin_;
            return *this;
        }

        zclock_base& stop_and_save() { return save(); }

        long long ticks()const { return ticks_; }
        long long cycles()const { return ticks_; }
        long long cost()const { return ticks_; }
        long long cost_ns()const { return (long long)(ticks_ * get_inverse_frequency<_C>()); }
        long long cost_ms()const { return cost_ns() / 1000 / 1000; }
        double cost_s() const { return (double)cost_ns() / (1000.0 * 1000.0 * 1000.0); }

        //utils  
    public:
        static long long now() { return get_tick<_C>(); }
        static long long sys_now_ns() { return get_tick<T_CLOCK_SYS>(); }
        static long long sys_now_us() { return get_tick<T_CLOCK_SYS>() / 1000; }
        static long long sys_now_ms() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
        static double sys_now_s() { return std::chrono::duration<double>(std::chrono::system_clock().now().time_since_epoch()).count(); }
        static vmdata get_self_mem() { return get_self_mem(); }
        static vmdata get_sys_mem() { return get_sys_mem(); }
    };

    template<clock_type _C = zclock_base<>::C>
    using Clock = zclock_base<_C>;
    using VMData = vmdata;
    constexpr static zprof::clock_type CLOCK_DEFAULT = Clock<>::C;



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

#ifndef ZPROF_REPORT_H
#define ZPROF_REPORT_H

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


    #define PROF_NAME_MAX_SIZE 50  
    #define PROF_DESC_MAX_SIZE 100
    #define PROF_LINE_MIN_SIZE 200
    #define PROF_MAX_DEPTH 5

    class Report
    {
    public:
        Report() = delete;
        explicit Report(char* buff, size_t buff_size)
        {
            buff_ = buff;
            buff_len_ = buff_size;
            offset_ = 0;
        }


        inline Report& push_human_count(long long count);
        inline Report& push_human_time(long long ns);
        inline Report& push_human_mem(long long bytes);
        inline Report& push_char(char ch, int repeat = 1);
        inline Report& push_string(const char* str);
        inline Report& push_string(const char* str, size_t size);
        inline Report& push_now_date();
        inline Report& push_date(long long date_ms);
        inline Report& push_number(unsigned long long number, int wide = 0);
        inline Report& push_number(long long number, int wide = 0);

        inline Report& push_indent(int count);
        inline Report& push_blank(int count);


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




    inline Report& Report::push_human_count(long long count)
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

    inline Report& Report::push_human_time(long long ns)
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


    inline Report& Report::push_human_mem(long long bytes)
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

    inline Report& Report::push_char(char ch, int repeat)
    {
        while (repeat > 0 && offset_ < buff_len_)
        {
            buff_[offset_++] = ch;
            repeat--;
        }
        return *this;
    }

    inline Report& Report::push_number(unsigned long long number, int wide)
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

    inline Report& Report::push_number(long long number, int wide)
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

    inline Report& Report::push_string(const char* str)
    {
        return push_string(str, strlen(str));
    }
    inline Report& Report::push_string(const char* str, size_t size)
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

    inline Report& Report::push_date(long long date_ms)
    {
        time_t ts = date_ms / 1000;
        unsigned int precise = (unsigned int)(date_ms % 1000);

        struct tm tt = { 0 };
#ifdef WIN32
        localtime_s(&tt, &ts);
#else 
        localtime_r(&ts, &tt);
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

    inline Report& Report::push_now_date()
    {
        return push_date(Clock<>::sys_now_ms());
    }


    inline void Report::closing_string()
    {
        size_t closed_id = offset_ >= buff_len_ ? buff_len_ - 1 : offset_;
        buff_[closed_id] = '\0';
    }

    inline Report& Report::push_indent(int count)
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



    inline Report& Report::push_blank(int count)
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

    class StaticReport : public Report
    {
    public:
        static const int BUFF_SIZE = 350;
        static_assert(BUFF_SIZE > PROF_LINE_MIN_SIZE, "");
        StaticReport() :Report(buff_, BUFF_SIZE)
        {
            buff_[0] = '\0';
        }
        ~StaticReport()
        {

        }

    private:
        char buff_[BUFF_SIZE];//1k  
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

#ifndef ZPROF_RECORD_H
#define ZPROF_RECORD_H

#include <algorithm>
#include <functional>
#include <atomic>




namespace zprof
{

    #define SMOOTH_CYCLES(s_ticks, ticks) (   (s_ticks * 12 + ticks * 4) >> 4   ) 
    #define SMOOTH_CYCLES_WITH_INIT(s_ticks, ticks) ( (s_ticks) == 0 ? (ticks) : SMOOTH_CYCLES(s_ticks, ticks) )

    enum RecordLevel
    {
        RECORD_LEVEL_NORMAL,
        RECORD_LEVEL_FAST,
        RECORD_LEVEL_FULL,
    };


    struct RecordTraits
    {
        int name;
        int name_len;
        int clk;
        bool resident;
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
        long long c;
        long long sum;
        long long t_u;
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

    enum OutFlags : unsigned int
    {
        OUT_FLAG_NULL,
        OUT_FLAG_INNER = 0x1,
        OUT_FLAG_RESERVE = 0x2,
        OUT_FLAG_DELCARE = 0x4,
        OUT_FLAG_ALL = 0xffff,
    };

    /*
    #ifdef _FN_LOG_LOG_H_
    static inline void ProfDefaultFNLogFunc(const Report& rp)
    {
        LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
    }
    #endif
    */

    template<int INST, int RESERVE, int DECLARE>
    class Record 
    {
    public:
        using ReportProc = void(*)(const Report& rp);
        enum InnerType
        {
            INNER_NULL,
            INNER_INIT_TS,
            INNER_RESET_TS,
            INNER_OUTPUT_TS,
            INNER_INIT_COST,
            INNER_MERGE_COST,
            INNER_REPORT_COST,
            INNER_SERIALIZE_COST,
            INNER_OUTPUT_COST,
            INNER_MEM_INFO_COST,
            INNER_CLOCK_COST,
            INNER_RECORD_COST,
            INNER_RECORD_SM_COST,
            INNER_RECORD_FULL_COST,
            INNER_CLOCK_RECORD_COST,
            INNER_ORIGIN_INC,
            INNER_ATOM_RELEAX,
            INNER_ATOM_COST,
            INNER_ATOM_SEQ_COST,
            INNER_MAX,
        };

        static constexpr int inst_id() { return INST; }



        static constexpr int reserve_begin_id() { return INNER_MAX; }
        static constexpr int reserve_count() { return RESERVE; }
        static constexpr int reserve_end_id() { return reserve_begin_id() + reserve_count(); }

        static constexpr int declare_begin_id() { return reserve_end_id(); }
        static constexpr int declare_count() { return DECLARE; }
        static constexpr int declare_end_id() { return declare_begin_id() + declare_count(); }
        inline int declare_window() { return declare_window_; }

        static constexpr int begin_id() { return INNER_NULL + 1; }
        static constexpr int count() { return declare_end_id() - 1; }
        static constexpr int end_id() { return begin_id() + count(); }
        static constexpr int max_count() { return count(); }

        static constexpr int compact_data_size() { return 30 * (1+end_id()); } //reserve node no name 
        static_assert(end_id() == INNER_MAX + reserve_count() + declare_count(), "");


    public:
        static inline Record& instance()
        {
            static Record inst;
            return inst;
        }

    public:
        Record();
        int init(const char* title);
        int regist(int idx, const char* name, unsigned int clk, bool resident, bool re_reg);
        const char* title() const { return &compact_data_[title_]; }

        const char* name(int idx);
        int rename(int idx, const char* name);


        int bind_childs(int idx, int child);
        int build_jump_path();

        int bind_merge(int to, int child);
        void do_merge();

        PROF_ALWAYS_INLINE  void reset_cpu(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.cpu, 0, sizeof(node.cpu));
            node.cpu.min_u = LLONG_MAX;
        }
        PROF_ALWAYS_INLINE void reset_mem(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.mem, 0, sizeof(node.mem));
        }
        PROF_ALWAYS_INLINE void reset_vm(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.vm, 0, sizeof(node.vm));
        }
        PROF_ALWAYS_INLINE void reset_timer(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.timer, 0, sizeof(node.timer));
        }
        PROF_ALWAYS_INLINE void reset_user(int idx)
        {
            RecordNode& node = nodes_[idx];
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
            overwrite_user(INNER_RESET_TS, 1, zprof::Clock<>::sys_now_ms());        
        }

        void reset_declare_node(bool keep_resident = true)
        {
            reset_range_node(declare_begin_id(), declare_end_id(), keep_resident);
            overwrite_user(INNER_RESET_TS, 1, zprof::Clock<>::sys_now_ms());
        }



        inline void reset_childs(int idx, int depth = 0);

        PROF_ALWAYS_INLINE void record_cpu(int idx, long long c, long long ticks)
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
        PROF_ALWAYS_INLINE void record_cpu(int idx, long long ticks)
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
        PROF_ALWAYS_INLINE void record_cpu_no_sm(int idx, long long ticks)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += ticks;
            node.cpu.sm = ticks;
            node.cpu.t_u += ticks;
        }
        PROF_ALWAYS_INLINE void record_cpu_no_sm(int idx, long long count, long long ticks)
        {
            long long dis = ticks / count;
            RecordNode& node = nodes_[idx];
            node.cpu.c += count;
            node.cpu.sum += ticks;
            node.cpu.sm = dis;
            node.cpu.t_u += ticks;
        }

        PROF_ALWAYS_INLINE void record_cpu_full(int idx, long long ticks)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += ticks;
            long long dis = ticks;
            long long avg = node.cpu.sum / node.cpu.c;

            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, ticks);
            node.cpu.h_sm = (dis >= avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
            node.cpu.l_sm = (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
            node.cpu.dv += abs(dis - node.cpu.sm);
            node.cpu.t_u += ticks;
            node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
        }

        PROF_ALWAYS_INLINE void record_cpu_full(int idx, long long c, long long ticks)
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


        PROF_ALWAYS_INLINE void record_timer(int idx, long long stamp)
        {
            RecordNode& node = nodes_[idx];
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
            RecordNode& node = nodes_[idx];
            node.mem.c += c;
            node.mem.sum += add;
            node.mem.t_u += add;
        }
        PROF_ALWAYS_INLINE void record_vm(int idx, const VMData& vm)
        {
            nodes_[idx].vm = vm;
        }
        PROF_ALWAYS_INLINE void record_user(int idx, long long c, long long add)
        {
            RecordNode& node = nodes_[idx];
            node.user.c += c;
            node.user.sum += add;
            node.user.t_u += add;
        }
        PROF_ALWAYS_INLINE void overwrite_user(int idx, long long c, long long add)
        {
            reset_user(idx);
            record_user(idx, c, add);
        }

        PROF_ALWAYS_INLINE void overwrite_mem(int idx, long long c, long long add)
        {
            reset_mem(idx);
            record_mem(idx, c, add);
        }


        PROF_ALWAYS_INLINE const RecordNode& at(int idx) const
        {
            return nodes_[idx];
        }

        PROF_ALWAYS_INLINE const RecordCPU& at_cpu(int idx) const
        {
            return nodes_[idx].cpu;
        }
        PROF_ALWAYS_INLINE const RecordTimer& at_timer(int idx) const
        {
            return nodes_[idx].timer;
        }
        PROF_ALWAYS_INLINE const RecordMem& at_mem(int idx) const
        {
            return nodes_[idx].mem;
        }
        PROF_ALWAYS_INLINE const RecordUser& at_user(int idx) const
        {
            return nodes_[idx].user;
        }
        PROF_ALWAYS_INLINE const RecordMerge& at_merge(int idx) const
        {
            return nodes_[idx].merge;
        }
        PROF_ALWAYS_INLINE const RecordShow& at_show(int idx) const
        {
            return nodes_[idx].show;
        }
        PROF_ALWAYS_INLINE const VMData& at_vmdata(int idx) const
        {
            return nodes_[idx].vm;
        }
        PROF_ALWAYS_INLINE const RecordTraits& at_traits(int idx) const
        {
            return nodes_[idx].traits;
        }
        PROF_ALWAYS_INLINE const char* at_name(int idx) const
        {
            return name(idx);
        }



        //递归展开  
        int recursive_output(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, Report& rp);
    

        //完整报告  
        int output_report(unsigned int flags = OUT_FLAG_ALL);
        int output_one_record(int entry_idx);
        int output_temp_record(const char* opt_name, size_t opt_name_len);
        int output_temp_record(const char* opt_name);


    public:
        Report& compact_writer() { return compact_writer_; }
        RecordNode& node(int idx) { return nodes_[idx]; }
    
        double particle_for_ns(int t) { return  particle_for_ns_[t]; }




     //output interface
    public:
        void set_output(ReportProc func) { output_ = func; }
    protected:
        void output_and_clean(Report& s) { s.closing_string(); output_(s); s.reset_offset(); }
        static void default_output(const Report& rp) { printf("%s\n", rp.buff()); }
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
        char compact_data_[compact_data_size()];
        Report compact_writer_;
        int unknown_desc_;
        int reserve_desc_;
        int no_name_space_;
        int no_name_space_len_;

    private:
        RecordNode nodes_[end_id()];
        int declare_window_;
        double particle_for_ns_[T_CLOCK_MAX];
    };

    template<int INST, int RESERVE, int DECLARE>
    Record<INST, RESERVE, DECLARE>::Record() : compact_writer_(compact_data_, compact_data_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        merge_leafs_size_ = 0;
        memset(particle_for_ns_, 0, sizeof(particle_for_ns_));
        declare_window_ = declare_begin_id();

        output_ = &Record::default_output;  //set default log;

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
    int Record<INST, RESERVE, DECLARE>::init(const char* title)
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
        zprof::Clock<> clk;
        clk.start();

        


        particle_for_ns_[T_CLOCK_NULL] = 0;
        particle_for_ns_[T_CLOCK_SYS] = get_inverse_frequency<T_CLOCK_SYS>();
        particle_for_ns_[T_CLOCK_CLOCK] = get_inverse_frequency<T_CLOCK_CLOCK>();
        particle_for_ns_[T_CLOCK_CHRONO] = get_inverse_frequency<T_CLOCK_CHRONO>();
        particle_for_ns_[T_CLOCK_STEADY_CHRONO] = get_inverse_frequency<T_CLOCK_STEADY_CHRONO>();
        particle_for_ns_[T_CLOCK_SYS_CHRONO] = get_inverse_frequency<T_CLOCK_SYS_CHRONO>();
        particle_for_ns_[T_CLOCK_SYS_MS] = get_inverse_frequency<T_CLOCK_SYS_MS>();
        particle_for_ns_[T_CLOCK_PURE_RDTSC] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();
        particle_for_ns_[T_CLOCK_VOLATILE_RDTSC] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();
        particle_for_ns_[T_CLOCK_FENCE_RDTSC] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();
        particle_for_ns_[T_CLOCK_MFENCE_RDTSC] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();
        particle_for_ns_[T_CLOCK_LOCK_RDTSC] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();
        particle_for_ns_[T_CLOCK_RDTSCP] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();
        particle_for_ns_[T_CLOCK_BTB_FENCE_RDTSC] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();
        particle_for_ns_[T_CLOCK_BTB_MFENCE_RDTSC] = get_inverse_frequency<T_CLOCK_PURE_RDTSC>();

        particle_for_ns_[T_CLOCK_NULL] = get_inverse_frequency<zprof::CLOCK_DEFAULT >();

        for (int i = begin_id(); i < reserve_end_id(); i++)
        {
            regist(i, "reserve", zprof::CLOCK_DEFAULT, false, false);
        }

        regist(INNER_NULL, "PROF_NULL", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_INIT_TS, "PROF_INIT_TS", T_CLOCK_SYS_MS, true, true);
        regist(INNER_RESET_TS, "PROF_RESET_TS", T_CLOCK_SYS_MS, true, true);
        regist(INNER_OUTPUT_TS, "PROF_OUTPUT_TS", T_CLOCK_SYS_MS, true, true);
        regist(INNER_INIT_COST, "PROF_INIT_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_MERGE_COST, "PROF_MERGE_COST", zprof::CLOCK_DEFAULT, true, true);

        regist(INNER_REPORT_COST, "PROF_REPORT_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_SERIALIZE_COST, "PROF_SERIALIZE_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_OUTPUT_COST, "PROF_OUTPUT_COST", zprof::CLOCK_DEFAULT, true, true);
    
        regist(INNER_MEM_INFO_COST, "PROF_MEM_INFO_COST", zprof::CLOCK_DEFAULT, true, true);

        regist(INNER_CLOCK_COST, "PROF_CLOCK_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_RECORD_COST, "PROF_RECORD_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_RECORD_SM_COST, "PROF_RECORD_SM_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_RECORD_FULL_COST, "PROF_RECORD_FULL_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_CLOCK_RECORD_COST, "PROF_CLOCK_RECORD_COST", zprof::CLOCK_DEFAULT, true, true);

        regist(INNER_ORIGIN_INC, "PROF_ORIGIN_INC", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_ATOM_RELEAX, "PROF_ATOM_RELEAX", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_ATOM_COST, "PROF_ATOM_COST", zprof::CLOCK_DEFAULT, true, true);
        regist(INNER_ATOM_SEQ_COST, "PROF_ATOM_SEQ_COST", zprof::CLOCK_DEFAULT, true, true);


        if (true)
        {
            overwrite_user(INNER_INIT_TS, 1, zprof::Clock<>::sys_now_ms());
            overwrite_user(INNER_RESET_TS, 1, zprof::Clock<>::sys_now_ms());
            overwrite_user(INNER_OUTPUT_TS, 1, zprof::Clock<>::sys_now_ms());
        }

        if (true)
        {
            zprof::Clock<> self_mem_clk;
            self_mem_clk.start();
            record_vm(INNER_MEM_INFO_COST, get_self_mem());
            record_cpu(INNER_MEM_INFO_COST, self_mem_clk.stop_and_save().cost());
            record_mem(INNER_MEM_INFO_COST, 1, sizeof(*this));
            record_user(INNER_MEM_INFO_COST, 1, max_count());
        }

        if (true)
        {
            zprof::Clock<> clk;
            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                zprof::Clock<> test_cost;
                test_cost.start();
                test_cost.stop_and_save();
                record_cpu(INNER_NULL, test_cost.cost());
            }
            record_cpu(INNER_CLOCK_RECORD_COST, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                clk.save();
            }
            record_cpu(INNER_CLOCK_COST, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                record_cpu_no_sm(INNER_NULL, clk.stop_and_save().cost());
            }
            record_cpu(INNER_RECORD_COST, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                record_cpu(INNER_NULL, 1, clk.stop_and_save().cost());
            }
            record_cpu(INNER_RECORD_SM_COST, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                record_cpu_full(INNER_NULL, 1, clk.stop_and_save().cost());
            }
            record_cpu(INNER_RECORD_FULL_COST, 1000, clk.stop_and_save().cost());


            std::atomic<long long> atomll_test(0);
            volatile long long origin_feetch_add_test = 0;
            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                origin_feetch_add_test++;
            }
            record_cpu(INNER_ORIGIN_INC, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_relaxed);
            }
            record_cpu(INNER_ATOM_RELEAX, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test++;
            }
            record_cpu(INNER_ATOM_COST, 1000, clk.stop_and_save().cost());

        
            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_seq_cst);
            }
            record_cpu(INNER_ATOM_SEQ_COST, 1000, clk.stop_and_save().cost());

            reset_node(INNER_NULL);
        }

        record_cpu(INNER_INIT_COST, clk.stop_and_save().cost());

        return 0;
    }


    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::build_jump_path()
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
    int Record<INST, RESERVE, DECLARE>::regist(int idx, const char* name, unsigned int clk, bool resident, bool re_reg)
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
        rename(idx, name);
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

    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::rename(int idx, const char* name)
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
    const char* Record<INST, RESERVE, DECLARE>::name(int idx)
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


    template<int INST, int RESERVE, int DECLARE>
    void Record<INST, RESERVE, DECLARE>::reset_childs(int idx, int depth)
    {
        if (idx < begin_id() || idx >= end_id())
        {
            return ;
        }
        RecordNode& node = nodes_[idx];
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
            RecordNode& child = nodes_[i];
            if (child.show.upper == idx)
            {
               reset_childs(i, depth + 1);
            }
        }
    }


    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::bind_childs(int idx, int cidx)
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
    int Record<INST, RESERVE, DECLARE>::bind_merge(int to, int child)
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
    void Record<INST, RESERVE, DECLARE>::do_merge()
    {
        Clock<> clk;
        clk.start();
        for (int i = 0; i < merge_leafs_size_; i++)
        {
            int leaf_id = merge_leafs_[i];
            RecordNode& leaf = nodes_[leaf_id];
            RecordNode* node = NULL;
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
        record_cpu(INNER_MERGE_COST, clk.stop_and_save().cost());
    }




    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::recursive_output(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, Report& rp)
    {
        if (entry_idx >= end_id())
        {
            return -1;
        }

        if (rp.buff_len() <= PROF_LINE_MIN_SIZE)
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
        if (node.traits.clk >= T_CLOCK_MAX)
        {
            return 0;
        }


    
        zprof::Clock<> single_line_cost;

        const char* name = &compact_data_[node.traits.name];
        size_t name_len = node.traits.name_len;
        double cpu_rate = particle_for_ns(node.traits.clk);
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

        rp.reset_offset();

    #define STRLEN(str) str, strlen(str)
        if (node.cpu.c > 0)
        {
            single_line_cost.start();
            rp.push_indent(depth * 2);
            rp.push_string(STRLEN("|"));
            rp.push_number((unsigned long long)entry_idx, 3);
            rp.push_string(STRLEN("| "));
            rp.push_string(name, name_len);
            rp.push_blank(name_blank);
            rp.push_string(STRLEN(" |"));

            rp.push_string(STRLEN("\tcpu*|-- "));
            if (true)
            {
                rp.push_human_count(node.cpu.c);
                rp.push_string(STRLEN("c, "));
                rp.push_human_time((long long)(node.cpu.sum * cpu_rate / node.cpu.c));
                rp.push_string(STRLEN(", "));
                rp.push_human_time((long long)(node.cpu.sum * cpu_rate));
            }

        
            if (node.cpu.min_u != LLONG_MAX && node.cpu.max_u > 0)
            {
                rp.push_string(STRLEN(" --|\tmax-min:|-- "));
                rp.push_human_time((long long)(node.cpu.max_u * cpu_rate));
                rp.push_string(STRLEN(", "));
                rp.push_human_time((long long)(node.cpu.min_u * cpu_rate));
            }

        
            if (node.cpu.dv > 0 || node.cpu.sm > 0)
            {
                rp.push_string(STRLEN(" --|\tdv-sm:|-- "));
                rp.push_human_time((long long)(node.cpu.dv * cpu_rate / node.cpu.c));
                rp.push_string(STRLEN(", "));
                rp.push_human_time((long long)(node.cpu.sm * cpu_rate));
            }

        
            if (node.cpu.h_sm > 0 || node.cpu.l_sm > 0)
            {
                rp.push_string(STRLEN(" --|\th-l:|-- "));
                rp.push_human_time((long long)(node.cpu.h_sm * cpu_rate));
                rp.push_string(STRLEN(", "));
                rp.push_human_time((long long)(node.cpu.l_sm * cpu_rate));
            }
            rp.push_string(STRLEN(" --|"));
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.cost());

            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.cost());

        }

        if (node.mem.c > 0)
        {
            single_line_cost.start();
            rp.push_indent(depth * 2);
            rp.push_string(STRLEN("|"));
            rp.push_number((unsigned long long)entry_idx, 3);
            rp.push_string(STRLEN("| "));
            rp.push_string(name, name_len);
            rp.push_blank(name_blank);
            rp.push_string(STRLEN(" |"));

            rp.push_string(STRLEN("\tmem*|-- "));
            if (true)
            {
                rp.push_human_count(node.mem.c);
                rp.push_string(STRLEN("c, "));
                rp.push_human_mem(node.mem.sum / node.mem.c);
                rp.push_string(STRLEN(", "));
                rp.push_human_mem(node.mem.sum);
            }

            rp.push_string(STRLEN(" --||-- "));
            if (node.mem.delta > 0)
            {
                rp.push_human_mem(node.mem.sum - node.mem.delta);
                rp.push_string(STRLEN(", "));
                rp.push_human_mem(node.mem.delta);
            }
            rp.push_string(STRLEN(" --|"));
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.cost());


            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.cost());
        }

        if (node.vm.rss_size + node.vm.vm_size > 0)
        {
            single_line_cost.start();
            rp.push_indent(depth * 2);
            rp.push_string(STRLEN("|"));
            rp.push_number((unsigned long long)entry_idx, 3);
            rp.push_string(STRLEN("| "));
            rp.push_string(name, name_len);
            rp.push_blank(name_blank);
            rp.push_string(STRLEN(" |"));


            rp.push_string(STRLEN("\t vm*|-- "));
            if (true)
            {
                rp.push_human_mem(node.vm.vm_size);
                rp.push_string(STRLEN("(vm), "));
                rp.push_human_mem(node.vm.rss_size);
                rp.push_string(STRLEN("(rss), "));
                rp.push_human_mem(node.vm.shr_size);
                rp.push_string(STRLEN("(shr), "));
                rp.push_human_mem(node.vm.rss_size - node.vm.shr_size);
                rp.push_string(STRLEN("(uss)"));
            }

            rp.push_string(STRLEN(" --|"));
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.cost());

            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.cost());
        }

        if (node.user.c > 0)
        {
            single_line_cost.start();
            rp.push_indent(depth * 2);
            rp.push_string(STRLEN("|"));
            rp.push_number((unsigned long long)entry_idx, 3);
            rp.push_string(STRLEN("| "));
            rp.push_string(name, name_len);
            rp.push_blank(name_blank);
            rp.push_string(STRLEN(" |"));


            rp.push_string(STRLEN("\tuser*|-- "));
            if (true)
            {
                rp.push_human_count(node.user.c);
                rp.push_string(STRLEN("c, "));
                rp.push_human_count(node.user.sum / node.user.c);
                rp.push_string(STRLEN(", "));
                rp.push_human_count(node.user.sum);
            }

            rp.push_string(STRLEN(" --|"));
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.cost());

            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.cost());
        }

        if (depth > PROF_MAX_DEPTH)
        {
            rp.push_indent(depth * 2);
            output_and_clean(rp);
            return -4;
        }

        for (int i = node.show.child; i < node.show.child + node.show.window; i++)
        {
            RecordNode& child = nodes_[i];
            if (child.show.upper == entry_idx)
            {
                int ret = recursive_output(i, depth + 1, NULL, 0, rp);
                if (ret < 0)
                {
                    return ret;
                }
            }
        }

        return 0;
    }






    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::output_one_record(int entry_idx)
    {
        StaticReport rp;
        int ret = recursive_output(entry_idx, 0, NULL, 0, rp);
        (void)ret;
        return ret;
    }

    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::output_temp_record(const char* opt_name, size_t opt_name_len)
    {
        StaticReport rp;
        int ret = recursive_output(0, 0, opt_name, opt_name_len, rp);
        reset_node(0);//reset  
        return ret;
    }

    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::output_temp_record(const char* opt_name)
    {
        return output_temp_record(opt_name, strlen(opt_name));
    }

    template<int INST, int RESERVE, int DECLARE>
    int Record<INST, RESERVE, DECLARE>::output_report(unsigned int flags)
    {
        if (output_ == nullptr)
        {
            return -1;
        }
        overwrite_user(INNER_OUTPUT_TS, 1, zprof::Clock<>::sys_now_ms());

        zprof::Clock<> clk;
        clk.start();
        StaticReport rp;

        rp.reset_offset();
        output_and_clean(rp);


        rp.push_char('=', 30);
        rp.push_char(' ');
        rp.push_string(title());
        rp.push_string(STRLEN(" output report at: "));
        rp.push_now_date();
        rp.push_string(STRLEN(" dist start time:["));
        rp.push_human_time((Clock<>::sys_now_ms() - nodes_[INNER_INIT_TS].user.sum)*1000*1000);
        rp.push_string(STRLEN("] dist reset time:["));
        rp.push_human_time((Clock<>::sys_now_ms() - nodes_[INNER_RESET_TS].user.sum) * 1000 * 1000);
        rp.push_char(']');
        rp.push_char(' ');

        rp.push_char('=', 30);
        output_and_clean(rp);

        rp.push_string(STRLEN("| -- index -- | ---    cpu  ------------ | ----------   hits, avg, sum   ---------- | ---- max, min ---- | ------ dv, sm ------ |  --- hsm, lsm --- | "));
        output_and_clean(rp);
        rp.push_string(STRLEN("| -- index -- | ---    mem  ---------- | ----------   hits, avg, sum   ---------- | ------ last, delta ------ | "));
        output_and_clean(rp);
        rp.push_string(STRLEN("| -- index -- | ---    vm  ------------ | ----------   vm, rss, shr, uss   ------------------ | " ));
        output_and_clean(rp);

        rp.push_string(STRLEN("| -- index -- | ---    user  ----------- | -----------  hits, avg, sum   ---------- | "));
        output_and_clean(rp);

        if (flags & OUT_FLAG_INNER)
        {
            rp.push_string(STRLEN(PROF_LINE_FEED));
            for (int i = INNER_NULL + 1; i < INNER_MAX; i++)
            {
                int ret = recursive_output(i, 0, NULL, 0, rp);
                (void)ret;
            }
        }

        if (flags & OUT_FLAG_RESERVE)
        {
            rp.push_string(STRLEN(PROF_LINE_FEED));
            for (int i = reserve_begin_id(); i < reserve_end_id(); i++)
            {
                int ret = recursive_output(i, 0, NULL, 0, rp);
                (void)ret;
            }
        }
    
        if (flags & OUT_FLAG_DELCARE)
        {
            rp.push_string(STRLEN(PROF_LINE_FEED));
            for (int i = declare_begin_id(); i < declare_window(); )
            {
                int ret = recursive_output(i, 0, NULL, 0, rp);
                (void)ret;
                i += nodes_[i].show.jumps + 1;
            }
        }

        rp.reset_offset();
        rp.push_char('=', 30);
        rp.push_char('\t');
        rp.push_string(" end : ");
        rp.push_now_date();
        rp.push_char('\t');
        rp.push_char('=', 30);
        output_and_clean(rp);
        output_and_clean(rp);

        record_cpu(INNER_REPORT_COST, clk.stop_and_save().cost());
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
#define ProfInstType zprof::Record<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT>
#define ProfInst ProfInstType::instance()


//包装函数 根据模版参数在编译阶段直接使用不同的入口  从而减少常见使用场景下的运行时判断消耗.  
template<bool IS_BAT, zprof::RecordLevel PROF_LEVEL>
inline void ProfRecordWrap(int idx, long long count, long long ticks)
{

}

template<>
inline void ProfRecordWrap<true, zprof::RECORD_LEVEL_NORMAL>(int idx, long long count, long long ticks)
{
    ProfInst.record_cpu(idx, count, ticks);
}

template<>
inline void ProfRecordWrap<false, zprof::RECORD_LEVEL_NORMAL>(int idx, long long count, long long ticks)
{
    (void)count;
    ProfInst.record_cpu(idx, ticks);
}
template<>
inline void ProfRecordWrap<true, zprof::RECORD_LEVEL_FAST>(int idx, long long count, long long ticks)
{
    ProfInst.record_cpu_no_sm(idx, count, ticks);
}
template<>
inline void ProfRecordWrap<false, zprof::RECORD_LEVEL_FAST>(int idx, long long count, long long ticks)
{
    (void)count;
    ProfInst.record_cpu_no_sm(idx, ticks);
}

template<>
inline void ProfRecordWrap<true, zprof::RECORD_LEVEL_FULL>(int idx, long long count, long long ticks)
{
    ProfInst.record_cpu_full(idx, count, ticks);
}
template<>
inline void ProfRecordWrap<false, zprof::RECORD_LEVEL_FULL>(int idx, long long count, long long ticks)
{
    (void)count;
    ProfInst.record_cpu_full(idx, ticks);
}

template<long long COUNT>
struct ProfCountIsGreatOne
{
    static const bool is_bat = COUNT > 1;
};


//RAII小函数  
//用于快速记录<注册条目>的性能信息  
template <long long COUNT = 1, zprof::RecordLevel PROF_LEVEL = zprof::RECORD_LEVEL_NORMAL,
    zprof::clock_type C = zprof::CLOCK_DEFAULT>
class ProfAutoRecord
{
public:
    //idx为<注册条目>的ID  
    ProfAutoRecord(int idx)
    {
        idx_ = idx;
        clock_.start();
    }
    ~ProfAutoRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>(idx_, COUNT, clock_.save().cost());
    }
    zprof::Clock<C>& clock() { return clock_; }
private:
    zprof::Clock<C> clock_;
    int idx_;
};



//RAII小函数  
//一次性记录并直接输出到日志 不需要提前注册任何条目  
//整体性能影响要稍微高于<注册条目>  但消耗部分并不影响记录本身. 使用在常见的一次性流程或者demo场景中.    
template <zprof::RecordLevel PROF_LEVEL = zprof::RECORD_LEVEL_NORMAL,
    zprof::clock_type C = zprof::CLOCK_DEFAULT>
class ProfAutoAnonRecord
{
public:
    ProfAutoAnonRecord(const char* desc, long long cnt = 1)
    {
        strncpy(desc_, desc, PROF_NAME_MAX_SIZE);
        desc_[PROF_NAME_MAX_SIZE - 1] = '\0'; 
        cnt_ = cnt;
        clock_.start();
    }
    ~ProfAutoAnonRecord()
    {
        //ProfCountIsGreatOne
        if (cnt_ == 1)
        {
            ProfRecordWrap<false, PROF_LEVEL>(ProfInstType::INNER_NULL, cnt_, clock_.save().cost());
        }
        else
        {
            ProfRecordWrap<true, PROF_LEVEL>(ProfInstType::INNER_NULL, cnt_, clock_.save().cost());
        }
        ProfInst.output_temp_record(desc_);
    }

    zprof::Clock<C>& clock() { return clock_; }
private:
    long long cnt_;
    zprof::Clock<C> clock_;
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
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.regist(id, name, zprof::CLOCK_DEFAULT,  false, false)

//快速注册条目: 同上, 名字也默认提供 即ID自身    
#define PROF_FAST_REGIST_NODE(id)  PROF_FAST_REGIST_NODE_ALIAS(id, #id)

//快速注册条目: 同上 但是为常驻条目 
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.regist(id, #id, zprof::CLOCK_DEFAULT,  true, false)  

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
#define PROF_RECORD_CPU_SAMPLE(idx, ticks) ProfInst.record_cpu(idx, ticks)   

//记录性能消耗信息(携带总耗时和执行次数) 平均耗时约为6ns      
//COUNT为常数 ticks为总耗时, 根据记录等级选择性存储 平滑数据, 抖动偏差 等     RecordLevel:RECORD_LEVEL_NORMAL  
#define PROF_RECORD_CPU_WRAP(idx, COUNT, ticks, PROF_LEVEL)  \
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>((int)(idx), (long long)(COUNT), (long long)ticks)  
//记录性能消耗信息: 同上, 但count非常数  
#define PROF_RECORD_CPU_DYN_WRAP(idx, count, ticks, PROF_LEVEL)  \
        ProfRecordWrap<true, PROF_LEVEL>((int)(idx), (long long)(count), (long long)ticks)

//同PROF_RECORD_CPU_SAMPLE  
#define PROF_RECORD_CPU(idx, ticks) PROF_RECORD_CPU_WRAP((idx), 1, (ticks), zprof::RECORD_LEVEL_NORMAL)

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
#define PROF_DEFINE_COUNTER(var)  zprof::Clock<> var

//定义一个带起始时间戳的计时器(通常场景很少用这个)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  zprof::Clock<> tc(start)  

//设置当前时间为 定时器开始时间    
#define PROF_START_COUNTER(var) var.start()   

//重新设置当前时间为 定时器开始时间    
#define PROF_RESTART_COUNTER(var) var.start()   

//设置当前时间为定时器结束时间  
#define PROF_STOP_AND_SAVE_COUNTER(var) var.stop_and_save()  

//设置当前时间为定时器结束时间 并写入idx对应的条目中  
#define PROF_STOP_AND_RECORD(idx, var) PROF_RECORD_CPU_WRAP((idx), 1, (var).stop_and_save().cost(), zprof::RECORD_LEVEL_NORMAL)



//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时候写入记录条目)-----------
#define PROF_DEFINE_AUTO_RECORD(var, idx) ProfAutoRecord<> var(idx)   


//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ANON_RECORD(var, desc) ProfAutoAnonRecord<> var(desc)
//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_MULTI_ANON_RECORD(var, count, desc) ProfAutoAnonRecord<> var(desc, count)
//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, level, ct, desc) ProfAutoAnonRecord<count, level, ct> var(desc, count)



//使用特殊条目<0>进行一次性输出  
//用于立刻输出性能信息而不是走报告输出  
#define PROF_OUTPUT_TEMP_RECORD(desc)        do {ProfInst.output_temp_record(desc, strlen(desc));}while(0)  

//立刻输出一条信息  
#define PROF_OUTPUT_RECORD(idx)        do {ProfInst.output_one_record(idx);}while(0)

//输出完整报告 (OUT_FLAG_ALL)   
#define PROF_OUTPUT_REPORT(...)    ProfInst.output_report(__VA_ARGS__)

//其他立即输出
#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  do {ProfRecordWrap<true, zprof::RECORD_LEVEL_FAST>((int)ProfInstType::INNER_NULL, (long long)(count), (long long)num);  PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) do {PROF_RECORD_USER(ProfInstType::INNER_NULL, count, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) do {PROF_RECORD_MEM(ProfInstType::INNER_NULL, count, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   do {PROF_RECORD_CPU(ProfInstType::INNER_NULL, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_USER(desc, num) do {PROF_RECORD_USER(ProfInstType::INNER_NULL, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) do {PROF_RECORD_MEM(ProfInstType::INNER_NULL, 1, num);PROF_OUTPUT_TEMP_RECORD(desc);} while(0)

//输出当前进程的vm/rss信息 
#define PROF_OUTPUT_SELF_MEM(desc) do{PROF_RECORD_VM(ProfInstType::INNER_NULL, zprof::get_self_mem()); PROF_OUTPUT_TEMP_RECORD(desc);}while(0)
#define PROF_OUTPUT_SYS_MEM(desc) do{PROF_RECORD_VM(ProfInstType::INNER_NULL, zprof::get_sys_mem()); PROF_OUTPUT_TEMP_RECORD(desc);}while(0)


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


#define PROF_RECORD_CPU_SAMPLE(idx, ticks) 
#define PROF_RECORD_CPU(idx, ticks) 
#define PROF_RECORD_CPU_WRAP(idx, COUNT, ticks, PROF_LEVEL) 
#define PROF_RECORD_CPU_DYN_WRAP(idx, count, ticks, PROF_LEVEL)
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


//临时兼容代码  
template<zprof::clock_type T = zprof::Clock<>::C>
using ProfCounter = zprof::Clock<T>;

using ProfSerializer = zprof::Report;



#endif
#ifdef __GNUG__
#pragma GCC pop_options
#endif
