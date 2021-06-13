
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
#include <functional>
#include <atomic>
#ifndef ZPERF_RECORD_H
#define ZPERF_RECORD_H




#define PERF_MAX_DEPTH 5


enum PerfCPURecType
{
    PERF_CPU_NORMAL,
    PERF_CPU_FAST,
    PERF_CPU_FULL,
};


struct PerfDesc
{
    int node_name;
    int node_name_len;
    int counter_type;
    bool resident;
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
    int parrent;
    int first_child;
    int child_count; 
    int merge_to;
    PerfCPU cpu; 
    PerfMEM mem; 
    PerfTimer timer;
    PerfUser user;
};  


template<int INST, int RESERVE, int DECLARE, int ANON>
class PerfRecord 
{
public:
    enum InnerType
    {
        INST_INNER_NULL,
        INST_INNER_INIT_COST,
        INST_INNER_SERIALIZE_COST,
        INST_INNER_SELF_MEM_COST,
        INST_INNER_AUTO_TEST_COST,
        INST_INNER_FULL_AUTO_COST,
        INST_INNER_COUNTER_COST,
        INST_INNER_ORIGIN_INC,
        INST_INNER_ATOM_RELEAX,
        INST_INNER_ATOM_COST,
        INST_INNER_ATOM_SEQ_COST,
        INST_INNER_MAX,
    };

    static constexpr int inst_id() { return INST; }



    static constexpr int node_reserve_begin_id() { return INST_INNER_MAX; }
    static constexpr int node_reserve_count() { return RESERVE; }
    static constexpr int node_reserve_end_id() { return node_reserve_begin_id() + node_reserve_count(); }

    static constexpr int node_declare_begin_id() { return node_reserve_end_id(); }
    static constexpr int node_declare_count() { return DECLARE; }
    static constexpr int node_declare_end_id() { return node_declare_begin_id() + node_declare_count(); }
    inline int node_delcare_reg_end_id() { return declare_reg_end_id_; }

    static constexpr int node_anon_begin_id() { return node_declare_end_id(); }
    static constexpr int node_anon_count() { return ANON; }
    static constexpr int node_anon_end_id() { return node_anon_begin_id() + node_anon_count(); }
    inline int node_anon_real_count() { return used_node_id_ - node_anon_begin_id(); }
    inline int node_anon_real_end_id() { return used_node_id_; }


    static constexpr int node_begin_id() { return INST_INNER_NULL + 1; }
    static constexpr int node_count() { return node_anon_end_id() - 1; }
    static constexpr int node_end_id() { return node_anon_end_id(); }
    static constexpr int max_node_count() { return node_count(); }

    static constexpr int max_serialize_buff_size() { return 500; }
    static constexpr int max_compact_string_size() { return 30 * (1+node_end_id()); } //reserve node no name 

    static_assert(node_end_id() == INST_INNER_MAX + node_reserve_count() + node_declare_count() + node_anon_count(), "");

public:
    long long init_timestamp_;
    long long last_timestamp_;
public:

    PerfRecord() : compact_buffer_(compact_string_, max_compact_string_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        memset(node_descs_, 0, sizeof(node_descs_));
        merge_to_size_ = 0;
        memset(circles_per_ns_, 0, sizeof(circles_per_ns_));
        used_node_id_ = node_anon_begin_id();
        declare_reg_end_id_ = node_declare_begin_id();
        serialize_buff_[0] = '\0';
        init_timestamp_ = 0;
        last_timestamp_ = 0;
        static_assert(max_compact_string_size() > 150, "");
        unknown_desc_ = 0;
        compact_buffer_.push_string("unknown");
        compact_buffer_.push_char('\0');
        reserve_desc_ = (int)compact_buffer_.offset();
        compact_buffer_.push_string("reserve");
        compact_buffer_.push_char('\0');
        no_name_space_ = (int)compact_buffer_.offset();
        compact_buffer_.serialize("the string of store name is too small..");
        no_name_space_len_ = (int)(compact_buffer_.offset() - no_name_space_);
        compact_buffer_.push_char('\0');
        desc_ = 0;

    };
    static inline PerfRecord& instance()
    {
        static PerfRecord inst;
        return inst;
    }
    inline int init_perf(const char* desc);
    inline int regist_node(int idx, const char* desc, unsigned int counter, bool resident, bool re_reg);
    inline int rename_node(int idx, const char* desc);
    inline int bind_childs(int idx, int child);
    inline int bind_merge(int idx, int to);

    PERF_ALWAYS_INLINE void reset_cpu(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.cpu, 0, sizeof(node.cpu));
    }
    PERF_ALWAYS_INLINE void reset_mem(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.mem, 0, sizeof(node.mem));
    }
    PERF_ALWAYS_INLINE void reset_timer(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.timer, 0, sizeof(node.timer));
    }
    PERF_ALWAYS_INLINE void reset_user(int idx)
    {
        PerfNode& node = nodes_[idx];
        memset(&node.user, 0, sizeof(node.user));
    }
    PERF_ALWAYS_INLINE void reset_node(int idx)
    {
        reset_cpu(idx);
        reset_mem(idx);
        reset_timer(idx);
        reset_user(idx);
    }
    void clean_node_info_range(int first_idx, int end_idx, bool keep_resident = true)
    {
        for (int idx = first_idx; idx < end_idx; idx++)
        {
            if (!keep_resident || !node_descs_[idx].resident)
            {
                reset_node(idx);
            }
        }
    }

    void clean_reserve_info(bool keep_resident = true)
    {
        clean_node_info_range(node_reserve_begin_id(), node_reserve_end_id(), keep_resident);
        last_timestamp_ = time(NULL);
    }

    void clean_declare_info(bool keep_resident = true)
    {
        clean_node_info_range(node_declare_begin_id(), node_declare_end_id(), keep_resident);
        last_timestamp_ = time(NULL);
    }

    void clean_anon_info(bool keep_resident = true)
    {
        clean_node_info_range(node_anon_begin_id(), node_anon_end_id(), keep_resident);
        last_timestamp_ = time(NULL);
    }

    inline void reset_childs(int idx, int depth = 0);

    PERF_ALWAYS_INLINE void call_cpu(int idx, long long c, long long cost)
    {
        long long dis = cost / c;
        PerfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        node.cpu.sm = node.cpu.sm == 0 ? dis : node.cpu.sm;
        node.cpu.sm = (node.cpu.sm * 12 + dis * 4) >> 4;
        node.cpu.max_u = node.cpu.max_u > dis ? node.cpu.max_u : dis;
        node.cpu.min_u = node.cpu.min_u < dis ? node.cpu.min_u : dis;
        node.cpu.dv += abs(dis - node.cpu.sum/node.cpu.c);
        node.cpu.t_c += c;
        node.cpu.t_u += cost;
    }
    PERF_ALWAYS_INLINE void call_cpu(int idx, long long cost)
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
    PERF_ALWAYS_INLINE void call_cpu_no_sm(int idx, long long cost)
    {
        PerfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = cost;
        node.cpu.t_c += 1;
        node.cpu.t_u += cost;
    }
    PERF_ALWAYS_INLINE void call_cpu_no_sm(int idx, long long count, long long cost)
    {
        long long dis = cost / count;
        PerfNode& node = nodes_[idx];
        node.cpu.c += count;
        node.cpu.sum += cost;
        node.cpu.sm = dis;
        node.cpu.t_c += count;
        node.cpu.t_u += cost;
    }

    PERF_ALWAYS_INLINE void call_cpu_full(int idx, long long cost)
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

    PERF_ALWAYS_INLINE void call_cpu_full(int idx, long long c, long long cost)
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


    PERF_ALWAYS_INLINE void call_timer(int idx, long long stamp)
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

    PERF_ALWAYS_INLINE void call_mem(int idx, long long c, long long add)
    {
        PerfNode& node = nodes_[idx];
        node.mem.c += c;
        node.mem.sum += add;
        node.mem.t_c += c;
        node.mem.t_u += add;
    }

    PERF_ALWAYS_INLINE void call_user(int idx, long long c, long long add)
    {
        PerfNode& node = nodes_[idx];
        node.user.c += c;
        node.user.sum += add;
        node.user.t_c += c;
        node.user.t_u += add;
    }

    PERF_ALWAYS_INLINE void refresh_mem(int idx, long long c, long long add)
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
    int serialize(int entry_idx, int depth, PerfSerializeBuffer& buffer, std::function<void(const PerfSerializeBuffer& buffer)> call_log = NULL);
    PerfSerializeBuffer serialize(int entry_idx, std::function<void(const PerfSerializeBuffer& buffer)> call_log = NULL);
    int serialize(std::function<void(const PerfSerializeBuffer& buffer)> call_log);


    PerfNode& node(int idx) { return nodes_[idx]; }
    PerfDesc& node_desc(int idx) { return node_descs_[idx]; }
    const char* desc() const { return &compact_string_[desc_]; }
    double circles_per_ns(int t) { return  circles_per_ns_[t == PERF_COUNTER_NULL ? PERF_COUNTER_DEFAULT : t]; }
    int new_anon_node_id() 
    { 
        if (used_node_id_ >= node_anon_end_id())
        {
            return 0;
        }
        return used_node_id_++;
    }
public:
    PerfSerializeBuffer& compact_buffer() { return compact_buffer_; }
private:
    PerfNode nodes_[node_end_id()];
    PerfDesc node_descs_[node_end_id()];
    int desc_;
    std::array<int, node_end_id()> merge_to_;
    int merge_to_size_;
    double circles_per_ns_[PERF_COUNTER_MAX];
    int declare_reg_end_id_;
    int used_node_id_;
    char serialize_buff_[max_serialize_buff_size()];
    char compact_string_[max_compact_string_size()];
    PerfSerializeBuffer compact_buffer_;
    int unknown_desc_;
    int reserve_desc_;
    int no_name_space_;
    int no_name_space_len_;
};



template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::bind_childs(int idx, int cidx)
{
    if (idx < node_begin_id() || idx >= node_end_id() || cidx < node_begin_id() || cidx >= node_end_id())
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
    if (node.first_child == 0)
    {
        node.first_child = cidx;
        node.child_count = 1;
    }
    else 
    {
        if (cidx < node.first_child)
        {
            node.child_count += node.first_child - cidx;
            node.first_child = cidx;
        }
        else if (cidx >= node.first_child + node.child_count)
        {
            node.child_count = cidx - node.first_child + 1;
        }
    }
    
    child.parrent = idx;
    return 0;
}



template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::bind_merge(int idx, int to)
{
    if (idx < node_begin_id() || idx >= node_end_id() || to < node_begin_id() || to >= node_end_id())
    {
        return -1;
    }

    if (idx == to)
    {
        return -2;  
    }
    if (merge_to_size_ >= node_end_id())
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
    if (desc == NULL || compact_buffer_.is_full())
    {
        desc_ = 0;
    }
    else
    {
        desc_ = (int)compact_buffer_.offset();
        compact_buffer_.push_string("desc");
        compact_buffer_.push_char('\0');
        compact_buffer_.closing_string();
    }
    PerfCounter<> counter;
    counter.start();

    last_timestamp_ = time(NULL);
    init_timestamp_ = time(NULL);

    circles_per_ns_[PERF_COUNTER_NULL] = 0;
    circles_per_ns_[PERF_COUNTER_SYS] = perf_get_time_inverse_frequency<PERF_COUNTER_SYS>();
    circles_per_ns_[PERF_COUNTER_CLOCK] = perf_get_time_inverse_frequency<PERF_COUNTER_CLOCK>();
    circles_per_ns_[PERF_CONNTER_CHRONO] = perf_get_time_inverse_frequency<PERF_CONNTER_CHRONO>();
    circles_per_ns_[PERF_COUNTER_RDTSC] = perf_get_time_inverse_frequency<PERF_COUNTER_RDTSC>();
    circles_per_ns_[PERF_COUNTER_RDTSC_STOP] = circles_per_ns_[PERF_COUNTER_RDTSC];
    circles_per_ns_[PERF_COUNTER_RDTSCP] = circles_per_ns_[PERF_COUNTER_RDTSC];
    circles_per_ns_[PERF_COUNTER_RDTSC_MFENCE] = circles_per_ns_[PERF_COUNTER_RDTSC];
    circles_per_ns_[PERF_COUNTER_RDTSC_NOFENCE] = circles_per_ns_[PERF_COUNTER_RDTSC];
    circles_per_ns_[PERF_COUNTER_RDTSC_PURE] = circles_per_ns_[PERF_COUNTER_RDTSC];
    circles_per_ns_[PERF_COUNTER_RDTSC_LOCK] = circles_per_ns_[PERF_COUNTER_RDTSC];
    circles_per_ns_[PERF_COUNTER_NULL] = circles_per_ns_[PERF_COUNTER_DEFAULT];

    for (int i = node_begin_id(); i < node_reserve_end_id(); i++)
    {
        regist_node(i, "reserve", PERF_COUNTER_DEFAULT, false, false);
    }

    regist_node(INST_INNER_INIT_COST, "INST_INNER_INIT_COST", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_SERIALIZE_COST, "INST_INNER_SERIALIZE_COST", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_SELF_MEM_COST, "INST_INNER_SELF_MEM_COST", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_AUTO_TEST_COST, "INST_INNER_AUTO_TEST_COST", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_FULL_AUTO_COST, "INST_INNER_FULL_AUTO_COST", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_COUNTER_COST, "INST_INNER_COUNTER_COST", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_ORIGIN_INC, "INST_INNER_ORIGIN_INC", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_ATOM_RELEAX, "INST_INNER_ATOM_RELEAX", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_ATOM_COST, "INST_INNER_ATOM_COST", PERF_COUNTER_DEFAULT, true, true);
    regist_node(INST_INNER_ATOM_SEQ_COST, "INST_INNER_ATOM_SEQ_COST", PERF_COUNTER_DEFAULT, true, true);

    if (true)
    {
        PerfCounter<> self_mem_cost;
        self_mem_cost.start();
        call_mem(INST_INNER_SELF_MEM_COST, 1, perf_get_mem_use());
        call_cpu(INST_INNER_SELF_MEM_COST, self_mem_cost.stop_and_save().cycles());
    }

    if (true)
    {
        PerfCounter<> cost;
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            PerfCounter<> test_cost;
            test_cost.start();
            test_cost.stop_and_save();
            call_cpu(INST_INNER_AUTO_TEST_COST, test_cost.cycles());
        }
        call_cpu(INST_INNER_FULL_AUTO_COST, 1000, cost.stop_and_save().cycles());
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            call_cpu_no_sm(INST_INNER_AUTO_TEST_COST, cost.stop_and_save().cycles());
        }
        call_cpu(INST_INNER_COUNTER_COST, 1000, cost.stop_and_save().cycles());
        std::atomic<long long> atomll_test(0);
        volatile long long origin_feetch_add_test = 0;
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            origin_feetch_add_test++;
        }
        call_cpu(INST_INNER_ORIGIN_INC, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_relaxed);
        }
        call_cpu(INST_INNER_ATOM_RELEAX, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test++;
        }
        call_cpu(INST_INNER_ATOM_COST, 1000, cost.stop_and_save().cycles());

        
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_seq_cst);
        }
        call_cpu(INST_INNER_ATOM_SEQ_COST, 1000, cost.stop_and_save().cycles());

        call_cpu(INST_INNER_AUTO_TEST_COST, origin_feetch_add_test);
        call_cpu(INST_INNER_AUTO_TEST_COST, atomll_test.load());
        reset_node(INST_INNER_AUTO_TEST_COST);
    }

    call_cpu(INST_INNER_INIT_COST, counter.stop_and_save().cycles());

    return 0;
}





template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::regist_node(int idx, const char* desc, unsigned int counter_type, bool resident, bool re_reg)
{
    if (idx < node_begin_id() || idx >= node_end_id() )
    {
        return -1;
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

    memset(&node, 0, sizeof(node));
    rename_node(idx, desc);
    node_descs_[idx].counter_type = counter_type;
    node_descs_[idx].resident = resident;
    node.active = true;
    node.cpu.min_u = LLONG_MAX;

    if (idx >= node_declare_begin_id() && idx < node_declare_end_id() && idx + 1 > declare_reg_end_id_)
    {
        declare_reg_end_id_ = idx + 1;
    }

    return 0;
}

template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::rename_node(int idx, const char* desc)
{
    if (idx < node_begin_id() || idx >= node_end_id() )
    {
        return -1;
    }
    if (desc == NULL)
    {
        return -3;
    }
    if (strcmp(desc, "reserve") == 0)
    {
        node_descs_[idx].node_name = reserve_desc_;
        node_descs_[idx].node_name_len = 7;
        return 0;
    }


    node_descs_[idx].node_name = (int)compact_buffer_.offset();// node name is "" when compact buffer full 
    compact_buffer_.push_string(desc);
    compact_buffer_.push_char('\0');
    compact_buffer_.closing_string();
    node_descs_[idx].node_name_len = (int)strlen(&compact_string_[node_descs_[idx].node_name]);
    if (node_descs_[idx].node_name_len == 0)
    {
        node_descs_[idx].node_name = no_name_space_;
        node_descs_[idx].node_name_len = no_name_space_len_;
    }
    return 0;
}



template<int INST, int RESERVE, int DECLARE, int ANON>
void PerfRecord<INST, RESERVE, DECLARE,  ANON>::reset_childs(int idx, int depth)
{
    if (idx < node_begin_id() || idx >= node_end_id())
    {
        return ;
    }
    PerfNode& node = nodes_[idx];
    reset_cpu(idx);
    reset_mem(idx);
    reset_timer(idx);
    reset_user(idx);
    if (depth > PERF_MAX_DEPTH)
    {
        return;
    }
    for (int i = node.first_child; i < node.first_child + node.child_count; i++)
    {
        PerfNode& child = nodes_[i];
        if (child.parrent == idx)
        {
           reset_childs(i, depth + 1);
        }
    }
}



template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE,  ANON>::serialize(int entry_idx, int depth, PerfSerializeBuffer& buffer, std::function<void(const PerfSerializeBuffer& buffer)> call_log)
{
    if (entry_idx < node_begin_id() || entry_idx >= node_end_id())
    {
        return -1;
    }

    if (buffer.offset() >= buffer.buff_len())
    {
        return -2;
    }
    const int min_line_size = 120;
    if (buffer.buff_len() - buffer.offset() < min_line_size)
    {
        buffer.push_char(' ', depth * 2);
        buffer.push_string("serialize buffer too short ..." PERF_LINE_FEED);
        buffer.closing_string();
        if (call_log)
        {
            call_log(buffer);
            buffer.reset_offset();
        }
        return -3;
    }


    PerfNode& node = nodes_[entry_idx];

    if (depth == 0 && node.parrent)
    {
        return 0;
    }
    if (!node.active)
    {
        return 0;
    }

    int name_blank = node_descs_[entry_idx].node_name_len + depth * 2 + 6;
    if (name_blank < 35)
    {
        name_blank = 35 - name_blank;
    }
    else
    {
        name_blank = 10 - ((name_blank - 35) % 10);
    }



    if (node.cpu.c > 0)
    {
        buffer.push_char(' ', depth * 2);
        buffer.serialize("[[ %s ]] ", &compact_string_[node_descs_[entry_idx].node_name]);
        //buffer.serialize("[[ %03d| %s ]] ", entry_idx, &compact_string_[node_descs_[entry_idx].node_name]);
        buffer.push_char(' ', name_blank);
        buffer.push_string("cpu: call:");

        buffer.push_human_count(node.cpu.c);
        buffer.push_string("\t avg:");
        buffer.push_human_time((long long)(node.cpu.sum * circles_per_ns(node_descs_[entry_idx].counter_type) / node.cpu.c));
        buffer.push_string("\t sum:");
        buffer.push_human_time((long long)(node.cpu.sum * circles_per_ns(node_descs_[entry_idx].counter_type)));
        if (node.cpu.dv > 0 || node.cpu.sm > 0)
        {
            buffer.push_string("\t --||-- ");
            buffer.push_string(" dv:");
            buffer.push_human_time((long long)(node.cpu.dv * circles_per_ns(node_descs_[entry_idx].counter_type) / node.cpu.c));
            buffer.push_string(" sm:");
            buffer.push_human_time((long long)(node.cpu.sm * circles_per_ns(node_descs_[entry_idx].counter_type)));
        }
        if (node.cpu.h_sm > 0 || node.cpu.l_sm > 0)
        {
            buffer.push_string("\t --||-- ");
            buffer.push_string(" hsm:");
            buffer.push_human_time((long long)(node.cpu.h_sm * circles_per_ns(node_descs_[entry_idx].counter_type)));
            buffer.push_string(" lsm:");
            buffer.push_human_time((long long)(node.cpu.l_sm * circles_per_ns(node_descs_[entry_idx].counter_type)));
        }
        if (node.cpu.min_u != LLONG_MAX && node.cpu.max_u > 0)
        {
            buffer.push_string("\t --||-- ");
            buffer.push_string(" max:");
            buffer.push_human_time((long long)(node.cpu.max_u * circles_per_ns(node_descs_[entry_idx].counter_type)));
            buffer.push_string(" min:");
            buffer.push_human_time((long long)(node.cpu.min_u * circles_per_ns(node_descs_[entry_idx].counter_type)));
        }
        buffer.push_string(PERF_LINE_FEED);
        buffer.closing_string();
        if (call_log)
        {
            call_log(buffer);
            buffer.reset_offset();
        }
    }

    if (node.mem.c > 0)
    {
        buffer.push_char(' ', depth * 2);
        buffer.serialize("[[ %s ]] ", &compact_string_[node_descs_[entry_idx].node_name]);
//        buffer.serialize("[[ %03d| %s ]] ", entry_idx, &compact_string_[node_descs_[entry_idx].node_name]);
        buffer.push_char(' ', name_blank);
        buffer.push_string("mem: call:");

        buffer.push_human_count(node.mem.c);
        buffer.push_string("\t avg:");
        buffer.push_human_mem(node.mem.sum / node.mem.c);
        buffer.push_string("\t sum:");
        buffer.push_human_mem(node.mem.sum);
        if (node.mem.delta > 0)
        {
            buffer.push_string("\t last sum:");
            buffer.push_human_mem(node.mem.sum - node.mem.delta);
            buffer.push_string("\t delta:");
            buffer.push_human_mem(node.mem.delta);
        }
        buffer.push_string(PERF_LINE_FEED);
        buffer.closing_string();
        if (call_log)
        {
            call_log(buffer);
            buffer.reset_offset();
        }
    }
    if (node.user.c > 0)
    {
        buffer.push_char(' ', depth * 2);
        buffer.serialize("[[ %s ]] ", &compact_string_[node_descs_[entry_idx].node_name]);
        //        buffer.serialize("[[ %03d| %s ]] ", &compact_string_[entry_idx, node_descs_[entry_idx].node_name]);
        buffer.push_char(' ', name_blank);
        buffer.push_string("usr: call:");

        buffer.push_human_count(node.user.c);
        buffer.push_string("\t avg:");
        buffer.push_human_count(node.user.sum / node.user.c);
        buffer.push_string("\t sum:");
        buffer.push_human_count(node.user.sum);
        
        buffer.push_string(PERF_LINE_FEED);
        buffer.closing_string();
        if (call_log)
        {
            call_log(buffer);
            buffer.reset_offset();
        }
    }
    if (depth > PERF_MAX_DEPTH)
    {
        buffer.push_char(' ', depth * 2);
        buffer.push_string("more node in here ... " PERF_LINE_FEED);
        buffer.closing_string();
        if (call_log)
        {
            call_log(buffer);
            buffer.reset_offset();
        }
        return -4;
    }

    for (int i = node.first_child; i < node.first_child + node.child_count; i++)
    {
        PerfNode& child = nodes_[i];
        if (child.parrent == entry_idx)
        {
            int ret = serialize(i, depth + 1, buffer, call_log);
            if (ret < 0)
            {
                return ret;
            }
        }
    }

    return 0;
}






template<int INST, int RESERVE, int DECLARE, int ANON>
PerfSerializeBuffer PerfRecord<INST, RESERVE, DECLARE,  ANON>::serialize(int entry_idx, std::function<void(const PerfSerializeBuffer& buffer)> call_log)
{
    PerfSerializeBuffer buffer(serialize_buff_, sizeof(serialize_buff_));
    int ret = serialize(entry_idx, 0, buffer, call_log);
    (void)ret;
    buffer.closing_string();
    return buffer;
}


template<int INST, int RESERVE, int DECLARE, int ANON>
int PerfRecord<INST, RESERVE, DECLARE, ANON>::serialize(std::function<void(const PerfSerializeBuffer& buffer)> call_log)
{
    PerfCounter<> counter;
    counter.start();
   
    PerfSerializeBuffer buffer(serialize_buff_, sizeof(serialize_buff_));
    if (!call_log)
    {
        return -1;
    }


    buffer.push_string(PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();


    buffer.push_char('-', 30);
    buffer.push_char('\t');
    buffer.push_string(desc());
    buffer.push_string(" begin serialize: ");
    buffer.push_now_date();
    buffer.push_char('\t');
    buffer.push_char('-', 30);
    buffer.push_string(PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();
    for (int i = INST_INNER_NULL + 1; i < INST_INNER_MAX; i++)
    {
        int ret = serialize(i, 0, buffer, call_log);
        (void)ret;
    }
    buffer.push_string("-----------------------" PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();
    for (int i = node_reserve_begin_id(); i < node_reserve_end_id(); i++)
    {
        int ret = serialize(i, 0, buffer, call_log);
        (void)ret;
    }
    buffer.push_string("-----------------------" PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();
    for (int i = node_declare_begin_id(); i < node_delcare_reg_end_id(); i++)
    {
        int ret = serialize(i, 0, buffer, call_log);
        (void)ret;
    }
    buffer.push_string("-----------------------" PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();
    for (int i = node_anon_begin_id(); i < node_anon_real_end_id(); i++)
    {
        int ret = serialize(i, 0, buffer, call_log);
        (void)ret;
    }
    call_cpu(INST_INNER_SERIALIZE_COST, counter.stop_and_save().cycles());




    buffer.push_char('-', 30);
    buffer.push_char('\t');
    buffer.push_string(" end : ");
    buffer.push_now_date();
    buffer.push_char('\t');
    buffer.push_char('-', 30);
    buffer.push_string(PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();

    buffer.push_char('1', 120);
    buffer.push_string(PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();


    buffer.push_string(PERF_LINE_FEED);
    call_log(buffer);
    buffer.reset_offset();


    return 0;
}


#endif
