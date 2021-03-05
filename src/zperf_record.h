
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

#include "zperf_counter.h"
#include <algorithm>
#ifndef ZPERF_RECORD_H
#define ZPERF_RECORD_H


#define PERF_DECLARE_TRACK_BEGIN 1

#define PERF_MAX_TRACK_NAME_SIZE 128
#define PERF_MAX_TRACK_LINE_SIZE (PERF_MAX_TRACK_NAME_SIZE + 200)
#define PERF_MAX_TRACK_CHILD_COUNT 10
#define PERF_MAX_TRACK_CHILD_DEPTH 5


static_assert(PERF_DECLARE_TRACK_BEGIN > 0, "");


enum PerfCPURecType
{
    PERF_CPU_NORMAL,
    PERF_CPU_FAST,
    PERF_CPU_FULL,
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
    long long merge_temp;
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
    long long merge_temp;
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


template<int T, int S, int D>
class PerfRecord 
{
public:
    static const int DECLARE_TRACK_COUNT = S;
    static const int TRACK_DYN_COUNT = D;
    static const int TRACK_MAX_COUNT = PERF_DECLARE_TRACK_BEGIN + DECLARE_TRACK_COUNT + TRACK_DYN_COUNT;
    static const int PERF_RECORD_TYPE = T;
    static const int SERIALIZE_BUFF_LEN = PERF_MAX_TRACK_CHILD_DEPTH* PERF_MAX_TRACK_CHILD_COUNT* PERF_MAX_TRACK_LINE_SIZE * 2;
    static constexpr int track_count() { return TRACK_MAX_COUNT; }
    static constexpr int track_max_count() { return TRACK_MAX_COUNT; }
    static constexpr int track_declare_begin() { return PERF_DECLARE_TRACK_BEGIN; }
    static constexpr int track_declare_count() { return DECLARE_TRACK_COUNT; }
    static constexpr int track_declare_end() { return track_declare_begin() + track_declare_count(); }
    static constexpr int track_dyn_begin() { return track_declare_end(); }
    static constexpr int track_dyn_count() { return TRACK_DYN_COUNT; }
    static constexpr int track_dyn_end() { return track_dyn_begin() + track_dyn_count(); }

    static_assert(track_max_count() == track_dyn_end(), "");

public:
    long long init_timestamp_;
    long long last_timestamp_;
public:

    PerfRecord() 
    {
        memset(tracks_, 0, sizeof(tracks_));
        desc_[0] = '\0';
        merge_to_size_ = 0;
        memset(circles_per_ns_, 0, sizeof(circles_per_ns_));
        used_track_id_ = track_dyn_begin();
        serialize_buff_[0] = '\0';
        init_timestamp_ = 0;
        last_timestamp_ = 0;
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
    void reset_timer(int idx)
    {
        PerfTrack& track = tracks_[idx];
        memset(&track.timer, 0, sizeof(track.timer));
    }
    void reset_declare_info()
    {
        for (int i = track_declare_begin(); i < track_declare_end(); i++)
        {
            reset_cpu(i);
            reset_mem(i);
            reset_timer(i);
        }
        last_timestamp_ = time(NULL);
    }
    inline void reset_childs(int idx, int depth = 0);

    void call_cpu(int idx, long long c, long long cost)
    {
        long long dis = cost / c;
        PerfTrack& track = tracks_[idx];
        track.cpu.c += c;
        track.cpu.sum += cost;
        track.cpu.sm = track.cpu.sm == 0 ? dis : track.cpu.sm;
        track.cpu.sm = (track.cpu.sm * 12 + cost * 4) >> 4;
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
        track.cpu.sm = (track.cpu.sm * 12 + cost * 4) >> 4;
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

    void call_cpu_full(int idx, long long cost)
    {

        PerfTrack& track = tracks_[idx];
        track.cpu.c += 1;
        track.cpu.sum += cost;
        long long dis = cost;
        long long avg = track.cpu.sum / track.cpu.c;
        if (track.cpu.sm == 0)
        {
            track.cpu.sm = track.cpu.sm == 0 ? dis : track.cpu.sm;
            track.cpu.l_sm = track.cpu.l_sm == 0 ? dis : track.cpu.l_sm;
            track.cpu.h_sm = track.cpu.h_sm == 0 ? dis : track.cpu.h_sm;
        }
        long long& hlsm = dis > avg ? track.cpu.h_sm : track.cpu.l_sm;
        hlsm = (hlsm * 12 + dis * 4) >> 4;
        track.cpu.sm = (track.cpu.sm * 12 + cost * 4) >> 4;
        track.cpu.dv += abs(dis - track.cpu.sm);
        track.cpu.t_c += 1;
        track.cpu.t_u += cost;
        track.cpu.max_u = track.cpu.max_u > dis ? track.cpu.max_u : dis;
        track.cpu.min_u = track.cpu.min_u < dis ? track.cpu.min_u : dis;
    }

    void call_cpu_full(int idx, long long c, long long cost)
    {
        
        PerfTrack& track = tracks_[idx];
        track.cpu.c += c;
        track.cpu.sum += cost;
        long long dis = cost / c;
        long long avg = track.cpu.sum / track.cpu.c;
        if (track.cpu.sm == 0)
        {
            track.cpu.sm = track.cpu.sm == 0 ? dis : track.cpu.sm;
            track.cpu.l_sm = track.cpu.l_sm == 0 ? dis : track.cpu.l_sm;
            track.cpu.h_sm = track.cpu.h_sm == 0 ? dis : track.cpu.h_sm;
        }
        long long& hlsm = dis > avg ? track.cpu.h_sm : track.cpu.l_sm;
        hlsm = (hlsm * 12 + dis * 4) >> 4;
        track.cpu.sm = (track.cpu.sm * 12 + cost * 4) >> 4;
        track.cpu.dv += abs(dis - track.cpu.sm);
        track.cpu.t_c += c;
        track.cpu.t_u += cost;
        track.cpu.max_u = track.cpu.max_u > dis ? track.cpu.max_u : dis;
        track.cpu.min_u = track.cpu.min_u < dis ? track.cpu.min_u : dis;
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
        track.mem.t_u += add;
    }
    void refresh_mem(int idx, long long c, long long add)
    {
        PerfTrack& track = tracks_[idx];
        track.mem.c = c;
        track.mem.delta = add - track.mem.sum;
        track.mem.sum = add;
        track.mem.t_c = c;
        track.mem.t_u = add;
    }


    void merge_cpu_temp(long long t_u, int to)
    {
        PerfTrack& to_track = tracks_[to];
        to_track.cpu.merge_temp += t_u;
        if (to_track.merge_to > 0)
        {
            merge_cpu_temp(t_u, to_track.merge_to);
        }
    }
    void merge_mem_temp(long long t_u, int to)
    {
        PerfTrack& to_track = tracks_[to];
        to_track.mem.merge_temp += t_u;
        if (to_track.merge_to > 0)
        {
            merge_mem_temp(t_u, to_track.merge_to);
        }
    }
    void merge_proc(int idx, int to)
    {
        PerfTrack& track = tracks_[idx];
        if (track.cpu.t_c > 0)
        {
            merge_cpu_temp(track.cpu.t_u, to);
            track.cpu.t_c = 0;
            track.cpu.t_u = 0;
        }
        if (track.mem.t_c > 0)
        {
            merge_mem_temp(track.mem.t_u, to);
            track.mem.t_c = 0;
            track.mem.t_u = 0;
        }
    }

    void merge_to(int idx, int to)
    {
        PerfTrack& to_track = tracks_[to];
        if (to_track.cpu.merge_temp > 0)
        {
            call_cpu_full(to, to_track.cpu.merge_temp);
            to_track.cpu.merge_temp = 0;
            to_track.cpu.t_c = 0;
            to_track.cpu.t_u = 0;
        }
        if (to_track.mem.merge_temp > 0)
        {
            call_mem(to, 1, to_track.mem.merge_temp);
            to_track.mem.merge_temp = 0;
            to_track.mem.t_c = 0;
            to_track.mem.t_u = 0;
        }
    }

    void update_merge()
    {
        for (int i = 0; i < merge_to_size_; i++)
        {
            PerfTrack& track = tracks_[merge_to_[i]];
            merge_proc(merge_to_[i], track.merge_to);
        }
        for (int i = 0; i < merge_to_size_; i++)
        {
            PerfTrack& track = tracks_[merge_to_[i]];
            merge_to(merge_to_[i], track.merge_to);
        }
    }
    int serialize(int entry_idx, int depth, char* org_buff, int buff_len);
    const char* serialize(int entry_idx);


    PerfTrack& track(int idx) { return tracks_[idx]; }
    
    const char* desc() const { return desc_; }
    char* mutable_desc() { return desc_; }
    double circles_per_ns(int t) { return  circles_per_ns_[t == PERF_COUNTER_NULL ? PERF_COUNTER_DEFAULT : t]; }
    int new_dyn_track_id() 
    { 
        if (used_track_id_ >= TRACK_MAX_COUNT)
        {
            return 0;
        }
        return ++used_track_id_; 
    }
private:
    PerfTrack tracks_[TRACK_MAX_COUNT];
    char desc_[PERF_MAX_TRACK_NAME_SIZE];
    std::array<int, TRACK_MAX_COUNT> merge_to_;
    int merge_to_size_;
    double circles_per_ns_[PERF_COUNTER_MAX];
    int used_track_id_;
    char serialize_buff_[SERIALIZE_BUFF_LEN];
};



template<int T, int S, int D>
int PerfRecord<T, S, D>::add_track_child(int idx, int cidx)
{
    if (idx < 0 || idx >= TRACK_MAX_COUNT || cidx < 0 || cidx >= TRACK_MAX_COUNT)
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



template<int T, int S, int D>
int PerfRecord<T, S, D>::add_merge_to(int idx, int to)
{
    if (idx < 0 || idx >= TRACK_MAX_COUNT || to < 0 || to >= TRACK_MAX_COUNT)
    {
        return -1;
    }

    if (idx == to)
    {
        return -2;  
    }
    if (merge_to_size_ >= TRACK_MAX_COUNT)
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


template<int T, int S, int D>
int PerfRecord<T, S, D>::init_perf(const char* desc)
{
    sprintf(desc_, "%s", desc);

    last_timestamp_ = time(NULL);
    init_timestamp_ = time(NULL);


    double chrono_rate = (double)std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::seconds(1)).count();
    chrono_rate /= 1000.0 * 1000.0 * 1000.0;
    chrono_rate = 1.0 / chrono_rate;
    circles_per_ns_[PERF_CONNTER_CHRONO] = chrono_rate;
    circles_per_ns_[PERF_COUNTER_NULL] = 0;
#ifdef WIN32
    double rdtsc_rate = 1.0/(perf_self_cpu_mhz()/1000);
    double freq_rate = perf_win_freq_rate();
    circles_per_ns_[PERF_COUNTER_RDTSC] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_RDTSC_NOFENCE] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_CLOCK] = freq_rate;
    circles_per_ns_[PERF_COUNTER_SYS] = 1.0;

#elif (defined __APPLE__)
    double rdtsc_rate = 1.0 / (perf_self_cpu_mhz() / 1000);
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

    double rdtsc_rate = 1.0 / (perf_self_cpu_mhz() / 1000);
    circles_per_ns_[PERF_COUNTER_RDTSC] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_RDTSC_NOFENCE] = rdtsc_rate;
    circles_per_ns_[PERF_COUNTER_CLOCK] = 1.0;
    circles_per_ns_[PERF_COUNTER_SYS] = 1.0;
#endif


    return 0;

}





template<int T, int S, int D>
int PerfRecord<T, S, D>::regist_track(int idx, const char* desc, unsigned int counter_type, bool overwrite)
{
    if (idx < 0)
    {
        return -1;
    }
    if (idx >= TRACK_MAX_COUNT)
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

template<int T, int S, int D>
void PerfRecord<T, S, D>::reset_childs(int idx, int depth)
{
    if (idx < 0)
    {
        return;
    }
    if (idx >= TRACK_MAX_COUNT)
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



template<int T, int S, int D>
int PerfRecord<T, S, D>::serialize(int entry_idx, int depth, char* org_buff, int buff_len)
{
    if (entry_idx < 0 || entry_idx >= TRACK_MAX_COUNT)
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
        human_time_format(buff_min, (long long)(track.cpu.min_u == LLONG_MAX ? 0 : track.cpu.min_u * circles_per_ns(track.desc.counter_type)));

        int ret = sprintf(buff, "[[ %s ]] cpu: call:%s  [avg: %s] dv: %s sum: %s  [sm: %s] hsm: %s lsm: %s max: %s min: %s  \n",
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






template<int T, int S, int D>
const char* PerfRecord<T, S, D>::serialize(int entry_idx)
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



#endif
