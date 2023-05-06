
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

#include "zprof_counter.h"
#include "zprof_serialize.h"
#include <algorithm>
#include <functional>
#include <atomic>
#ifndef ZPROF_RECORD_H
#define ZPROF_RECORD_H






#define SMOOTH_CYCLES(s_cost, cost) (   (s_cost * 12 + cost * 4) >> 4   ) 
#define SMOOTH_CYCLES_WITH_INIT(s_cost, cost) ( (s_cost) == 0 ? (cost) : SMOOTH_CYCLES(s_cost, cost) )

enum ProfLevel
{
    PROF_LEVEL_NORMAL,
    PROF_LEVEL_FAST,
    PROF_LEVEL_FULL,
};


struct ProfDesc
{
    int node_name;
    int node_name_len;
    int counter_type;
    bool resident;
};

struct ProfCPU
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

struct ProfTimer
{
    long long last;
};

struct ProfMEM
{
    long long c;  
    long long sum;
    long long delta;
    long long t_u;
};


struct ProfUser
{
    long long c;
    long long sum;
    long long t_u;
};

struct ProfNode
{
    bool active;  
    int parrent;
    int jump_child;
    int first_child;
    int child_count; 
    int merge_to;
    int merge_child_count;
    int merge_current_child_count;

    ProfCPU cpu; 
    ProfMEM mem; 
    ProfTimer timer;
    ProfUser user;
    ProfVM vm;
};  

enum ProfSerializeFlags : unsigned int
{
    PROF_SER_NULL,
    PROF_SER_INNER = 0x1,
    PROF_SER_RESERVE = 0x2,
    PROF_SER_DELCARE = 0x4,
};

/*
#ifdef _FN_LOG_LOG_H_
static inline void ProfDefaultFNLogFunc(const ProfSerializer& serializer)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(serializer.buff(), (int)serializer.offset());
}
#endif
*/

template<int INST, int RESERVE, int DECLARE>
class ProfRecord 
{
public:
    using Output = void(*)(const ProfSerializer& serializer);
    enum InnerType
    {
        INNER_PROF_NULL,
        INNER_PROF_INIT_COST,
        INNER_PROF_MERGE_COST,
        INNER_PROF_REPORT_COST,
        INNER_PROF_SERIALIZE_COST,
        INNER_PROF_OUTPUT_COST,
        INNER_PROF_MEM_INFO_COST,
        INNER_PROF_COUNTER_COST,
        INNER_PROF_RECORD_COST,
        INNER_PROF_RECORD_SM_COST,
        INNER_PROF_RECORD_FULL_COST,
        INNER_PROF_COUNTER_RECORD_COST,
        INNER_PROF_ORIGIN_INC,
        INNER_PROF_ATOM_RELEAX,
        INNER_PROF_ATOM_COST,
        INNER_PROF_ATOM_SEQ_COST,
        INNER_PROF_MAX,
    };

    static constexpr int inst_id() { return INST; }



    static constexpr int node_reserve_begin_id() { return INNER_PROF_MAX; }
    static constexpr int node_reserve_count() { return RESERVE; }
    static constexpr int node_reserve_end_id() { return node_reserve_begin_id() + node_reserve_count(); }

    static constexpr int node_declare_begin_id() { return node_reserve_end_id(); }
    static constexpr int node_declare_count() { return DECLARE; }
    static constexpr int node_declare_end_id() { return node_declare_begin_id() + node_declare_count(); }
    inline int node_delcare_reg_end_id() { return declare_reg_end_id_; }

    static constexpr int node_begin_id() { return INNER_PROF_NULL + 1; }
    static constexpr int node_count() { return node_declare_end_id() - 1; }
    static constexpr int node_end_id() { return node_begin_id() + node_count(); }
    static constexpr int max_node_count() { return node_count(); }

    static constexpr int max_compact_string_size() { return 30 * (1+node_end_id()); } //reserve node no name 
    static_assert(node_end_id() == INNER_PROF_MAX + node_reserve_count() + node_declare_count(), "");

public:
    long long init_timestamp_;
    long long last_timestamp_;
public:

    ProfRecord() : compact_buffer_(compact_string_, max_compact_string_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        memset(node_descs_, 0, sizeof(node_descs_));
        merge_to_size_ = 0;
        memset(circles_per_ns_, 0, sizeof(circles_per_ns_));
        declare_reg_end_id_ = node_declare_begin_id();

        output_ = &ProfRecord::default_output;  //set default log;

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
        compact_buffer_.push_string("the string of store name is too small..");
        no_name_space_len_ = (int)(compact_buffer_.offset() - no_name_space_);
        compact_buffer_.push_char('\0');
        desc_ = 0;

    };
    static inline ProfRecord& instance()
    {
        static ProfRecord inst;
        return inst;
    }
    inline int init(const char* desc);
    inline int build_jump_path();
    inline int regist_node(int idx, const char* desc, unsigned int counter, bool resident, bool re_reg);
    inline int rename_node(int idx, const char* desc);
    inline const char* node_name(int idx);
    inline int bind_childs(int idx, int child);
    inline int bind_merge(int idx, int to);

    PROF_ALWAYS_INLINE void reset_cpu(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.cpu, 0, sizeof(node.cpu));
        node.cpu.min_u = LLONG_MAX;
    }
    PROF_ALWAYS_INLINE void reset_mem(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.mem, 0, sizeof(node.mem));
    }
    PROF_ALWAYS_INLINE void reset_vm(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.vm, 0, sizeof(node.vm));
    }
    PROF_ALWAYS_INLINE void reset_timer(int idx)
    {
        ProfNode& node = nodes_[idx];
        memset(&node.timer, 0, sizeof(node.timer));
    }
    PROF_ALWAYS_INLINE void reset_user(int idx)
    {
        ProfNode& node = nodes_[idx];
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



    inline void reset_childs(int idx, int depth = 0);

    PROF_ALWAYS_INLINE void call_cpu(int idx, long long c, long long cost)
    {
        long long dis = cost / c;
        ProfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
        node.cpu.dv += abs(dis - node.cpu.sum/node.cpu.c);
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void call_cpu(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.max_u = (node.cpu.max_u < cost ? cost : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < cost ? node.cpu.min_u : cost);
        node.cpu.dv += abs(cost - node.cpu.sm);
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void call_cpu_no_sm(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        node.cpu.sm = cost;
        node.cpu.t_u += cost;
    }
    PROF_ALWAYS_INLINE void call_cpu_no_sm(int idx, long long count, long long cost)
    {
        long long dis = cost / count;
        ProfNode& node = nodes_[idx];
        node.cpu.c += count;
        node.cpu.sum += cost;
        node.cpu.sm = dis;
        node.cpu.t_u += cost;
    }

    PROF_ALWAYS_INLINE void call_cpu_full(int idx, long long cost)
    {
        ProfNode& node = nodes_[idx];
        node.cpu.c += 1;
        node.cpu.sum += cost;
        long long dis = cost;
        long long avg = node.cpu.sum / node.cpu.c;

        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.h_sm = (dis > avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
        node.cpu.l_sm = (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_u += cost;
        node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
    }

    PROF_ALWAYS_INLINE void call_cpu_full(int idx, long long c, long long cost)
    {
        
        ProfNode& node = nodes_[idx];
        node.cpu.c += c;
        node.cpu.sum += cost;
        long long dis = cost / c;
        long long avg = node.cpu.sum / node.cpu.c;

        node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
        node.cpu.h_sm =  (dis > avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
        node.cpu.l_sm =  (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
        node.cpu.dv += abs(dis - node.cpu.sm);
        node.cpu.t_u += cost;
        node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
        node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
    }


    PROF_ALWAYS_INLINE void call_timer(int idx, long long stamp)
    {
        ProfNode& node = nodes_[idx];
        if (node.timer.last == 0)
        {
            node.timer.last = stamp;
            return;
        }
        call_cpu_full(idx, 1, stamp - node.timer.last);
        node.timer.last = stamp;
    }

    PROF_ALWAYS_INLINE void call_mem(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.mem.c += c;
        node.mem.sum += add;
        node.mem.t_u += add;
    }
    PROF_ALWAYS_INLINE void call_vm(int idx, const ProfVM& vm)
    {
        nodes_[idx].vm = vm;
    }
    PROF_ALWAYS_INLINE void call_user(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.user.c += c;
        node.user.sum += add;
        node.user.t_u += add;
    }

    PROF_ALWAYS_INLINE void refresh_mem(int idx, long long c, long long add)
    {
        ProfNode& node = nodes_[idx];
        node.mem.c = c;
        node.mem.delta = add - node.mem.sum;
        node.mem.sum = add;
        node.mem.t_u = add;
    }


    void update_merge()
    {
        ProfCounter<PROF_COUNTER_DEFAULT> cost;
        cost.start();
        for (int i = 0; i < merge_to_size_; i++)
        {
            ProfNode& leaf = nodes_[merge_to_[i]];
            ProfNode* node = NULL;
            long long append_cpu = 0;
            long long append_mem = 0;
            long long append_user = 0;
            int node_id = 0;
            node = &nodes_[leaf.merge_to];
            append_cpu = leaf.cpu.t_u;
            append_mem = leaf.mem.t_u;
            append_user = leaf.user.t_u;
            node_id = leaf.merge_to;
            leaf.cpu.t_u = 0;
            leaf.mem.t_u = 0;
            leaf.user.t_u = 0;
            do
            {
                node->cpu.t_u += append_cpu;
                node->mem.t_u += append_mem;
                node->user.t_u += append_user;
                node->merge_current_child_count++;
                if (node->merge_current_child_count >= node->merge_child_count)
                {
                    node->merge_current_child_count = 0;
                    append_cpu = node->cpu.t_u;
                    append_mem = node->mem.t_u;
                    append_user = node->user.t_u;
                    if (append_cpu > 0)
                    {
                        call_cpu_full(node_id, append_cpu);
                    }
                    if (append_mem > 0)
                    {
                        call_mem(node_id, 1, append_mem);
                    }
                    if (append_user > 0)
                    {
                        call_user(node_id, 1, append_user);
                    }
                    node->cpu.t_u = 0;
                    node->mem.t_u = 0;
                    node->user.t_u = 0;
                    if (node->merge_to == 0)
                    {
                        break;
                    }
                    node_id = node->merge_to;
                    node = &nodes_[node->merge_to];
                    continue;
                }
                break;
            } while (true);
        }
        call_cpu(INNER_PROF_MERGE_COST, cost.stop_and_save().cycles());
    }

    //递归展开  
    int recursive_serialize(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, ProfSerializer& serializer);
    

    //完整报告  
    int output_report(unsigned int flags);
    int output_one_record(int entry_idx);



    ProfNode& node(int idx) { return nodes_[idx]; }
    ProfDesc& node_desc(int idx) { return node_descs_[idx]; }
    const char* desc() const { return &compact_string_[desc_]; }
    double circles_per_ns(int t) { return  circles_per_ns_[t == PROF_COUNTER_NULL ? PROF_COUNTER_DEFAULT : t]; }

public:
    ProfSerializer& compact_buffer() { return compact_buffer_; }
public:
    void set_output(Output func) { output_ = func; }
private:
    void output_and_clean(ProfSerializer& s) { s.closing_string(); output_(s); s.reset_offset(); }
    static void default_output(const ProfSerializer& serializer) { printf("%s", serializer.buff()); }
    Output output_;
private:
    ProfNode nodes_[node_end_id()];
    ProfDesc node_descs_[node_end_id()];
    int desc_;
    std::array<int, node_end_id()> merge_to_;
    int merge_to_size_;
    double circles_per_ns_[PROF_COUNTER_MAX];
    int declare_reg_end_id_;
    char compact_string_[max_compact_string_size()];
    ProfSerializer compact_buffer_;
    int unknown_desc_;
    int reserve_desc_;
    int no_name_space_;
    int no_name_space_len_;
};



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::bind_childs(int idx, int cidx)
{
    if (idx < node_begin_id() || idx >= node_end_id() || cidx < node_begin_id() || cidx >= node_end_id())
    {
        return -1;
    }

    if (idx == cidx)
    {
        return -2;  
    }

    ProfNode& node = nodes_[idx];
    ProfNode& child = nodes_[cidx];
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



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::bind_merge(int idx, int to)
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
    ProfNode& node = nodes_[idx];
    ProfNode& to_node = nodes_[to];
    if (!node.active || !to_node.active)
    {
        return -3; //regist method has memset all info ; 
    }
    to_node.merge_child_count++;
    if (to_node.merge_child_count == 1 && to_node.merge_to != 0)
    {
        for (int i = 0; i < merge_to_size_; i++)
        {
            if (merge_to_[i] == to)
            {
                node.merge_to = to;
                merge_to_[i] = idx;
                return 0;
            }
        }
    }

    node.merge_to = to;
    merge_to_[merge_to_size_++] = idx;
    return 0;
}


template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::init(const char* desc)
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
    ProfCounter<> counter;
    counter.start();

    last_timestamp_ = time(NULL);
    init_timestamp_ = time(NULL);

    circles_per_ns_[PROF_COUNTER_NULL] = 0;
    circles_per_ns_[PROF_COUNTER_SYS] = prof_get_time_inverse_frequency<PROF_COUNTER_SYS>();
    circles_per_ns_[PROF_COUNTER_CLOCK] = prof_get_time_inverse_frequency<PROF_COUNTER_CLOCK>();
    circles_per_ns_[PROF_CONNTER_CHRONO] = prof_get_time_inverse_frequency<PROF_CONNTER_CHRONO>();
    circles_per_ns_[PROF_COUNTER_RDTSC] = prof_get_time_inverse_frequency<PROF_COUNTER_RDTSC>();
    circles_per_ns_[PROF_COUNTER_RDTSC_BTB] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSCP] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_MFENCE] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_MFENCE_BTB] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_NOFENCE] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_PURE] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_RDTSC_LOCK] = circles_per_ns_[PROF_COUNTER_RDTSC];
    circles_per_ns_[PROF_COUNTER_NULL] = circles_per_ns_[PROF_COUNTER_DEFAULT];

    for (int i = node_begin_id(); i < node_reserve_end_id(); i++)
    {
        regist_node(i, "reserve", PROF_COUNTER_DEFAULT, false, false);
    }

    regist_node(INNER_PROF_NULL, "INNER_PROF_NULL", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_INIT_COST, "INIT_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_MERGE_COST, "MERGE_COST", PROF_COUNTER_DEFAULT, true, true);

    regist_node(INNER_PROF_REPORT_COST, "REPORT_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_SERIALIZE_COST, "SERIALIZE_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_OUTPUT_COST, "OUTPUT_COST", PROF_COUNTER_DEFAULT, true, true);
    
    regist_node(INNER_PROF_MEM_INFO_COST, "MEM_INFO_COST", PROF_COUNTER_DEFAULT, true, true);

    regist_node(INNER_PROF_COUNTER_COST, "COUNTER_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_RECORD_COST, "RECORD_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_RECORD_SM_COST, "RECORD_SM_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_RECORD_FULL_COST, "RECORD_FULL_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_COUNTER_RECORD_COST, "COUNTER_RECORD_COST", PROF_COUNTER_DEFAULT, true, true);

    regist_node(INNER_PROF_ORIGIN_INC, "ORIGIN_INC", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_ATOM_RELEAX, "ATOM_RELEAX", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_ATOM_COST, "ATOM_COST", PROF_COUNTER_DEFAULT, true, true);
    regist_node(INNER_PROF_ATOM_SEQ_COST, "ATOM_SEQ_COST", PROF_COUNTER_DEFAULT, true, true);




    if (true)
    {
        ProfCounter<> self_mem_cost;
        self_mem_cost.start();
        call_vm(INNER_PROF_MEM_INFO_COST, prof_get_mem_use());
        call_cpu(INNER_PROF_MEM_INFO_COST, self_mem_cost.stop_and_save().cycles());
        call_mem(INNER_PROF_MEM_INFO_COST, 1, sizeof(*this));
        call_user(INNER_PROF_MEM_INFO_COST, 1, max_node_count());
    }

    if (true)
    {
        ProfCounter<> cost;
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            ProfCounter<> test_cost;
            test_cost.start();
            test_cost.stop_and_save();
            call_cpu(INNER_PROF_NULL, test_cost.cycles());
        }
        call_cpu(INNER_PROF_COUNTER_RECORD_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            cost.save();
        }
        call_cpu(INNER_PROF_COUNTER_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            call_cpu_no_sm(INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        call_cpu(INNER_PROF_RECORD_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            call_cpu(INNER_PROF_NULL, 1, cost.stop_and_save().cycles());
        }
        call_cpu(INNER_PROF_RECORD_SM_COST, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            call_cpu_full(INNER_PROF_NULL, 1, cost.stop_and_save().cycles());
        }
        call_cpu(INNER_PROF_RECORD_FULL_COST, 1000, cost.stop_and_save().cycles());


        std::atomic<long long> atomll_test(0);
        volatile long long origin_feetch_add_test = 0;
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            origin_feetch_add_test++;
        }
        call_cpu(INNER_PROF_ORIGIN_INC, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_relaxed);
        }
        call_cpu(INNER_PROF_ATOM_RELEAX, 1000, cost.stop_and_save().cycles());

        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test++;
        }
        call_cpu(INNER_PROF_ATOM_COST, 1000, cost.stop_and_save().cycles());

        
        cost.start();
        for (int i = 0; i < 1000; i++)
        {
            atomll_test.fetch_add(1, std::memory_order_seq_cst);
        }
        call_cpu(INNER_PROF_ATOM_SEQ_COST, 1000, cost.stop_and_save().cycles());

        reset_node(INNER_PROF_NULL);
    }

    call_cpu(INNER_PROF_INIT_COST, counter.stop_and_save().cycles());

    return 0;
}


template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::build_jump_path()
{
    for (int i = node_declare_begin_id(); i < node_declare_end_id(); )
    {
        int next_parrent_id = i + 1;
        while (next_parrent_id < node_declare_end_id())
        {
            if (nodes_[next_parrent_id].parrent == 0)
            {
                break;
            }
            next_parrent_id++;
        }
        for (int j = i; j < next_parrent_id; j++)
        {
            nodes_[j].jump_child = next_parrent_id - j - 1;
        }
        i = next_parrent_id;
    }
    return 0;
}

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::regist_node(int idx, const char* desc, unsigned int counter_type, bool resident, bool re_reg)
{
    if (idx >= node_end_id() )
    {
        return -1;
    }
    if (desc == NULL)
    {
        return -3;
    }
    
    
    ProfNode& node = nodes_[idx];


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

template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::rename_node(int idx, const char* desc)
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


    node_descs_[idx].node_name = (int)compact_buffer_.offset();// node name is "" when compact serializer full 
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


template<int INST, int RESERVE, int DECLARE>
const char* ProfRecord<INST, RESERVE, DECLARE>::node_name(int idx)
{
    if (idx < node_begin_id() || idx >= node_end_id())
    {
        return "";
    }
    ProfDesc& desc = node_descs_[idx];
    if (desc.node_name >= max_compact_string_size())
    {
        return "";
    }
    return &compact_string_[desc.node_name];
};


template<int INST, int RESERVE, int DECLARE>
void ProfRecord<INST, RESERVE, DECLARE>::reset_childs(int idx, int depth)
{
    if (idx < node_begin_id() || idx >= node_end_id())
    {
        return ;
    }
    ProfNode& node = nodes_[idx];
    reset_cpu(idx);
    reset_mem(idx);
    reset_timer(idx);
    reset_user(idx);
    if (depth > PROF_MAX_DEPTH)
    {
        return;
    }
    for (int i = node.first_child; i < node.first_child + node.child_count; i++)
    {
        ProfNode& child = nodes_[i];
        if (child.parrent == idx)
        {
           reset_childs(i, depth + 1);
        }
    }
}



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::recursive_serialize(int entry_idx, int depth, const char* opt_name, size_t opt_name_len, ProfSerializer& serializer)
{
    if (entry_idx >= node_end_id())
    {
        return -1;
    }

    if (serializer.buff_len() <= PROF_LINE_MIN_SIZE)
    {
        return -2;
    }

    if (output_ == nullptr)
    {
        return -3;
    }

    ProfNode& node = nodes_[entry_idx];

    if (depth == 0 && node.parrent)
    {
        return 0;
    }
    if (!node.active)
    {
        return 0;
    }
    if (node_descs_[entry_idx].node_name + node_descs_[entry_idx].node_name_len >= max_compact_string_size())
    {
        return 0;
    }
    if (node_descs_[entry_idx].counter_type >= PROF_COUNTER_MAX)
    {
        return 0;
    }


    
    ProfCounter<> cost_single_serialize;

    const char* desc_name = &compact_string_[node_descs_[entry_idx].node_name];
    size_t desc_len = node_descs_[entry_idx].node_name_len;
    double cpu_rate = circles_per_ns(node_descs_[entry_idx].counter_type);
    if (opt_name != NULL)
    {
        desc_name = opt_name;
        desc_len = opt_name_len;
    }

    int name_blank = (int)desc_len + depth  + depth;
    name_blank = name_blank < 35 ? 35 - name_blank : 0;

    if (desc_len + name_blank > PROF_DESC_MAX_SIZE)
    {
        return -5;
    }

    serializer.reset_offset();

#define STRLEN(str) str, strlen(str)
    if (node.cpu.c > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(desc_name, desc_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));

        serializer.push_string(STRLEN("\tcpu*|-- "));
        if (true)
        {
            serializer.push_human_count(node.cpu.c);
            serializer.push_string(STRLEN("c, "));
            serializer.push_human_time((long long)(node.cpu.sum * cpu_rate / node.cpu.c));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.sum * cpu_rate));
        }

        
        if (node.cpu.min_u != LLONG_MAX && node.cpu.max_u > 0)
        {
            serializer.push_string(STRLEN(" --|*\tmin-max|-- "));
            serializer.push_human_time((long long)(node.cpu.max_u * cpu_rate));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.min_u * cpu_rate));
        }

        
        if (node.cpu.dv > 0 || node.cpu.sm > 0)
        {
            serializer.push_string(STRLEN(" --| \tdv-sm|-- "));
            serializer.push_human_time((long long)(node.cpu.dv * cpu_rate / node.cpu.c));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.sm * cpu_rate));
        }

        
        if (node.cpu.h_sm > 0 || node.cpu.l_sm > 0)
        {
            serializer.push_string(STRLEN(" --| \th-l|-- "));
            serializer.push_human_time((long long)(node.cpu.h_sm * cpu_rate));
            serializer.push_string(STRLEN(", "));
            serializer.push_human_time((long long)(node.cpu.l_sm * cpu_rate));
        }
        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());

        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());

    }

    if (node.mem.c > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(desc_name, desc_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));

        serializer.push_string(STRLEN("\tmem*|-- "));
        if (true)
        {
            serializer.push_human_count(node.mem.c);
            serializer.push_string(STRLEN("c, "));
            serializer.push_human_mem(node.mem.sum / node.mem.c);
            serializer.push_string(STRLEN(", "));
            serializer.push_human_mem(node.mem.sum);
        }

        serializer.push_string(STRLEN(" --||-- "));
        if (node.mem.delta > 0)
        {
            serializer.push_human_mem(node.mem.sum - node.mem.delta);
            serializer.push_string(STRLEN(", "));
            serializer.push_human_mem(node.mem.delta);
        }
        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());


        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());
    }

    if (node.vm.rss_size + node.vm.vm_size > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(desc_name, desc_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));


        serializer.push_string(STRLEN("\t vm*|-- "));
        if (true)
        {
            serializer.push_human_mem(node.vm.vm_size);
            serializer.push_string(STRLEN("(vm), "));
            serializer.push_human_mem(node.vm.rss_size);
            serializer.push_string(STRLEN("(rss), "));
            serializer.push_human_mem(node.vm.shr_size);
            serializer.push_string(STRLEN("(shr), "));
            serializer.push_human_mem(node.vm.rss_size - node.vm.shr_size);
            serializer.push_string(STRLEN("(uss)"));
        }

        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());

        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());
    }

    if (node.user.c > 0)
    {
        cost_single_serialize.start();
        serializer.push_indent(depth * 2);
        serializer.push_string(STRLEN("|"));
        serializer.push_number((unsigned long long)entry_idx, 3);
        serializer.push_string(STRLEN("| "));
        serializer.push_string(desc_name, desc_len);
        serializer.push_blank(name_blank);
        serializer.push_string(STRLEN(" |"));


        serializer.push_string(STRLEN("\tuser*|-- "));
        if (true)
        {
            serializer.push_human_count(node.user.c);
            serializer.push_string(STRLEN("c, "));
            serializer.push_human_count(node.user.sum / node.user.c);
            serializer.push_string(STRLEN(", "));
            serializer.push_human_count(node.user.sum);
        }

        serializer.push_string(STRLEN(" --|"));
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_SERIALIZE_COST, cost_single_serialize.cycles());

        cost_single_serialize.start();
        output_and_clean(serializer);
        cost_single_serialize.stop_and_save();
        call_cpu_full(INNER_PROF_OUTPUT_COST, cost_single_serialize.cycles());
    }

    if (depth > PROF_MAX_DEPTH)
    {
        serializer.push_indent(depth * 2);
        output_and_clean(serializer);
        return -4;
    }

    for (int i = node.first_child; i < node.first_child + node.child_count; i++)
    {
        ProfNode& child = nodes_[i];
        if (child.parrent == entry_idx)
        {
            int ret = recursive_serialize(i, depth + 1, NULL, 0, serializer);
            if (ret < 0)
            {
                return ret;
            }
        }
    }

    return 0;
}






template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::output_one_record(int entry_idx)
{
    ProfStackSerializer serializer;
    int ret = recursive_serialize(entry_idx, 0, NULL, 0, serializer);
    (void)ret;
    return ret;
}



template<int INST, int RESERVE, int DECLARE>
int ProfRecord<INST, RESERVE, DECLARE>::output_report(unsigned int flags)
{
    if (output_ == nullptr)
    {
        return -1;
    }
    ProfCounter<> cost;
    cost.start();
    ProfStackSerializer serializer;

    serializer.reset_offset();
    output_and_clean(serializer);


    serializer.push_char('=', 30);
    serializer.push_char('\t');
    serializer.push_string(desc());
    serializer.push_string(STRLEN(" begin output: "));
    serializer.push_now_date();
    serializer.push_char('\t');
    serializer.push_char('=', 30);
    output_and_clean(serializer);

    serializer.push_string(STRLEN("| -- index -- | ---    cpu  ------------ | ----------   hits, avg, sum   ---------- | ---- max, min ---- | ------ dv, sm ------ |  --- hsm, lsm --- | "));
    output_and_clean(serializer);
    serializer.push_string(STRLEN("| -- index -- | ---    mem  ---------- | ----------   hits, avg, sum   ---------- | ------ last, delta ------ | "));
    output_and_clean(serializer);
    serializer.push_string(STRLEN("| -- index -- | ---    vm  ------------ | ----------   vm, rss, shr, uss   ------------------ | " ));
    output_and_clean(serializer);

    serializer.push_string(STRLEN("| -- index -- | ---    user  ----------- | -----------  hits, avg, sum   ---------- | "));
    output_and_clean(serializer);

    if (flags & PROF_SER_INNER)
    {
        serializer.push_string(STRLEN(PROF_LINE_FEED));
        for (int i = INNER_PROF_NULL + 1; i < INNER_PROF_MAX; i++)
        {
            int ret = recursive_serialize(i, 0, NULL, 0, serializer);
            (void)ret;
        }
    }

    if (flags & PROF_SER_RESERVE)
    {
        serializer.push_string(STRLEN(PROF_LINE_FEED));
        for (int i = node_reserve_begin_id(); i < node_reserve_end_id(); i++)
        {
            int ret = recursive_serialize(i, 0, NULL, 0, serializer);
            (void)ret;
        }
    }
    
    if (flags & PROF_SER_DELCARE)
    {
        serializer.push_string(STRLEN(PROF_LINE_FEED));
        for (int i = node_declare_begin_id(); i < node_delcare_reg_end_id(); )
        {
            int ret = recursive_serialize(i, 0, NULL, 0, serializer);
            (void)ret;
            i += nodes_[i].jump_child + 1;
        }
    }

    serializer.reset_offset();
    serializer.push_char('=', 30);
    serializer.push_char('\t');
    serializer.push_string(" end : ");
    serializer.push_now_date();
    serializer.push_char('\t');
    serializer.push_char('=', 30);
    output_and_clean(serializer);
    output_and_clean(serializer);

    call_cpu(INNER_PROF_REPORT_COST, cost.stop_and_save().cycles());
    return 0;
}


#endif
