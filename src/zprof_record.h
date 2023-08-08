
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

    constexpr static int g_node_size = sizeof(RecordNode);


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
            rerecord_user(INNER_RESET_TS, zprof::Clock<>::sys_now_ms());        
        }

        void reset_declare_node(bool keep_resident = true)
        {
            reset_range_node(declare_begin_id(), declare_end_id(), keep_resident);
            rerecord_user(INNER_RESET_TS, zprof::Clock<>::sys_now_ms());
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
        PROF_ALWAYS_INLINE void record_user(int idx, long long param1, long long param2 = 0, long long param3 = 0, long long param4 = 0)
        {
            RecordNode& node = nodes_[idx];
            node.user.param1 += param1;
            node.user.param2 += param2;
            node.user.param3 += param3;
            node.user.param4 += param4;
        }
        PROF_ALWAYS_INLINE void rerecord_user(int idx, long long param1, long long param2 = 0, long long param3 = 0, long long param4 = 0)
        {
            RecordNode& node = nodes_[idx];
            node.user.param1 = param1;
            node.user.param2 = param2;
            node.user.param3 = param3;
            node.user.param4 = param4;
        }

        PROF_ALWAYS_INLINE void rerecord_mem(int idx, long long c, long long add)
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
            rerecord_user(INNER_INIT_TS, zprof::Clock<>::sys_now_ms());
            rerecord_user(INNER_RESET_TS, zprof::Clock<>::sys_now_ms());
            rerecord_user(INNER_OUTPUT_TS, zprof::Clock<>::sys_now_ms());
        }

        if (true)
        {
            record_vm(INNER_INIT_COST, get_self_mem());
            record_mem(INNER_INIT_COST, max_count(), sizeof(*this));
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
            int id = 0;
            node = &nodes_[leaf.merge.to];
            append_cpu = leaf.cpu.t_u;
            append_mem = leaf.mem.t_u;
            id = leaf.merge.to;
            leaf.cpu.t_u = 0;
            leaf.mem.t_u = 0;
            do
            {
                node->cpu.t_u += append_cpu;
                node->mem.t_u += append_mem;
                node->merge.merged++;
                if (node->merge.merged >= node->merge.childs)
                {
                    node->merge.merged = 0;
                    append_cpu = node->cpu.t_u;
                    append_mem = node->mem.t_u;
                    if (append_cpu > 0)
                    {
                        record_cpu_full(id, append_cpu);
                    }
                    if (append_mem > 0)
                    {
                        record_mem(id, 1, append_mem);
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

        if (node.user.param1 != 0 || node.user.param2 != 0 || node.user.param3 != 0 || node.user.param4 != 0)
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
                rp.push_human_count(node.user.param1);
                rp.push_string(STRLEN(", "));
                rp.push_human_count(node.user.param2);
                rp.push_string(STRLEN(", "));
                rp.push_human_count(node.user.param3);
                rp.push_string(STRLEN(", "));
                rp.push_human_count(node.user.param4);
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
        rerecord_user(INNER_OUTPUT_TS, zprof::Clock<>::sys_now_ms());

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
        rp.push_human_time((Clock<>::sys_now_ms() - nodes_[INNER_INIT_TS].user.param1)*1000*1000);
        rp.push_string(STRLEN("] dist reset time:["));
        rp.push_human_time((Clock<>::sys_now_ms() - nodes_[INNER_RESET_TS].user.param1) * 1000 * 1000);
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
