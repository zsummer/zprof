
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

#ifndef ZPERF_H
#define ZPERF_H

#ifndef PERF_DEFAULT_INST_ID 
#define PERF_DEFAULT_INST_ID 0
#endif

#define PERF_RESERVE_TRACK_BEGIN 1

#ifndef PERF_DYN_TRACK_BEGIN
#define PERF_DYN_TRACK_BEGIN 200
#endif 

#ifndef PERF_DYN_TRACK_COUNT
#define PERF_DYN_TRACK_COUNT 200
#endif

#define PERF_MAX_TRACK_SIZE (PERF_DYN_TRACK_BEGIN+PERF_DYN_TRACK_COUNT)

#define PERF_MAX_TRACK_NAME_SIZE 128
#define PERF_MAX_TRACK_LINE_SIZE (PERF_MAX_TRACK_NAME_SIZE + 200)
#define PERF_MAX_TRACK_CHILD_COUNT 10
#define PERF_MAX_TRACK_CHILD_DEPTH 5


static_assert(PERF_RESERVE_TRACK_BEGIN > 0, "");
static_assert(PERF_RESERVE_TRACK_BEGIN < PERF_DYN_TRACK_BEGIN, "");
static_assert(PERF_DYN_TRACK_COUNT > 0, "");


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

enum PerfCPURecType
{
    PERF_CPU_NORMAL,
    PERF_CPU_FAST,
    PERF_CPU_FULL,
};

#define PERF_COUNTER_DEFAULT PERF_COUNTER_RDTSC
template<PerfCounterType T>
struct PerfCounterTypeClass
{
};



struct PerfDesc
{
    char track_name[PERF_MAX_TRACK_NAME_SIZE];
    int track_name_len;
    int counter_type;
};

struct PerfCPU
{
    long long c; 
    long long sum;  
    long long dv; 
    long long sm;
    long long h_sm;
    long long l_sm;
    long long max_u;
    long long min_u;
    long long t_c;
    long long t_u;
};

struct PerfTimer
{
    long long last;
};

struct PerfMEM
{
    long long c;  
    long long sum;
    long long delta;
    long long t_c;
    long long t_u;
};

struct PerfTrack
{
    bool active; 
    bool is_child;  
    std::array<unsigned int, PERF_MAX_TRACK_CHILD_COUNT> child_ids; 
    int child_count; 
    int merge_to;
    PerfDesc desc; 
    PerfCPU cpu; 
    PerfMEM mem; 
    PerfTimer timer;
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



template<int T, int S>
class PerfRecord 
{
public:
    static const int TRACK_COUNT = S;
    static const int PERF_RECORD_TYPE = T;
    static const int SERIALIZE_BUFF_LEN = PERF_MAX_TRACK_CHILD_DEPTH* PERF_MAX_TRACK_CHILD_COUNT* PERF_MAX_TRACK_LINE_SIZE * 2;
    static_assert(TRACK_COUNT <= PERF_MAX_TRACK_SIZE, "");
    PerfRecord() 
    {
        memset(tracks_, 0, sizeof(tracks_));
        desc_[0] = '\0';
        state_ = 0;
        merge_to_size_ = 0;
        memset(circles_per_ns_, 0, sizeof(circles_per_ns_));
        used_track_id_ = PERF_DYN_TRACK_BEGIN;
        serialize_buff_[0] = '\0';
    };
    static PerfRecord& instance()
    {
        static PerfRecord inst;
        return inst;
    }
    inline int init_perf(const char* desc);
    inline int regist_track(int idx, const char* desc, unsigned int counter, bool overwrite);
    inline int add_track_child(int idx, int child);
    inline int add_merge_to(int idx, int to);

    void reset_cpu(int idx)
    {
        PerfTrack& track = tracks_[idx];
        memset(&track.cpu, 0, sizeof(track.cpu));
    }
    void reset_mem(int idx)
    {
        PerfTrack& track = tracks_[idx];
        memset(&track.mem, 0, sizeof(track.mem));
    }
    inline void reset_childs(int idx, int depth = 0);

    void call_cpu(int idx, long long c, long long cost)
    {
        long long dis = cost / c;
        PerfTrack& track = tracks_[idx];
        track.cpu.c += c;
        track.cpu.sum += cost;
        track.cpu.sm = track.cpu.sm == 0 ? dis : track.cpu.sm;
        track.cpu.sm = (track.cpu.sm * 8 + dis * 2) / 10;
        track.cpu.max_u = track.cpu.max_u > dis ? track.cpu.max_u : dis;
        track.cpu.min_u = track.cpu.min_u < dis ? track.cpu.min_u : dis;
        track.cpu.dv += abs(dis - track.cpu.sm);
        track.cpu.t_c += c;
        track.cpu.t_u += cost;
    }
    void call_cpu(int idx, long long cost)
    {
        PerfTrack& track = tracks_[idx];
        track.cpu.c += 1;
        track.cpu.sum += cost;
        track.cpu.sm = track.cpu.sm == 0 ? cost : track.cpu.sm;
        track.cpu.sm = (track.cpu.sm * 12 + cost * 4) / 16;
        track.cpu.dv += abs(cost - track.cpu.sm);
        track.cpu.t_c += 1;
        track.cpu.t_u += cost;
    }
    void call_cpu_no_sm(int idx, long long cost)
    {
        PerfTrack& track = tracks_[idx];
        track.cpu.c += 1;
        track.cpu.sum += cost;
        track.cpu.sm = cost;
        track.cpu.t_c += 1;
        track.cpu.t_u += cost;
    }
    void call_cpu_no_sm(int idx, long long count, long long cost)
    {
        long long dis = cost / count;
        PerfTrack& track = tracks_[idx];
        track.cpu.c += count;
        track.cpu.sum += cost;
        track.cpu.sm = dis;
        track.cpu.t_c += count;
        track.cpu.t_u += cost;
    }
    void call_cpu_full(int idx, long long c, long long cost)
    {
        
        PerfTrack& track = tracks_[idx];
        track.cpu.c += c;
        track.cpu.sum += cost;
        long long dis = cost / c;
        long long h_sm = (track.cpu.h_sm * 12 + cost * 4) >> 4;
        long long l_sm = (track.cpu.l_sm * 12 + cost * 4) >> 4;
        long long avg = track.cpu.sum / track.cpu.c;
        


        track.cpu.sm = track.cpu.sm == 0 ? dis : track.cpu.sm;
        track.cpu.h_sm = track.cpu.h_sm == 0 ? dis : track.cpu.h_sm;
        track.cpu.l_sm = track.cpu.l_sm == 0 ? dis : track.cpu.l_sm;


        track.cpu.sm = (track.cpu.sm * 12 + cost * 4) >> 4;
        track.cpu.dv += abs(dis - track.cpu.sm);
        track.cpu.t_c += c;
        track.cpu.t_u += cost;


        track.cpu.max_u = track.cpu.max_u > dis ? track.cpu.max_u : dis;
        track.cpu.min_u = track.cpu.min_u < dis ? track.cpu.min_u : dis;

        track.cpu.h_sm = dis > avg ? h_sm : track.cpu.h_sm;
        track.cpu.l_sm = dis < avg ? l_sm : track.cpu.l_sm;




    }
    void call_timer(int idx, long long stamp)
    {
        PerfTrack& track = tracks_[idx];
        if (track.timer.last == 0)
        {
            track.timer.last = stamp;
            return;
        }
        call_cpu_full(idx, 1, stamp - track.timer.last);
        track.timer.last = stamp;
    }

    void call_mem(int idx, long long c, long long add)
    {
        PerfTrack& track = tracks_[idx];
        track.mem.c += c;
        track.mem.sum += add;
        track.mem.t_c += c;
        track.cpu.t_u += add;
    }
    void refresh_mem(int idx, long long c, long long add)
    {
        PerfTrack& track = tracks_[idx];
        track.mem.c = c;
        track.mem.delta = add - track.mem.sum;
        track.mem.sum = add;
        track.mem.t_c = c;
        track.cpu.t_u = add;
    }
    void merge_to(int idx, int to)
    {
        PerfTrack& track = tracks_[idx];
        if (track.cpu.t_c > 0)
        {
            call_cpu(to, track.cpu.t_u);
            track.cpu.t_c = 0;
            track.cpu.t_u = 0;
        }
        if (track.mem.t_c > 0)
        {
            call_mem(to, 1, track.mem.t_u);
            track.mem.t_c = 0;
            track.mem.t_u = 0;
        }
    }

    void update_merge()
    {
        for (int i = 0; i < merge_to_size_; i++)
        {
            PerfTrack& track = tracks_[merge_to_[i]];
            merge_to(merge_to_[i], track.merge_to);
        }
    }
    int serialize(int entry_idx, int depth, char* org_buff, int buff_len);
    const char* serialize(int entry_idx);


    PerfTrack& track(int idx) { return tracks_[idx]; }
    int track_count()const { return TRACK_COUNT; }
    const char* desc() const { return desc_; }
    char* mutable_desc() { return desc_; }
    const unsigned int state() const { return state_; }
    void set_state(unsigned int state) { state_ = state; }
    double circles_per_ns(int t) { return  circles_per_ns_[t == PERF_COUNTER_NULL ? PERF_COUNTER_DEFAULT : t]; }
    int new_dyn_track_id() 
    { 
        if (used_track_id_ >= TRACK_COUNT)
        {
            return 0;
        }
        return ++used_track_id_; 
    }
private:
    PerfTrack tracks_[S];
    char desc_[PERF_MAX_TRACK_NAME_SIZE];
    std::array<int, S> merge_to_;
    int merge_to_size_;
    unsigned int state_;
    double circles_per_ns_[PERF_COUNTER_MAX];
    int used_track_id_;
    char serialize_buff_[SERIALIZE_BUFF_LEN];
};



template<int T, int S>
int PerfRecord<T, S>::add_track_child(int idx, int cidx)
{
    if (idx < 0 || idx >= TRACK_COUNT || cidx < 0 || cidx >= TRACK_COUNT)
    {
        return -1;
    }

    if (idx == cidx)
    {
        return -2;  
    }

    PerfTrack& track = tracks_[idx];
    PerfTrack& child = tracks_[cidx];
    if (!track.active || !child.active)
    {
        return -3; //regist method has memset all info ; 
    }
    if (track.child_count >= PERF_MAX_TRACK_CHILD_COUNT)
    {
        return -4;
    }
    if (std::find_if(track.child_ids.begin(), track.child_ids.end(), [cidx](int id) {return id == cidx; }) != track.child_ids.end())
    {
        return -5; //duplicate 
    }

    track.child_ids[track.child_count++] = cidx;
    child.is_child = true;
    return 0;
}



template<int T, int S>
int PerfRecord<T, S>::add_merge_to(int idx, int to)
{
    if (idx < 0 || idx >= TRACK_COUNT || to < 0 || to >= TRACK_COUNT)
    {
        return -1;
    }

    if (idx == to)
    {
        return -2;  
    }
    if (merge_to_size_ >= TRACK_COUNT)
    {
        return -3;
    }
    PerfTrack& track = tracks_[idx];
    PerfTrack& to_track = tracks_[to];
    if (!track.active || !to_track.active)
    {
        return -3; //regist method has memset all info ; 
    }
    track.merge_to = to;
    merge_to_[merge_to_size_++] = idx;
    return 0;
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
template<int T, int S>
int PerfRecord<T, S>::init_perf(const char* desc)
{
    sprintf(desc_, "%s", desc);
    double chrono_rate = (double)std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count();
    chrono_rate /= 1000.0 * 1000.0 * 1000.0;
    chrono_rate = 1.0 / chrono_rate;
    circles_per_ns_[PERF_CONNTER_CHRONO] = chrono_rate;
    circles_per_ns_[PERF_COUNTER_NULL] = 0;
#ifdef WIN32
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
        return -1;
    }
    double tsc_rate = 1.0 / pppi[0].MaxMhz * 1000;
    
    circles_per_ns_[PERF_COUNTER_RDTSC] = tsc_rate;
    circles_per_ns_[PERF_COUNTER_RDTSC_NOFENCE] = tsc_rate;
    circles_per_ns_[PERF_COUNTER_CLOCK] = freq_rate;
    circles_per_ns_[PERF_COUNTER_SYS] = 1.0;

#elif (defined __APPLE__)
    double rdtsc_rate = perf_self_cpu_mhz();
    rdtsc_rate *= 1000 * 1000;
    rdtsc_rate = 1.0 / rdtsc_rate;
    rdtsc_rate *= 1000 * 1000 * 1000;
    circles_per_ns_[PERF_COUNTER_RDTSC] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_RDTSC_NOFENCE] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_CLOCK] = 1.0;
    circles_per_ns_[PERF_COUNTER_SYS] = 1.0;
#else
    //cpu_set_t set;
    //CPU_ZERO(&set);
    //CPU_SET(2, &set);
    //sched_setaffinity(0, sizeof(set), &set);
    //cat /proc/cpuinfo |grep constant_tsc  

    double rdtsc_rate = perf_self_cpu_mhz();
    rdtsc_rate *= 1000 * 1000;
    rdtsc_rate = 1.0 / rdtsc_rate;
    rdtsc_rate *= 1000 * 1000 * 1000;
    circles_per_ns_[PERF_COUNTER_RDTSC] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_RDTSC_NOFENCE] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_CLOCK] = 1.0;
    circles_per_ns_[PERF_COUNTER_SYS] = 1.0;
#endif


    return 0;

}





template<int T, int S>
int PerfRecord<T, S>::regist_track(int idx, const char* desc, unsigned int counter_type, bool overwrite)
{
    if (idx < 0)
    {
        return -1;
    }
    if (idx >= TRACK_COUNT)
    {
        return -2;
    }
    if (desc == NULL)
    {
        return -3;
    }
    PerfTrack& track = tracks_[idx];


    if (overwrite && track.active)
    {
        return 0;
    }

    int len = (int)strlen(desc);
    if (len + 1 > PERF_MAX_TRACK_NAME_SIZE)
    {
        return -4;
    }
    
    memset(&track, 0, sizeof(track));
    memcpy(track.desc.track_name, desc, len+1);
    track.desc.track_name_len = len;
    track.active = true;
    track.desc.counter_type = counter_type;
    track.cpu.min_u = LLONG_MAX;
    return 0;
}

template<int T, int S>
void PerfRecord<T, S>::reset_childs(int idx, int depth)
{
    if (idx < 0)
    {
        return;
    }
    if (idx >= TRACK_COUNT)
    {
        return;
    }
    PerfTrack& track = tracks_[idx];
    memset(&track.cpu, 0, sizeof(track.cpu));
    memset(&track.mem, 0, sizeof(track.mem));
    if (depth > 5)
    {
        return;
    }
    for (int i = 0; i < track.child_count; i++)
    {
        reset_childs(track.child_ids[i], depth + 1);
    }
}



template<int T, int S>
int PerfRecord<T, S>::serialize(int entry_idx, int depth, char* org_buff, int buff_len)
{
    if (entry_idx < 0 || entry_idx >= TRACK_COUNT)
    {
        return -1;
    }
    if (buff_len < PERF_MAX_TRACK_LINE_SIZE)
    {
        return -2;
    }
    char* buff = org_buff;

    PerfTrack& track = tracks_[entry_idx];
    if (track.cpu.c > 0)
    {
        if (buff_len < PERF_MAX_TRACK_LINE_SIZE)
        {
            return -6;
        }
        for (int i = 0; i < depth; i++)
        {
            *buff++ = ' ';
            *buff++ = ' ';
            buff_len -= 2;
        }
        *(buff + 1) = '\0';
        if (buff_len < PERF_MAX_TRACK_LINE_SIZE)
        {
            return -6;
        }
        char buff_call[50];
        human_count_format(buff_call, track.cpu.c);
        char buff_avg[50];
        human_time_format(buff_avg, (long long)( track.cpu.sum * circles_per_ns(track.desc.counter_type)) / track.cpu.c);
        char buff_sm[50];
        human_time_format(buff_sm, (long long)(track.cpu.sm * circles_per_ns(track.desc.counter_type)));
        char buff_dv[50];
        human_time_format(buff_dv, (long long)(track.cpu.dv * circles_per_ns(track.desc.counter_type) / track.cpu.c));
        char buff_sum[50];
        human_time_format(buff_sum, (long long)(track.cpu.sum * circles_per_ns(track.desc.counter_type)));
        char buff_hsm[50];
        human_time_format(buff_hsm, (long long)(track.cpu.h_sm * circles_per_ns(track.desc.counter_type)));
        char buff_lsm[50];
        human_time_format(buff_lsm, (long long)(track.cpu.l_sm * circles_per_ns(track.desc.counter_type)));
        char buff_max[50];
        human_time_format(buff_max, (long long)(track.cpu.max_u * circles_per_ns(track.desc.counter_type)));
        char buff_min[50];
        human_time_format(buff_min, (long long)(track.cpu.min_u * circles_per_ns(track.desc.counter_type)));

        int ret = sprintf(buff, "[[ %s ]] cpu: call:%s  avg: %s dv: %s sum: %s  sm: %s hsm: %s lsm: %s max: %s min: %s  \n",
            track.desc.track_name, buff_call, buff_avg, buff_dv, buff_sum, buff_sm, buff_hsm, buff_lsm, buff_max, buff_min);

        if (ret < 0)
        {
            return -4;
        }
        buff += ret;
        buff_len -= ret;
        if (buff_len < 0)
        {
            return -5;
        }
    }
    if (track.mem.c > 0)
    {
        if (buff_len < PERF_MAX_TRACK_LINE_SIZE)
        {
            return -6;
        }
        for (int i = 0; i < depth; i++)
        {
            *buff++ = ' ';
            *buff++ = ' ';
            buff_len -= 2;
        }
        *(buff + 1) = '\0';
        if (buff_len < PERF_MAX_TRACK_LINE_SIZE)
        {
            return -6;
        }
        char buff_call[50];
        human_count_format(buff_call, track.mem.c);
        char buff_avg[50];
        human_mem_format(buff_avg, track.mem.sum / track.mem.c);
        char buff_use[50];
        human_mem_format(buff_use, track.mem.sum);
        char buff_front[50];
        human_mem_format(buff_front, track.mem.sum - track.mem.delta);
        char buff_delta[50];
        human_mem_format(buff_delta, track.mem.delta);
        int ret = sprintf(buff, "[[ %s ]] mem: call:%s  avg: %s  sum: %s  front: %s delta: %s \n",
            track.desc.track_name, buff_call, buff_avg, buff_use, buff_front, buff_delta);

        if (ret < 0)
        {
            return -7;
        }
        buff += ret;
        buff_len -= ret;
        if (buff_len < 0)
        {
            return -8;
        }
    }
    if (track.cpu.c <= 0 && track.mem.c <= 0)
    {
        if (buff_len < PERF_MAX_TRACK_LINE_SIZE)
        {
            return -6;
        }
        for (int i = 0; i < depth; i++)
        {
            *buff++ = ' ';
            *buff++ = ' ';
            buff_len -= 2;
        }
        *(buff + 1) = '\0';
        if (buff_len < PERF_MAX_TRACK_LINE_SIZE)
        {
            return -6;
        }
        int ret = sprintf(buff, "[%s] no any call data. \n", track.desc.track_name);
        if (ret < 0)
        {
            return -7;
        }
        buff += ret;
        buff_len -= ret;
        if (buff_len < 0)
        {
            return -8;
        }
    }

    if (depth > 5)
    {
        return -9;
    }

    for (int i = 0; i < track.child_count; i++)
    {
        int ret = serialize(track.child_ids[i], depth + 1, buff, buff_len);
        if (ret < 0)
        {
            ret -= 10 * (1+depth);
            return ret;
        }
        buff += buff_len - ret;
        buff_len = ret;
    }
    return buff_len;
}






template<int T, int S>
const char* PerfRecord<T, S>::serialize(int entry_idx)
{
    char* buff = serialize_buff_;
    int remaind = SERIALIZE_BUFF_LEN;
    int ret = serialize(entry_idx, 0, buff, remaind);
    if (ret < 0)
    {
        sprintf(buff, "serialize idx:<%d> has error:<%d>\n", entry_idx, ret);
    }
    return buff;
}


#define PerfInst PerfRecord<PERF_DEFAULT_INST_ID, PERF_MAX_TRACK_SIZE>::instance()





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
    long long duration_ns() { return (long long)(cycles_ * PerfInst.circles_per_ns(T)); }
    double duration_second() { return (double)duration_ns() / (1000.0 * 1000.0 * 1000.0); }
    long long stop_val() { return start_val_ + cycles_; }
    long long start_val() { return start_val_; }

private:
    long long start_val_;
    long long cycles_;
};





template<bool IS_BAT, PerfCPURecType CPU_REC_TYPE>
struct PerfRecordTypeClass
{

};

template<bool IS_BAT, PerfCPURecType CPU_REC_TYPE>
inline void PerfRecordWrap(int idx, long long count, long long cost, PerfRecordTypeClass<IS_BAT, CPU_REC_TYPE>*)
{

}

template<>
inline void PerfRecordWrap(int idx, long long count, long long cost, PerfRecordTypeClass<true, PERF_CPU_NORMAL>*)
{
    PerfInst.call_cpu(idx, count, cost);
}

template<>
inline void PerfRecordWrap(int idx, long long count, long long cost, PerfRecordTypeClass<false, PERF_CPU_NORMAL>*)
{
    (void)count;
    PerfInst.call_cpu(idx, cost);
}
template<>
inline void PerfRecordWrap(int idx, long long count, long long cost, PerfRecordTypeClass<true, PERF_CPU_FAST>*)
{
    PerfInst.call_cpu_no_sm(idx, count, cost);
}
template<>
inline void PerfRecordWrap(int idx, long long count, long long cost, PerfRecordTypeClass<false, PERF_CPU_FAST>*)
{
    (void)count;
    PerfInst.call_cpu_no_sm(idx, cost);
}

template<>
inline void PerfRecordWrap(int idx, long long count, long long cost, PerfRecordTypeClass<true, PERF_CPU_FULL>*)
{
    PerfInst.call_cpu_full(idx, count, cost);
}
template<>
inline void PerfRecordWrap(int idx, long long count, long long cost, PerfRecordTypeClass<false, PERF_CPU_FULL>*)
{
    PerfInst.call_cpu_full(idx, count, cost);
}

template<long long COUNT>
struct PerfCountIsGreatOne
{
    static const bool is_bat = COUNT > 1;
};


template <bool COUNT = 1, PerfCPURecType CPU_REC_TYPE = PERF_CPU_NORMAL,
    PerfCounterType C = PERF_COUNTER_DEFAULT>
class PerfAutoRecord
{
public:
    PerfAutoRecord(int idx)
    {
        idx_ = idx;
        counter_.start();
    }
    ~PerfAutoRecord()
    {
        PerfRecordWrap(idx_, COUNT, counter_.save().cycles(), 
            (PerfRecordTypeClass <PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE> *)NULL);
    }
    PerfCounter<C>& counter() { return counter_; }
private:
    PerfCounter<C> counter_;
    int idx_;
};



template <PerfCounterType T = PERF_COUNTER_DEFAULT>
class PerfRegister
{
public:
    PerfRegister(const char* desc)
    {
        this_id_ = PerfInst.new_dyn_track_id();
        PerfInst.regist_track(this_id_, desc, T, false);
    }

    ~PerfRegister()
    {

    }

    void start()
    {
        counter_.start();
    }

    void restart()
    {
        counter_.start();
    }

    template <long long COUNT = 1LL, PerfCPURecType CPU_REC_TYPE = PERF_CPU_NORMAL>
    void record_current()
    {
        PerfRecordWrap(this_id_, COUNT, counter_.save().cycles(), 
            (PerfRecordTypeClass<PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE > *)NULL);
    }


    void record_mem(long long mem)
    {
        PerfInst.call_mem(this_id_, 1, mem);
    }
    void refresh_mem(long long mem)
    {
        PerfInst.refresh_mem(this_id_, 1, mem);
    }
    int track_id() { return this_id_; }
    PerfCounter<T>& counter() { return counter_; }

private:
    int this_id_;
    PerfCounter<T> counter_;
};

template <long long COUNT = 1LL, PerfCPURecType CPU_REC_TYPE = PERF_CPU_NORMAL,
    PerfCounterType C = PERF_COUNTER_DEFAULT>
class PerfAutoSingleRecord
{
public:
    PerfAutoSingleRecord(const char* desc):reg_(desc)
    {
        reg_.start();
    }
    ~PerfAutoSingleRecord()
    {
        PerfRecordWrap(reg_.track_id(), COUNT, reg_.counter().save().cycles(), (PerfRecordTypeClass <PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE>*)NULL);
    }

    PerfRegister<C>& reg() { return reg_; }
private:
    PerfRegister<C> reg_;
};





#ifdef OPEN_ZPERF

#define PERF_REGIST_TRACK(id, name, c, force)  PerfInst.regist_track(id, name, c, force)
#define PERF_FAST_REGIST_TRACK(id)  PerfInst.regist_track(id, #id, PERF_COUNTER_DEFAULT, false)
#define PERF_BIND_CHILD(id, cid)  PerfInst.add_track_child(id, cid)
#define PERF_BIND_MERGE(id, tid) PerfInst.add_merge_to(id, tid)

#define PERF_INIT(desc) PerfInst.init_perf(desc)
#define PERF_RESET_CHILD(idx) PerfInst.reset_childs(idx)
#define PERF_UPDATE_MERGE() PerfInst.update_merge()

#define PERF_CALL_CPU(idx, cost) PerfInst.call_cpu(idx, cost)
#define PERF_CALL_CPU_WRAP(idx, COUNT, cost, CPU_REC_TYPE)  \
            PerfRecordWrap<PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE>((int)(idx), (long long)(COUNT), (long long)cost, \
                    (PerfRecordTypeClass <PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE> *)NULL)
#define PERF_CALL_MEM(idx, count, mem) PerfInst.call_mem(idx, count, mem)
#define PERF_REFRESH_MEM(idx, count, mem) PerfInst.refresh_mem(idx, count, mem)
#define PERF_CALL_TIMER(idx, stamp) PerfInst.call_timer(idx, stamp)


#define PERF_DEFINE_COUNTER(c)  PerfCounter<> c
#define PERF_DEFINE_COUNTER_INIT(tc, start)  PerfCounter<> tc(start)
#define PERF_START_COUNTER(c) c.start()
#define PERF_RESTART_COUNTER(c) c.start()

#define PERF_DEFINE_AUTO_RECORD(c, idx) PerfAutoRecord<> c(idx)


#define PERF_DEFINE_REGISTER(reg, desc, counter) PerfRegister<counter> reg(desc);  
#define PERF_DEFINE_REGISTER_DEFAULT(reg, desc) PerfRegister<> reg(desc);  
#define PERF_REGISTER_START(reg) reg.start()
#define PERF_REGISTER_RECORD(reg) reg.record_current()
#define PERF_REGISTER_RECORD_WRAP(reg, COUNT, CPU_REC_TYPE) reg.record_current<COUNT, CPU_REC_TYPE>()
#define PERF_REGISTER_REC_MEM(reg, add) reg.record_mem(add)
#define PERF_REGISTER_REFRESH_MEM(reg, add) reg.refresh_mem(add)


#define PERF_DEFINE_AUTO_SINGLE_RECORD(rec, COUNT, CPU_REC_TYPE, desc) PerfAutoSingleRecord<COUNT, CPU_REC_TYPE, PERF_COUNTER_DEFAULT> rec(desc)
#define PERF_DEFINE_AUTO_RECORD_SELF_MEM(desc) do{ PerfRegister<> __temp_perf_record_mem__(desc); PERF_CALL_MEM(__temp_perf_record_mem__.track_id(), perf_self_memory_use()); }while(0)




#else
#define PERF_REGIST_TRACK(id, name, pt, force)
#define PERF_FAST_REGIST_TRACK(id) 
#define PERF_BIND_CHILD(id, cid) 
#define PERF_BIND_MERGE(id, tid) 

#define PERF_INIT(desc) 
#define PERF_RESET_CHILD(idx) 
#define PERF_UPDATE_MERGE() 

#define PERF_CALL_CPU(idx, cost) 
#define PERF_CALL_CPU_WRAP(idx, COUNT, cost, CPU_REC_TYPE) 
#define PERF_CALL_MEM(idx, count, mem) 
#define PERF_REFRESH_MEM(idx, count, mem) 
#define PERF_CALL_TIMER(idx, stamp) 


#define PERF_DEFINE_COUNTER(c)  
#define PERF_DEFINE_COUNTER_INIT(tc, start)  
#define PERF_START_COUNTER(c) 
#define PERF_RESTART_COUNTER(c) 

#define PERF_DEFINE_AUTO_RECORD(c, idx) 


#define PERF_DEFINE_REGISTER(reg, desc, counter) 
#define PERF_DEFINE_REGISTER_DEFAULT(reg, desc) 
#define PERF_REGISTER_START(reg) 
#define PERF_REGISTER_RECORD(reg) 
#define PERF_REGISTER_RECORD_WRAP(reg, COUNT, CPU_REC_TYPE) 
#define PERF_REGISTER_REC_MEM(reg, add) 
#define PERF_REGISTER_REFRESH_MEM(reg, add) 


#define PERF_DEFINE_AUTO_SINGLE_RECORD(rec, COUNT, CPU_REC_TYPE, desc) 
#define PERF_DEFINE_AUTO_RECORD_SELF_MEM(desc) 

#endif


#define PERF_SERIALIZE_FN_LOG()     LogDebug() \
<< "\n\n ------------------------------------------------------------------ " \
"\n ----------------------PerfRecord[" << PerfInst.desc() << "] begin ---------------------- \n"; \
for (int i = 0; i < PerfInst.track_count(); i++) \
{ \
    if (PerfInst.track(i).active && !PerfInst.track(i).is_child) \
    { \
        LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, FNLog::LOG_PREFIX_NULL) << PerfInst.serialize(i); \
    } \
} \
LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, FNLog::LOG_PREFIX_NULL) \
<< "\n ----------------------PerfRecord[" << PerfInst.desc() << "] end ----------------------" \
"\n ------------------------------------------------------------------\n\n";


#endif
