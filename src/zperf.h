
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
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "User32.lib")
#pragma comment(lib,"ws2_32.lib")
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

#define PERF_MAX_TRACK_SIZE 400
#define PERF_RESERVE_TRACK_BEGIN 1
#define PERF_USER_TRACK_BEGIN 100
#define PERF_DYN_TRACK_BEGIN 200


#define PERF_MAX_TRACK_NAME_SIZE 100
#define PERF_MAX_TRACK_LINE_SIZE (PERF_MAX_TRACK_NAME_SIZE + 200)
#define PERF_MAX_TRACK_CHILD_COUNT 10
#define PERF_MAX_TRACK_CHILD_DEPTH 5


static_assert(PERF_RESERVE_TRACK_BEGIN < PERF_USER_TRACK_BEGIN, "");
static_assert(PERF_USER_TRACK_BEGIN < PERF_DYN_TRACK_BEGIN, "");
static_assert(PERF_DYN_TRACK_BEGIN < PERF_MAX_TRACK_SIZE, "");

enum PerfCycleCounter
{
    PERF_CYCLE_COUNTER_DEFAULT,
    PERF_CYCLE_COUNTER_SYS,
    PERF_CYCLE_COUNTER_CLOCK,
    PERF_CYCLE_COUNTER_RDTSC,
    PERF_CYCLE_COUNTER_MAX,
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
    long long use;  
    long long dv; 
    long long sm;
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
    long long use;
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



static const int perf_serialize_len = PERF_MAX_TRACK_CHILD_DEPTH * PERF_MAX_TRACK_CHILD_COUNT * PERF_MAX_TRACK_LINE_SIZE * 2;
static inline char* perf_serialize_buff()
{
    static char buff[perf_serialize_len];
    return buff;
}
static inline int& perf_static_dyn_id()
{
    static_assert(PERF_DYN_TRACK_BEGIN > 0, "0 is invalid begin.");
    static int dyn_id = PERF_DYN_TRACK_BEGIN;
    return dyn_id;
}

static inline double* perf_inverse_hz_table()
{
    static double table[PERF_CYCLE_COUNTER_MAX];
    return table;
}

static inline double perf_inverse_hz(unsigned int t)
{
    if (t >= PERF_CYCLE_COUNTER_MAX)
    {
        return 1.0;
    }
    return perf_inverse_hz_table()[t];
}
static inline void perf_set_inverse_hz(unsigned int t, double hz)
{
    if (t >= PERF_CYCLE_COUNTER_MAX)
    {
        return ;
    }
    perf_inverse_hz_table()[t] = hz;
}




template<int T, int S>
class PerfRecord 
{
public:
    static const int TRACK_COUNT = S;
    static const int PERF_RECORD_TYPE = T;
    PerfRecord() 
    {
        memset(tracks_, 0, sizeof(tracks_));
        desc_[0] = '\0';
        state_ = 0;
        merge_to_size_ = 0;
    };
    static PerfRecord& instance()
    {
        static PerfRecord inst;
        return inst;
    }
    inline int init_perf();
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
    void call_cpu(int idx, long long c, long long use)
    {
        long long dis = use / c;
        PerfTrack& track = tracks_[idx];
        track.cpu.c += c;
        track.cpu.use += use;
        track.cpu.sm = track.cpu.sm == 0 ? dis : track.cpu.sm;
        track.cpu.sm = (track.cpu.sm * 8 + dis * 2) / 10;
        track.cpu.dv += abs(dis - track.cpu.sm);
        track.cpu.t_c += c;
        track.cpu.t_u += use;
    }
    void call_cpu(int idx, long long use)
    {
        PerfTrack& track = tracks_[idx];
        track.cpu.c += 1;
        track.cpu.use += use;
        track.cpu.sm = track.cpu.sm == 0 ? use : track.cpu.sm;
        track.cpu.sm = (track.cpu.sm * 8 + use * 2) / 10;
        track.cpu.dv += abs(use - track.cpu.sm);
        track.cpu.t_c += 1;
        track.cpu.t_u += use;
    }
    void call_timer(int idx, long long stamp)
    {
        PerfTrack& track = tracks_[idx];
        if (track.timer.last == 0)
        {
            track.timer.last = stamp;
            return;
        }
        call_cpu(idx, stamp - track.timer.last);
        track.timer.last = stamp;
    }

    void call_mem(int idx, long long c, long long use)
    {
        PerfTrack& track = tracks_[idx];
        track.mem.c += c;
        track.mem.use += use;
        track.mem.t_c += c;
        track.cpu.t_u += use;
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
private:
    PerfTrack tracks_[S];
    char desc_[PERF_MAX_TRACK_NAME_SIZE];
    std::array<int, S> merge_to_;
    int merge_to_size_;
    unsigned int state_;
};



//#define PERF_RDTSCP
#define PERF_RDTSC_INTEL






inline long long perf_tsc_rdtsc()
{
#ifdef WIN32
    long long count = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&count);
    return count;
    //return (long long)__rdtsc();
#elif (defined PERF_RDTSCP)
    unsigned long hi, lo;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi));
    uint64_t val = (((uint64_t)hi) << 32 | ((uint64_t)lo));
    return (long long)val;
#elif (defined PERF_RDTSC_INTEL)
    unsigned int lo, hi;
    __asm__ __volatile__("lfence;rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
    uint64_t val = ((uint64_t)hi << 32) | lo;
    return (long long)val;

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



inline int perf_self_memory_use()
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
    const char* file = "/proc/cpuinfo";
    FILE* fp = fopen(file, "r");
    if (NULL == fp)
    {
        return 0;
    }

    char line_buff[256];
    double mhz = 1;
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
    return mhz;
}


template<PerfCycleCounter T>
struct PerfCycleCounterClass
{
};

template<PerfCycleCounter T>
inline long long perf_tsc(const PerfCycleCounterClass<T>* ptr)
{
    (void)ptr;
    return perf_tsc_clock();
}

template<>
inline long long perf_tsc(const PerfCycleCounterClass<PERF_CYCLE_COUNTER_DEFAULT>* ptr)
{
    (void)ptr;
    return perf_tsc_rdtsc();
}


template<>
inline long long perf_tsc(const PerfCycleCounterClass<PERF_CYCLE_COUNTER_RDTSC>* ptr)
{
    (void)ptr;
    return perf_tsc_rdtsc();
}

template<>
inline long long perf_tsc(const PerfCycleCounterClass<PERF_CYCLE_COUNTER_CLOCK>* ptr)
{
    (void)ptr;
    return perf_tsc_clock();
}

template<>
inline long long perf_tsc(const PerfCycleCounterClass<PERF_CYCLE_COUNTER_SYS>* ptr)
{
    (void)ptr;
    return perf_tsc_sys();
}

template<PerfCycleCounter T = PERF_CYCLE_COUNTER_DEFAULT>
class PerfCounter
{
public:
    PerfCounter()
    {
        start();
    }
    PerfCounter(long long init_val)
    {
        start_val_ = init_val;
        cycles_ = 0;
    }

    void start()
    {
        start_val_ = perf_tsc<T>((PerfCycleCounterClass<T>*)NULL);
        cycles_ = 0;
    }

    PerfCounter& save()
    {
        long long elapse = perf_tsc<T>((PerfCycleCounterClass<T>*)NULL) - start_val_;
        cycles_ = elapse > 0 ? elapse : 0;
        return *this;
    }
    PerfCounter& stop_and_save() { return save(); }

    long long cycles() { return cycles_; }
    long long duration_ns(){ return (long long)(cycles_ * perf_inverse_hz(T));}
    double duration_second() { return (double)duration_ns() / (1000.0 * 1000.0 * 1000.0); }
    long long stop_val() { return start_val_ + cycles_; }
    long long start_val() { return start_val_; }

private:
    long long start_val_;
    long long cycles_;
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

template<int T, int S>
int PerfRecord<T, S>::init_perf()
{
#ifdef WIN32
    double rate = 0;
    long long win_freq = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&win_freq);
    rate = 1.0 / win_freq;
    rate *= 1000.0 * 1000 * 1000;
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_DEFAULT, rate);
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_RDTSC, rate);
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_CLOCK, rate);
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_SYS, 1.0);
#else
    //cpu_set_t set;
    //CPU_ZERO(&set);
    //CPU_SET(2, &set);
    //sched_setaffinity(0, sizeof(set), &set);
    //cat /proc/cpuinfo |grep constant_tsc  

    double mhz = perf_self_cpu_mhz();
    mhz *= 1000 * 1000;
    mhz = 1.0 / mhz;
    mhz *= 1000 * 1000 * 1000;
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_DEFAULT, mhz);
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_RDTSC, mhz);
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_CLOCK, 1.0);
    perf_set_inverse_hz(PERF_CYCLE_COUNTER_SYS, 1.0);

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



static inline const char* human_count_format(char *buff, long long count)
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
    if (ns > 1000*1000*1000)
    {
        sprintf(buff, "%.4lfs", ns / 1000.0 / 1000.0 / 1000.0);
        return buff;
    }
    else if (ns > 1000*1000)
    {
        sprintf(buff, "%.4lfms", ns / 1000.0 / 1000.0);
        return buff;
    }
    else if (ns > 1000)
    {
        sprintf(buff, "%.4lfus", ns / 1000.0 );
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
        human_time_format(buff_avg, (long long)(track.cpu.use * perf_inverse_hz(track.desc.counter_type)) / track.cpu.c);
        char buff_sm[50];
        human_time_format(buff_sm, (long long)(track.cpu.sm * perf_inverse_hz(track.desc.counter_type)));
        char buff_dv[50];
        human_time_format(buff_dv, (long long)(track.cpu.dv * perf_inverse_hz(track.desc.counter_type) / track.cpu.c));
        char buff_sum[50];
        human_time_format(buff_sum, (long long)(track.cpu.use * perf_inverse_hz(track.desc.counter_type)));


        int ret = sprintf(buff, "[[ %s ]] cpu: call:%s  avg: %s sm: %s dv:%s sum: %s  \n",
            track.desc.track_name, buff_call, buff_avg, buff_sm, buff_dv, buff_sum);

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
        human_mem_format(buff_avg, track.mem.use / track.mem.c);
        char buff_use[50];
        human_mem_format(buff_use, track.mem.use);

        int ret = sprintf(buff, "[[ %s ]] mem: call:%s  avg: %s  use: %s \n",
            track.desc.track_name, buff_call, buff_avg, buff_use);

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
    char* buff = perf_serialize_buff();
    int remaind = perf_serialize_len;
    int ret = serialize(entry_idx, 0, buff + (perf_serialize_len - remaind), remaind);
    if (ret < 0)
    {
        sprintf(buff, "serialize idx:<%d> has error:<%d>\n", entry_idx, ret);
    }
    return buff;
}


#define PerfInst PerfRecord<0, PERF_MAX_TRACK_SIZE>::instance()
#define REGIST_TRACK(id, name, pt, force)  PerfInst.regist_track(id, name, pt, force)
#define REGIST_TRACK_AUTO(id)  PerfInst.regist_track(id, #id, PERF_CYCLE_COUNTER_DEFAULT, false)
#define BIND_CHILD(id, cid)  PerfInst.add_track_child(id, cid)
#define BIND_MERGE(id, tid) PerfInst.add_merge_to(id, tid)




template <PerfCycleCounter T = PERF_CYCLE_COUNTER_DEFAULT>
class PerfCounterGuard
{
public:
    PerfCounterGuard(int idx)
    {
        idx_ = idx;
    }
    ~PerfCounterGuard()
    {
        if (idx_ >= 0 && idx_ < PERF_MAX_TRACK_SIZE)
        {
            PerfInst.call_cpu(idx_, 1, counter_.save().cycles());
        }
    }

private:
    PerfCounter<T> counter_;
    int idx_;
};


template <PerfCycleCounter T = PERF_CYCLE_COUNTER_DEFAULT>
class PerfOTCycleCounter
{
public:
    PerfOTCycleCounter(const char* desc) : counter_((long long)0)
    {
        this_id_ = 0;
        int& dyn_id = perf_static_dyn_id();
        if (dyn_id + 1 < PERF_MAX_TRACK_SIZE)
        {
            this_id_ = dyn_id++;
            PerfInst.regist_track(this_id_, desc, T, false);
        }
    }

    void start()
    {
        counter_.start();
    }

    void save()
    {
        PerfInst.call_cpu(this_id_, counter_.save().cycles());
    }


    void save(long long count)
    {
        PerfInst.call_cpu(this_id_, count, counter_.save().cycles());
    }


    void call_mem(long long mem)
    {
        PerfInst.call_mem(this_id_, 1, mem);
    }

    int track_id() { return this_id_; }
    PerfCounter<T>& counter() { return counter_; }

    const char* show()
    {
        return PerfInst.serialize(this_id_);
    }

    ~PerfOTCycleCounter()
    {

    }

private:
    int this_id_;
    PerfCounter<T> counter_;
};

template <PerfCycleCounter T = PERF_CYCLE_COUNTER_DEFAULT>
class PerfDynTimeGuard
{
public:
    PerfDynTimeGuard(const char* desc) :dyn_(desc), count_(1)
    {
        dyn_.start();
    }
    PerfDynTimeGuard(const char* desc, long long count):dyn_(desc), count_(count)
    {
        dyn_.start();
    }
    ~PerfDynTimeGuard()
    {
        dyn_.save(count_);
    }
    PerfOTCycleCounter<T>& dyn_line() { return dyn_; }
private:
    PerfOTCycleCounter<T> dyn_;
    long long count_;
};


#define OPEN_ZPERF
#ifdef OPEN_ZPERF
#define PERF_INIT() PerfInst.init_perf()
#define PERF_RESET_CHILD(idx) PerfInst.reset_childs(idx)
#define PERF_UPDATE_MERGE() PerfInst.update_merge()

#define PERF_CALL_CPU_G_REALTIME_WITH_C(idx, count, counter) PerfInst.call_cpu(idx, count, counter.save().cycles())
#define PERF_CALL_CPU_G_WITH_C(idx, count, counter) PerfInst.call_cpu(idx, count, counter.cycles())
#define PERF_CALL_CPU_G_REALTIME(idx, counter) PerfInst.call_cpu(idx, counter.save().cycles())
#define PERF_CALL_CPU_G(idx, counter) PerfInst.call_cpu(idx, counter.cycles())
#define PERF_CALL_CPU(idx, use) PerfInst.call_cpu(idx, use)
#define PERF_CALL_CPU_WITH_C(idx, c, use) PerfInst.call_cpu(idx, c, use)


#define PERF_CALL_MEM_WITH_C(idx, count, mem) PerfInst.call_mem(idx, count, mem)
#define PERF_CALL_MEM(idx, mem) PerfInst.call_mem(idx, 1, mem)

#define PERF_CALL_TIMER(idx, stamp) PerfInst.call_timer(idx, stamp)


#define PERF_DEFINE_COUNTER(tc)  PerfCounter<> tc
#define PERF_DEFINE_COUNTER_REF(tc, ref)  PerfCounter<> tc(ref.begin())
#define PERF_DEFINE_COUNTER_EMPTY(tc)  PerfCounter<> tc(0)
#define PERF_BEGIN_COUNTER(pf) pf.start()
#define PERF_END_COUNTER(pf) pf.save()

#define PERF_DEFINE_OT_COUNTER(otc, desc) PerfOTCycleCounter<> otc(desc);  
#define PERF_DEFINE_OT_COUNTER_GUARD(otc, desc) PerfDynTimeGuard<> otc(desc);  
#define PERF_DEFINE_OT_COUNTER_GUARD_WITH_C(otc, desc, c) PerfDynTimeGuard<> otc(desc, c);  
#define PERF_BEGIN_OT_COUNTER(otc) otc.start()
#define PERF_END_OT_COUNTER(otc) otc.save()

#define PERF_OT_COUNTER_CALL_MEM(otc, use) otc.call_mem(use)

#define PERF_DEFINE_COUNTER_GUARD(tc, idx) PerfCounterGuard<> tc(idx)

#else
#define PERF_INIT() 
#define PERF_RESET_CHILD(idx) 
#define PERF_UPDATE_MERGE()

#define PERF_CALL_CPU_G_REALTIME_WITH_C(idx, count, counter) 
#define PERF_CALL_CPU_G_WITH_C(idx, count, counter) 
#define PERF_CALL_MEM_WITH_C(idx, count, mem) 
#define PERF_CALL_CPU_G_REALTIME(idx, counter) 
#define PERF_CALL_CPU_G(idx, counter) 
#define PERF_CALL_CPU(idx, use) 
#define PERF_CALL_CPU_WITH_C(idx, c, use) 

#define PERF_CALL_MEM(idx, mem) 
#define PERF_CALL_TIMER(idx, stamp)

#define PERF_DEFINE_COUNTER(tc)  
#define PERF_DEFINE_COUNTER_REF(tc, ref)  
#define PERF_DEFINE_COUNTER_EMPTY(tc) 
#define PERF_BEGIN_COUNTER(pf) 
#define PERF_END_COUNTER(pf) 
#define PERF_DEFINE_OT_COUNTER(otc, desc)
#define PERF_DEFINE_OT_COUNTER_GUARD(otc, desc)
#define PERF_DEFINE_OT_COUNTER_GUARD_WITH_C(otc, desc, c)
#define PERF_BEGIN_OT_COUNTER(otc) 
#define PERF_END_OT_COUNTER(otc) 
#define PERF_OT_COUNTER_CALL_MEM(otc, use)

#define PERF_DEFINE_COUNTER_GUARD(tc, idx)
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
