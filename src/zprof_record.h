
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

#include "zprof_clock.h"
#include "zprof_report.h"
#include <algorithm>
#include <functional>
#include <atomic>




namespace zprof
{

    #define SMOOTH_CYCLES(s_cost, cost) (   (s_cost * 12 + cost * 4) >> 4   ) 
    #define SMOOTH_CYCLES_WITH_INIT(s_cost, cost) ( (s_cost) == 0 ? (cost) : SMOOTH_CYCLES(s_cost, cost) )

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
        int counter_type;
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
        ProfVM vm;
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
        long long init_timestamp_;
        long long reset_timestamp_;
        long long output_timestamp_;
    public:
        static inline Record& instance()
        {
            static Record inst;
            return inst;
        }

    public:
        Record();
        int init(const char* title);
        int regist(int idx, const char* name, unsigned int counter, bool resident, bool re_reg);
        const char* title() const { return &compact_data_[title_]; }

        const char* name(int idx);
        int rename(int idx, const char* name);


        int bind_childs(int idx, int child);
        int build_jump_path();

        int bind_merge(int to, int child);
        void do_merge();

        PROF_ALWAYS_INLINE void reset_cpu(int idx)
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
            reset_timestamp_ = time(NULL);
        }

        void reset_declare_node(bool keep_resident = true)
        {
            reset_range_node(declare_begin_id(), declare_end_id(), keep_resident);
            reset_timestamp_ = time(NULL);
        }



        inline void reset_childs(int idx, int depth = 0);

        PROF_ALWAYS_INLINE void record_cpu(int idx, long long c, long long cost)
        {
            long long dis = cost / c;
            RecordNode& node = nodes_[idx];
            node.cpu.c += c;
            node.cpu.sum += cost;
            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
            node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
            node.cpu.dv += abs(dis - node.cpu.sum/node.cpu.c);
            node.cpu.t_u += cost;
        }
        PROF_ALWAYS_INLINE void record_cpu(int idx, long long cost)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += cost;
            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
            node.cpu.max_u = (node.cpu.max_u < cost ? cost : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < cost ? node.cpu.min_u : cost);
            node.cpu.dv += abs(cost - node.cpu.sm);
            node.cpu.t_u += cost;
        }
        PROF_ALWAYS_INLINE void record_cpu_no_sm(int idx, long long cost)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += cost;
            node.cpu.sm = cost;
            node.cpu.t_u += cost;
        }
        PROF_ALWAYS_INLINE void record_cpu_no_sm(int idx, long long count, long long cost)
        {
            long long dis = cost / count;
            RecordNode& node = nodes_[idx];
            node.cpu.c += count;
            node.cpu.sum += cost;
            node.cpu.sm = dis;
            node.cpu.t_u += cost;
        }

        PROF_ALWAYS_INLINE void record_cpu_full(int idx, long long cost)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += cost;
            long long dis = cost;
            long long avg = node.cpu.sum / node.cpu.c;

            node.cpu.sm = SMOOTH_CYCLES_WITH_INIT(node.cpu.sm, cost);
            node.cpu.h_sm = (dis >= avg ? SMOOTH_CYCLES_WITH_INIT(node.cpu.h_sm, dis) : node.cpu.h_sm);
            node.cpu.l_sm = (dis > avg ? node.cpu.l_sm : SMOOTH_CYCLES_WITH_INIT(node.cpu.l_sm, dis));
            node.cpu.dv += abs(dis - node.cpu.sm);
            node.cpu.t_u += cost;
            node.cpu.max_u = (node.cpu.max_u < dis ? dis : node.cpu.max_u);
            node.cpu.min_u = (node.cpu.min_u < dis ? node.cpu.min_u : dis);
        }

        PROF_ALWAYS_INLINE void record_cpu_full(int idx, long long c, long long cost)
        {
        
            RecordNode& node = nodes_[idx];
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
        PROF_ALWAYS_INLINE void record_vm(int idx, const ProfVM& vm)
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

        PROF_ALWAYS_INLINE void overwrite_mem(int idx, long long c, long long add)
        {
            reset_mem(idx);
            record_mem(idx, c, add);
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
    
        double particle_for_ns(int t) { return  particle_for_ns_[t == CLOCK_NULL ? CLOCK_DEFAULT : t]; }




     //output interface
    public:
        void set_output(ReportProc func) { output_ = func; }
    protected:
        void output_and_clean(Report& s) { s.closing_string(); output_(s); s.reset_offset(); }
        static void default_output(const Report& rp) { printf("%s\n", rp.buff()); }
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
        double particle_for_ns_[CLOCK_MAX];
    };

    template<int INST, int RESERVE, int DECLARE>
    Record<INST, RESERVE, DECLARE>::Record() : compact_writer_(compact_data_, compact_data_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        merge_leafs_size_ = 0;
        memset(particle_for_ns_, 0, sizeof(particle_for_ns_));
        declare_window_ = declare_begin_id();

        output_ = &Record::default_output;  //set default log;

        init_timestamp_ = 0;
        reset_timestamp_ = 0;
        output_timestamp_ = 0;
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
        zprof::Clock<> counter;
        counter.start();

        
        init_timestamp_ = time(NULL);
        reset_timestamp_ = time(NULL);
        output_timestamp_ = time(NULL);

        particle_for_ns_[CLOCK_NULL] = 0;
        particle_for_ns_[CLOCK_SYS] = get_inverse_frequency<CLOCK_SYS>();
        particle_for_ns_[CLOCK_CLOCK] = get_inverse_frequency<CLOCK_CLOCK>();
        particle_for_ns_[CLOCK_CHRONO] = get_inverse_frequency<CLOCK_CHRONO>();
        particle_for_ns_[CLOCK_CHRONO_STEADY] = get_inverse_frequency<CLOCK_CHRONO_STEADY>();
        particle_for_ns_[CLOCK_CHRONO_SYS] = get_inverse_frequency<CLOCK_CHRONO_SYS>();
        particle_for_ns_[CLOCK_RDTSC] = get_inverse_frequency<CLOCK_RDTSC>();
        particle_for_ns_[CLOCK_RDTSC_BTB] = particle_for_ns_[CLOCK_RDTSC];
        particle_for_ns_[CLOCK_RDTSCP] = particle_for_ns_[CLOCK_RDTSC];
        particle_for_ns_[CLOCK_RDTSC_MFENCE] = particle_for_ns_[CLOCK_RDTSC];
        particle_for_ns_[CLOCK_RDTSC_MFENCE_BTB] = particle_for_ns_[CLOCK_RDTSC];
        particle_for_ns_[CLOCK_RDTSC_NOFENCE] = particle_for_ns_[CLOCK_RDTSC];
        particle_for_ns_[CLOCK_RDTSC_PURE] = particle_for_ns_[CLOCK_RDTSC];
        particle_for_ns_[CLOCK_RDTSC_LOCK] = particle_for_ns_[CLOCK_RDTSC];
        particle_for_ns_[CLOCK_NULL] = particle_for_ns_[CLOCK_DEFAULT];

        for (int i = begin_id(); i < reserve_end_id(); i++)
        {
            regist(i, "reserve", CLOCK_DEFAULT, false, false);
        }

        regist(INNER_NULL, "INNER_NULL", CLOCK_DEFAULT, true, true);
        regist(INNER_INIT_COST, "INIT_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_MERGE_COST, "MERGE_COST", CLOCK_DEFAULT, true, true);

        regist(INNER_REPORT_COST, "REPORT_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_SERIALIZE_COST, "SERIALIZE_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_OUTPUT_COST, "OUTPUT_COST", CLOCK_DEFAULT, true, true);
    
        regist(INNER_MEM_INFO_COST, "MEM_INFO_COST", CLOCK_DEFAULT, true, true);

        regist(INNER_CLOCK_COST, "COUNTER_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_RECORD_COST, "RECORD_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_RECORD_SM_COST, "RECORD_SM_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_RECORD_FULL_COST, "RECORD_FULL_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_CLOCK_RECORD_COST, "COUNTER_RECORD_COST", CLOCK_DEFAULT, true, true);

        regist(INNER_ORIGIN_INC, "ORIGIN_INC", CLOCK_DEFAULT, true, true);
        regist(INNER_ATOM_RELEAX, "ATOM_RELEAX", CLOCK_DEFAULT, true, true);
        regist(INNER_ATOM_COST, "ATOM_COST", CLOCK_DEFAULT, true, true);
        regist(INNER_ATOM_SEQ_COST, "ATOM_SEQ_COST", CLOCK_DEFAULT, true, true);




        if (true)
        {
            zprof::Clock<> self_mem_cost;
            self_mem_cost.start();
            record_vm(INNER_MEM_INFO_COST, get_self_mem());
            record_cpu(INNER_MEM_INFO_COST, self_mem_cost.stop_and_save().duration_ticks());
            record_mem(INNER_MEM_INFO_COST, 1, sizeof(*this));
            record_user(INNER_MEM_INFO_COST, 1, max_count());
        }

        if (true)
        {
            zprof::Clock<> cost;
            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                zprof::Clock<> test_cost;
                test_cost.start();
                test_cost.stop_and_save();
                record_cpu(INNER_NULL, test_cost.duration_ticks());
            }
            record_cpu(INNER_CLOCK_RECORD_COST, 1000, cost.stop_and_save().duration_ticks());

            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                cost.save();
            }
            record_cpu(INNER_CLOCK_COST, 1000, cost.stop_and_save().duration_ticks());

            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                record_cpu_no_sm(INNER_NULL, cost.stop_and_save().duration_ticks());
            }
            record_cpu(INNER_RECORD_COST, 1000, cost.stop_and_save().duration_ticks());

            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                record_cpu(INNER_NULL, 1, cost.stop_and_save().duration_ticks());
            }
            record_cpu(INNER_RECORD_SM_COST, 1000, cost.stop_and_save().duration_ticks());

            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                record_cpu_full(INNER_NULL, 1, cost.stop_and_save().duration_ticks());
            }
            record_cpu(INNER_RECORD_FULL_COST, 1000, cost.stop_and_save().duration_ticks());


            std::atomic<long long> atomll_test(0);
            volatile long long origin_feetch_add_test = 0;
            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                origin_feetch_add_test++;
            }
            record_cpu(INNER_ORIGIN_INC, 1000, cost.stop_and_save().duration_ticks());

            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_relaxed);
            }
            record_cpu(INNER_ATOM_RELEAX, 1000, cost.stop_and_save().duration_ticks());

            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test++;
            }
            record_cpu(INNER_ATOM_COST, 1000, cost.stop_and_save().duration_ticks());

        
            cost.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_seq_cst);
            }
            record_cpu(INNER_ATOM_SEQ_COST, 1000, cost.stop_and_save().duration_ticks());

            reset_node(INNER_NULL);
        }

        record_cpu(INNER_INIT_COST, counter.stop_and_save().duration_ticks());

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
    int Record<INST, RESERVE, DECLARE>::regist(int idx, const char* name, unsigned int counter_type, bool resident, bool re_reg)
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
        nodes_[idx].traits.counter_type = counter_type;
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
        Clock<CLOCK_DEFAULT> cost;
        cost.start();
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
        record_cpu(INNER_MERGE_COST, cost.stop_and_save().duration_ticks());
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
        if (node.traits.counter_type >= CLOCK_MAX)
        {
            return 0;
        }


    
        zprof::Clock<> single_line_cost;

        const char* name = &compact_data_[node.traits.name];
        size_t name_len = node.traits.name_len;
        double cpu_rate = particle_for_ns(node.traits.counter_type);
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
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.duration_ticks());

            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.duration_ticks());

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
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.duration_ticks());


            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.duration_ticks());
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
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.duration_ticks());

            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.duration_ticks());
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
            record_cpu_full(INNER_SERIALIZE_COST, single_line_cost.duration_ticks());

            single_line_cost.start();
            output_and_clean(rp);
            single_line_cost.stop_and_save();
            record_cpu_full(INNER_OUTPUT_COST, single_line_cost.duration_ticks());
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
        output_timestamp_ = time(NULL);
        zprof::Clock<> cost;
        cost.start();
        StaticReport rp;

        rp.reset_offset();
        output_and_clean(rp);


        rp.push_char('=', 30);
        rp.push_char('\t');
        rp.push_string(title());
        rp.push_string(STRLEN(" begin output: "));
        rp.push_now_date();
        rp.push_char('\t');
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

        record_cpu(INNER_REPORT_COST, cost.stop_and_save().duration_ticks());
        return 0;
    }

}

#endif
