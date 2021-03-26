
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
#include "zperf_serialize.h"
#include <algorithm>
#ifndef ZPERF_RECORD_H
#define ZPERF_RECORD_H


#define PERF_DECLARE_BEGIN 1

#define PERF_MAX_NODE_NAME_SIZE 128
#define PERF_MAX_SERIALIZE_LINE_SIZE (PERF_MAX_NODE_NAME_SIZE + 200)
#define PERF_MAX_BROTHER_COUNT 10
#define PERF_MAX_DEPTH 5


static_assert(PERF_DECLARE_BEGIN > 0, "");


enum PerfCPURecType
{
    PERF_CPU_NORMAL,
    PERF_CPU_FAST,
    PERF_CPU_FULL,
};


struct PerfDesc
{
    char node_name[PERF_MAX_NODE_NAME_SIZE];
    int node_name_len;
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

struct PerfUser
{
    long long c;
    long long sum;
    long long t_c;
    long long t_u;
    long long merge_temp;
};

struct PerfNode
{
    bool active; 
    bool is_child;  
    std::array<unsigned int, PERF_MAX_BROTHER_COUNT> child_ids; 
    int child_count; 
    int merge_to;
    PerfDesc desc; 
    PerfCPU cpu; 
    PerfMEM mem; 
    PerfTimer timer;
    PerfUser user;
};  


template<int INST, int RESERVE, int DECLARE, int ANON>
class PerfRecord 
{
public:
    static const int PERF_RECORD_TYPE = INST;
    static const int RESERVE_COUNT = RESERVE;
    static const int DECLARE_COUNT = DECLARE;
    static const int ANON_COUNT = ANON;
    static const int MAX_COUNT = RESERVE_COUNT + DECLARE_COUNT + ANON_COUNT;
    static_assert(RESERVE_COUNT > 0, "");
    static const int SERIALIZE_BUFF_LEN = PERF_MAX_DEPTH* PERF_MAX_BROTHER_COUNT* PERF_MAX_SERIALIZE_LINE_SIZE * 2;

    static constexpr int node_begin_id() { return 0; }
    static constexpr int node_count() { return MAX_COUNT; }
    static constexpr int node_end_id() { return node_begin_id() + node_count(); }

    static constexpr int node_reserve_begin_id() { return node_begin_id(); }
    static constexpr int node_reserve_count() { return RESERVE_COUNT; }
    static constexpr int node_reserve_end_id() { return node_reserve_begin_id() + node_reserve_count(); }

    static constexpr int node_declare_begin_id() { return node_reserve_end_id(); }
    static constexpr int node_declare_count() { return DECLARE_COUNT; }
    static constexpr int node_declare_end_id() { return node_declare_begin_id() + node_declare_count(); }

    static constexpr int node_anon_begin_id() { return node_declare_end_id(); }
    static constexpr int node_anon_count() { return ANON_COUNT; }
    static constexpr int node_anon_end_id() { return node_anon_begin_id() + node_anon_count(); }
    static constexpr int node_anon_real_count() { return used_node_id_ - node_anon_begin_id(); }
    static constexpr int node_anon_real_end_id() { return used_node_id_ + 1; }

public:
    long long init_timestamp_;
    long long last_timestamp_;
public:

    PerfRecord() 
    {
        memset(nodes_, 0, sizeof(nodes_));
        desc_[0] = '\0';
        merge_to_size_ = 0;
        memset(circles_per_ns_, 0, sizeof(circles_per_ns_));
        used_node_id_ = node_anon_begin();
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
    inline int regist_node(int idx, const char* desc, unsigned int counter, bool re_reg);
    inline int bind_childs(int idx, int child);
    inline int bind_merge(int idx, int to);

    void reset_cpu(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.cpu, 0, sizeof(node.cpu));
    }
    void reset_mem(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.mem, 0, sizeof(node.mem));
    }
    void reset_timer(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.timer, 0, sizeof(node.timer));
    }
    void reset_user(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.user, 0, sizeof(node.user));
    }

    void reset_reserve_info()
    {
        for (int i = node_reserve_begin_id(); i < node_reserve_end_id(); i++)
        {
            reset_cpu(i);
            reset_mem(i);
            reset_timer(i);
            reset_user(i);
        }
        last_timestamp_ = time(NULL);
    }

    void reset_declare_info()
    {
        for (int i = node_declare_begin_id(); i < node_declare_end_id(); i++)
        {
            reset_cpu(i);
            reset_mem(i);
            reset_timer(i);
            reset_user(i);
        }
        last_timestamp_ = time(NULL);
    }

    void reset_anon_info()
    {
        for (int i = node_anon_begin_id(); i < node_anon_end_id(); i++)
        {
            reset_cpu(i);
            reset_mem(i);
            reset_timer(i);
            reset_user(i);
        }
        last_timestamp_ = time(NULL);
    }

    inline void reset_childs(int idx, int depth = 0);

    void call_cpu(int idx, long long c, long long cost)
    {
        long long dis = cost / c;
        PerfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        node.cpu.sm = node.cpu.sm == 0 ? dis : node.cpu.sm;
        node.cpu.sm = (node.cpu.sm * 12 + cost * 4) >> 4;
        node.cpu.max_u = node.cpu.max_u > dis ? node.cpu.max_u : dis;
        node.cpu.min_u = node.cpu.min_u < dis ? node.cpu.min_u : dis;
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_c += c;
        node.cpu.t_u += cost;
    }
    void call_cpu(int idx, long long cost)
    {
        PerfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = node.cpu.sm == 0 ? cost : node.cpu.sm;
        node.cpu.sm = (node.cpu.sm * 12 + cost * 4) >> 4;
        node.cpu.dv += abs(cost - node.cpu.sm);
        node.cpu.t_c += 1;
        node.cpu.t_u += cost;
    }
    void call_cpu_no_sm(int idx, long long cost)
    {
        PerfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = cost;
        node.cpu.t_c += 1;
        node.cpu.t_u += cost;
    }
    void call_cpu_no_sm(int idx, long long count, long long cost)
    {
        long long dis = cost / count;
        PerfNode& node = nodes_[idx];
        node.cpu.c += count;
        node.cpu.sum += cost;
        node.cpu.sm = dis;
        node.cpu.t_c += count;
        node.cpu.t_u += cost;
    }

    void call_cpu_full(int idx, long long cost)
    {

        PerfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        long long dis = cost;
        long long avg = node.cpu.sum / node.cpu.c;
        if (node.cpu.sm == 0)
        {
            node.cpu.sm = node.cpu.sm == 0 ? dis : node.cpu.sm;
            node.cpu.l_sm = node.cpu.l_sm == 0 ? dis : node.cpu.l_sm;
            node.cpu.h_sm = node.cpu.h_sm == 0 ? dis : node.cpu.h_sm;
        }
        long long& hlsm = dis > avg ? node.cpu.h_sm : node.cpu.l_sm;
        hlsm = (hlsm * 12 + dis * 4) >> 4;
        node.cpu.sm = (node.cpu.sm * 12 + cost * 4) >> 4;
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_c += 1;
        node.cpu.t_u += cost;
        node.cpu.max_u = node.cpu.max_u > dis ? node.cpu.max_u : dis;
        node.cpu.min_u = node.cpu.min_u < dis ? node.cpu.min_u : dis;
    }

    void call_cpu_full(int idx, long long c, long long cost)
    {
        
        PerfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        long long dis = cost / c;
        long long avg = node.cpu.sum / node.cpu.c;
        if (node.cpu.sm == 0)
        {
            node.cpu.sm = node.cpu.sm == 0 ? dis : node.cpu.sm;
            node.cpu.l_sm = node.cpu.l_sm == 0 ? dis : node.cpu.l_sm;
            node.cpu.h_sm = node.cpu.h_sm == 0 ? dis : node.cpu.h_sm;
        }
        long long& hlsm = dis > avg ? node.cpu.h_sm : node.cpu.l_sm;
        hlsm = (hlsm * 12 + dis * 4) >> 4;
        node.cpu.sm = (node.cpu.sm * 12 + cost * 4) >> 4;
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_c += c;
        node.cpu.t_u += cost;
        node.cpu.max_u = node.cpu.max_u > dis ? node.cpu.max_u : dis;
        node.cpu.min_u = node.cpu.min_u < dis ? node.cpu.min_u : dis;
    }


    void call_timer(int idx, long long stamp)
    {
        PerfNode& node = nodes_[idx];
        if (node.timer.last == 0)
        {
            node.timer.last = stamp;
            return;
        }
        call_cpu_full(idx, 1, stamp - node.timer.last);
        node.timer.last = stamp;
    }

    void call_mem(int idx, long long c, long long add)
    {
        PerfNode& node = nodes_[idx];
        node.mem.c += c;
        node.mem.sum += add;
        node.mem.t_c += c;
        node.mem.t_u += add;
    }

    void call_user(int idx, long long c, long long add)
    {
        PerfNode& node = nodes_[idx];
        node.user.c += c;
        node.user.sum += add;
        node.user.t_c += c;
        node.user.t_u += add;
    }

    void refresh_mem(int idx, long long c, long long add)
    {
        PerfNode& node = nodes_[idx];
        node.mem.c = c;
        node.mem.delta = add - node.mem.sum;
        node.mem.sum = add;
        node.mem.t_c = c;
        node.mem.t_u = add;
    }


    void merge_cpu_temp(long long t_u, int to)
    {
        PerfNode& to_node = nodes_[to];
        to_node.cpu.merge_temp += t_u;
        if (to_node.merge_to > 0)
        {
            merge_cpu_temp(t_u, to_node.merge_to);
        }
    }

    void merge_mem_temp(long long t_u, int to)
    {
        PerfNode& to_node = nodes_[to];
        to_node.mem.merge_temp += t_u;
        if (to_node.merge_to > 0)
        {
            merge_mem_temp(t_u, to_node.merge_to);
        }
    }

    void merge_user_temp(long long t_u, int to)
    {
        PerfNode& to_node = nodes_[to];
        to_node.user.merge_temp += t_u;
        if (to_node.merge_to > 0)
        {
            merge_user_temp(t_u, to_node.merge_to);
        }
    }
    void merge_proc(int idx, int to)
    {
        PerfNode& node = nodes_[idx];
        if (node.cpu.t_c > 0)
        {
            merge_cpu_temp(node.cpu.t_u, to);
            node.cpu.t_c = 0;
            node.cpu.t_u = 0;
        }
        if (node.mem.t_c > 0)
        {
            merge_mem_temp(node.mem.t_u, to);
            node.mem.t_c = 0;
            node.mem.t_u = 0;
        }
        if (node.user.t_c > 0)
        {
            merge_user_temp(node.user.t_u, to);
            node.user.t_c = 0;
            node.user.t_u = 0;
        }

    }

    void merge_to(int idx, int to)
    {
        PerfNode& to_node = nodes_[to];
        if (to_node.cpu.merge_temp > 0)
        {
            call_cpu_full(to, to_node.cpu.merge_temp);
            to_node.cpu.merge_temp = 0;
            to_node.cpu.t_c = 0;
            to_node.cpu.t_u = 0;
        }
        if (to_node.mem.merge_temp > 0)
        {
            call_mem(to, 1, to_node.mem.merge_temp);
            to_node.mem.merge_temp = 0;
            to_node.mem.t_c = 0;
            to_node.mem.t_u = 0;
        }
        if (to_node.user.merge_temp > 0)
        {
            call_user(to, 1, to_node.user.merge_temp);
            to_node.user.merge_temp = 0;
            to_node.user.t_c = 0;
            to_node.user.t_u = 0;
        }
    }

    void update_merge()
    {
        for (int i = 0; i < merge_to_size_; i++)
        {
            PerfNode& node = nodes_[merge_to_[i]];
            merge_proc(merge_to_[i], node.merge_to);
        }
        for (int i = 0; i < merge_to_size_; i++)
        {
            PerfNode& node = nodes_[merge_to_[i]];
            merge_to(merge_to_[i], node.merge_to);
        }
    }
    int serialize(int entry_idx, int depth, char* org_buff, int buff_len);
    const char* serialize(int entry_idx);


    PerfNode& node(int idx) { return nodes_[idx]; }
    
    const char* desc() const { return desc_; }
    char* mutable_desc() { return desc_; }
    double circles_per_ns(int t) { return  circles_per_ns_[t == PERF_COUNTER_NULL ? PERF_COUNTER_DEFAULT : t]; }
    int new_anon_node_id() 
    { 
        if (used_node_id_ >= MAX_COUNT - 1)
        {
            return 0;
        }
        return ++used_node_id_; 
    }
private:
    PerfNode nodes_[MAX_COUNT];
    char desc_[PERF_MAX_NODE_NAME_SIZE];
    std::array<int, MAX_COUNT> merge_to_;
    int merge_to_size_;
    double circles_per_ns_[PERF_COUNTER_MAX];
    int used_node_id_;
    char serialize_buff_[SERIALIZE_BUFF_LEN];
};



template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::bind_childs(int idx, int cidx)
{
    if (idx < 0 || idx >= MAX_COUNT || cidx < 0 || cidx >= MAX_COUNT)
    {
        return -1;
    }

    if (idx == cidx)
    {
        return -2;  
    }

    PerfNode& node = nodes_[idx];
    PerfNode& child = nodes_[cidx];
    if (!node.active || !child.active)
    {
        return -3; //regist method has memset all info ; 
    }
    if (node.child_count >= PERF_MAX_BROTHER_COUNT)
    {
        return -4;
    }
    if (std::find_if(node.child_ids.begin(), node.child_ids.end(), [cidx](int id) {return id == cidx; }) != node.child_ids.end())
    {
        return -5; //duplicate 
    }

    node.child_ids[node.child_count++] = cidx;
    child.is_child = true;
    return 0;
}



template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::bind_merge(int idx, int to)
{
    if (idx < 0 || idx >= MAX_COUNT || to < 0 || to >= MAX_COUNT)
    {
        return -1;
    }

    if (idx == to)
    {
        return -2;  
    }
    if (merge_to_size_ >= MAX_COUNT)
    {
        return -3;
    }
    PerfNode& node = nodes_[idx];
    PerfNode& to_node = nodes_[to];
    if (!node.active || !to_node.active)
    {
        return -3; //regist method has memset all info ; 
    }
    node.merge_to = to;
    merge_to_[merge_to_size_++] = idx;
    return 0;
}


template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::init_perf(const char* desc)
{
    sprintf(desc_, "%s", desc);

    last_timestamp_ = time(NULL);
    init_timestamp_ = time(NULL);

    
    circles_per_ns_[PERF_COUNTER_NULL] = 0;
    circles_per_ns_[PERF_COUNTER_SYS] = 1.0;
    circles_per_ns_[PERF_COUNTER_CLOCK] = perf_static_clock_rate();
    circles_per_ns_[PERF_CONNTER_CHRONO] = perf_static_chrono_rate();
    circles_per_ns_[PERF_COUNTER_RDTSC] = perf_static_rdtsc_rate();
    circles_per_ns_[PERF_COUNTER_RDTSCP] = perf_static_rdtsc_rate();
    circles_per_ns_[PERF_COUNTER_RDTSC_MFENCE] = perf_static_rdtsc_rate();
    circles_per_ns_[PERF_COUNTER_RDTSC_NOFENCE] = perf_static_rdtsc_rate();
    circles_per_ns_[PERF_COUNTER_NULL] = circles_per_ns_[PERF_COUNTER_DEFAULT];
    return 0;
}





template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::regist_node(int idx, const char* desc, unsigned int counter_type, bool re_reg)
{
    if (idx < 0)
    {
        return -1;
    }
    if (idx >= MAX_COUNT)
    {
        return -2;
    }
    if (desc == NULL)
    {
        return -3;
    }
    PerfNode& node = nodes_[idx];


    if (!re_reg && node.active)
    {
        return 0;
    }

    int len = (int)strlen(desc);
    if (len + 1 > PERF_MAX_NODE_NAME_SIZE)
    {
        return -4;
    }
    
    memset(&node, 0, sizeof(node));
    memcpy(node.desc.node_name, desc, len+1);
    node.desc.node_name_len = len;
    node.active = true;
    node.desc.counter_type = counter_type;
    node.cpu.min_u = LLONG_MAX;
    return 0;
}

template<int INST, int RESERVE, int DECLARE, int ANON>
void PerfRecord<INST, RESERVE, DECLARE,  ANON>::reset_childs(int idx, int depth)
{
    if (idx < 0)
    {
        return;
    }
    if (idx >= MAX_COUNT)
    {
        return;
    }
    PerfNode& node = nodes_[idx];
    memset(&node.cpu, 0, sizeof(node.cpu));
    memset(&node.mem, 0, sizeof(node.mem));
    if (depth > 5)
    {
        return;
    }
    for (int i = 0; i < node.child_count; i++)
    {
        reset_childs(node.child_ids[i], depth + 1);
    }
}



template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::serialize(int entry_idx, int depth, char* org_buff, int buff_len)
{
    if (entry_idx < 0 || entry_idx >= MAX_COUNT)
    {
        return -1;
    }
    if (buff_len < PERF_MAX_SERIALIZE_LINE_SIZE)
    {
        return -2;
    }
    char* buff = org_buff;

    PerfNode& node = nodes_[entry_idx];
    if (node.cpu.c > 0)
    {
        if (buff_len < PERF_MAX_SERIALIZE_LINE_SIZE)
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
        if (buff_len < PERF_MAX_SERIALIZE_LINE_SIZE)
        {
            return -6;
        }
        char buff_call[50];
        human_count_format(buff_call, node.cpu.c);
        char buff_avg[50];
        human_time_format(buff_avg, (long long)( node.cpu.sum * circles_per_ns(node.desc.counter_type)) / node.cpu.c);
        char buff_sm[50];
        human_time_format(buff_sm, (long long)(node.cpu.sm * circles_per_ns(node.desc.counter_type)));
        char buff_dv[50];
        human_time_format(buff_dv, (long long)(node.cpu.dv * circles_per_ns(node.desc.counter_type) / node.cpu.c));
        char buff_sum[50];
        human_time_format(buff_sum, (long long)(node.cpu.sum * circles_per_ns(node.desc.counter_type)));
        char buff_hsm[50];
        human_time_format(buff_hsm, (long long)(node.cpu.h_sm * circles_per_ns(node.desc.counter_type)));
        char buff_lsm[50];
        human_time_format(buff_lsm, (long long)(node.cpu.l_sm * circles_per_ns(node.desc.counter_type)));
        char buff_max[50];
        human_time_format(buff_max, (long long)(node.cpu.max_u * circles_per_ns(node.desc.counter_type)));
        char buff_min[50];
        human_time_format(buff_min, (long long)(node.cpu.min_u == LLONG_MAX ? 0 : node.cpu.min_u * circles_per_ns(node.desc.counter_type)));

        int ret = sprintf(buff, "[[ %s ]] cpu: call:%s  [avg: %s] dv: %s sum: %s  [sm: %s] hsm: %s lsm: %s max: %s min: %s  \n",
            node.desc.node_name, buff_call, buff_avg, buff_dv, buff_sum, buff_sm, buff_hsm, buff_lsm, buff_max, buff_min);

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
    if (node.mem.c > 0)
    {
        if (buff_len < PERF_MAX_SERIALIZE_LINE_SIZE)
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
        if (buff_len < PERF_MAX_SERIALIZE_LINE_SIZE)
        {
            return -6;
        }
        char buff_call[50];
        human_count_format(buff_call, node.mem.c);
        char buff_avg[50];
        human_mem_format(buff_avg, node.mem.sum / node.mem.c);
        char buff_use[50];
        human_mem_format(buff_use, node.mem.sum);
        char buff_front[50];
        human_mem_format(buff_front, node.mem.sum - node.mem.delta);
        char buff_delta[50];
        human_mem_format(buff_delta, node.mem.delta);
        int ret = sprintf(buff, "[[ %s ]] mem: call:%s  avg: %s  sum: %s  front: %s delta: %s \n",
            node.desc.node_name, buff_call, buff_avg, buff_use, buff_front, buff_delta);

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
    if (node.cpu.c <= 0 && node.mem.c <= 0)
    {
        if (buff_len < PERF_MAX_SERIALIZE_LINE_SIZE)
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
        if (buff_len < PERF_MAX_SERIALIZE_LINE_SIZE)
        {
            return -6;
        }
        int ret = sprintf(buff, "[%s] no any call data. \n", node.desc.node_name);
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

    for (int i = 0; i < node.child_count; i++)
    {
        int ret = serialize(node.child_ids[i], depth + 1, buff, buff_len);
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






template<int INST, int RESERVE, int DECLARE, int ANON>
const char* PerfRecord<INST, RESERVE, DECLARE,  ANON>::serialize(int entry_idx)
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
