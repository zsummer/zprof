
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

#ifndef ZPROF_RECORD_H_
#define ZPROF_RECORD_H_

#include "zprof_clock.h"
#include "zprof_report.h"
#include <algorithm>
#include <functional>
#include <atomic>




namespace zprof
{
    //平滑  
    #define SMOOTH_CYCLES(s_ticks, ticks) (   (s_ticks * 12 + ticks * 4) >> 4   ) 
    #define SMOOTH_CYCLES_WITH_INIT(s_ticks, ticks) ( (s_ticks) == 0 ? (ticks) : SMOOTH_CYCLES(s_ticks, ticks) )  
    
    #define UNWIND_STR(str) str, strlen(str)


    static constexpr int  kCompactDataUnitSize = 30;
    static constexpr int  kCompactDataBuffMinSize = 150;
    static constexpr int  kRecordFormatAlignSize = 35;

    enum RecordLevel
    {
        kRecordLevelNormal,
        kRecordLevelFast,
        kRecordLevelFull,
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

    constexpr static int kNodeSize = sizeof(RecordNode);


    enum OutFlags : unsigned int
    {
        kOutFlagNull,
        kOutFlagInner = 0x1,
        kOutFlagReserve = 0x2,
        kOutFlagDelcare = 0x4,
        kOutFlagAll = 0xffff,
    };

    /*
    #ifdef _FN_LOG_LOG_H_
    static inline void ProfDefaultFNLogFunc(const Report& rp)
    {
        LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
    }
    #endif
    */

    template<int kInst, int kReserve, int kDeclare>
    class Record 
    {
    public:
        using ReportProc = void(*)(const Report& rp);
        enum InnerType
        {
            kInnerNull,
            kInnerInitTs,
            kInnerResetTs,
            kInnerOutputTs,
            kInnerInitCost,
            kInnerMergeCost,
            kInnerReportCost,
            kInnerSerializeCost,
            kInnerOutputCost,
            kInnerClockCost,
            kInnerRecordCost,
            kInnerRecordSmoothCost,
            kInnerRecordFullCost,
            kInnerClockRecordCost,
            kInnerOriginInc,
            kInnerAtomRelax,
            kInnerAtomCost,
            kInnerAtomSeqCost,
            kInnerMax,
        };

        static constexpr int inst_id() { return kInst; }



        static constexpr int reserve_begin_id() { return kInnerMax; }
        static constexpr int reserve_count() { return kReserve; }
        static constexpr int reserve_end_id() { return reserve_begin_id() + reserve_count(); }

        static constexpr int declare_begin_id() { return reserve_end_id(); }
        static constexpr int declare_count() { return kDeclare; }
        static constexpr int declare_end_id() { return declare_begin_id() + declare_count(); }
        inline int declare_window() { return declare_window_; }

        static constexpr int begin_id() { return kInnerNull + 1; }
        static constexpr int count() { return declare_end_id() - 1; }
        static constexpr int end_id() { return begin_id() + count(); }
        static constexpr int max_count() { return count(); }

        static constexpr int compact_data_size() { return kCompactDataUnitSize * (1+end_id()); } //reserve node no name 
        static_assert(end_id() == kInnerMax + reserve_count() + declare_count(), "");


    public:
        static inline Record& Instance()
        {
            static Record inst;
            return inst;
        }

    public:
        Record();
        int Init(const char* title);
        int Regist(int idx, const char* name, unsigned int clk, bool resident, bool re_reg);
        const char* Title() const { return &compact_data_[title_]; }

        const char* Name(int idx);
        int Rename(int idx, const char* name);


        int BindChilds(int idx, int child);
        int BuildJumpPath();

        int BindMerge(int to, int child);
        void DoMerge();

        PROF_ALWAYS_INLINE  RecordNode& Node(int idx)
        {
            return nodes_[idx];;
        }
        PROF_ALWAYS_INLINE  void ResetCpu(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.cpu, 0, sizeof(node.cpu));
            node.cpu.min_u = LLONG_MAX;
        }
        PROF_ALWAYS_INLINE void ResetMem(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.mem, 0, sizeof(node.mem));
        }
        PROF_ALWAYS_INLINE void ResetVm(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.vm, 0, sizeof(node.vm));
        }
        PROF_ALWAYS_INLINE void ResetTimer(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.timer, 0, sizeof(node.timer));
        }
        PROF_ALWAYS_INLINE void ResetUser(int idx)
        {
            RecordNode& node = nodes_[idx];
            memset(&node.user, 0, sizeof(node.user));
        }
        PROF_ALWAYS_INLINE void ResetNode(int idx)
        {
            ResetCpu(idx);
            ResetMem(idx);
            ResetVm(idx);
            ResetTimer(idx);
            ResetUser(idx);
        }

        void ResetRangeNode(int first_idx, int end_idx, bool keep_resident = true)
        {
            for (int idx = first_idx; idx < end_idx; idx++)
            {
                if (!keep_resident || !nodes_[idx].traits.resident)
                {
                    ResetNode(idx);
                }
            }
        }

        void ResetReserveNode(bool keep_resident = true)
        {
            ResetRangeNode(reserve_begin_id(), reserve_end_id(), keep_resident);
            RerecordUser(kInnerResetTs, zprof::Clock<>::SystemNowMS());        
        }

        void ResetDeclareNode(bool keep_resident = true)
        {
            ResetRangeNode(declare_begin_id(), declare_end_id(), keep_resident);
            RerecordUser(kInnerResetTs, zprof::Clock<>::SystemNowMS());
        }



        inline void ResetChilds(int idx, int depth = 0);

        PROF_ALWAYS_INLINE void RecordCpu(int idx, long long c, long long ticks)
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
        PROF_ALWAYS_INLINE void RecordCpu(int idx, long long ticks)
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
        PROF_ALWAYS_INLINE void RecordCpuNoSM(int idx, long long ticks)
        {
            RecordNode& node = nodes_[idx];
            node.cpu.c += 1;
            node.cpu.sum += ticks;
            node.cpu.sm = ticks;
            node.cpu.t_u += ticks;
        }
        PROF_ALWAYS_INLINE void RecordCpuNoSM(int idx, long long count, long long ticks)
        {
            long long dis = ticks / count;
            RecordNode& node = nodes_[idx];
            node.cpu.c += count;
            node.cpu.sum += ticks;
            node.cpu.sm = dis;
            node.cpu.t_u += ticks;
        }

        PROF_ALWAYS_INLINE void RecordCpuFull(int idx, long long ticks)
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

        PROF_ALWAYS_INLINE void RecordCpuFull(int idx, long long c, long long ticks)
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


        PROF_ALWAYS_INLINE void RecordTimer(int idx, long long stamp)
        {
            RecordNode& node = nodes_[idx];
            if (node.timer.last == 0)
            {
                node.timer.last = stamp;
                return;
            }
            RecordCpuFull(idx, 1, stamp - node.timer.last);
            node.timer.last = stamp;
        }

        PROF_ALWAYS_INLINE void RecordMem(int idx, long long c, long long add)
        {
            RecordNode& node = nodes_[idx];
            node.mem.c += c;
            node.mem.sum += add;
            node.mem.t_u += add;
        }
        PROF_ALWAYS_INLINE void RecordVm(int idx, const VMData& vm)
        {
            nodes_[idx].vm = vm;
        }
        PROF_ALWAYS_INLINE void RecordUser(int idx, long long param1, long long param2 = 0, long long param3 = 0, long long param4 = 0)
        {
            RecordNode& node = nodes_[idx];
            node.user.param1 += param1;
            node.user.param2 += param2;
            node.user.param3 += param3;
            node.user.param4 += param4;
        }
        PROF_ALWAYS_INLINE void RerecordUser(int idx, long long param1, long long param2 = 0, long long param3 = 0, long long param4 = 0)
        {
            RecordNode& node = nodes_[idx];
            node.user.param1 = param1;
            node.user.param2 = param2;
            node.user.param3 = param3;
            node.user.param4 = param4;
        }

        PROF_ALWAYS_INLINE void RerecordMem(int idx, long long c, long long add)
        {
            ResetMem(idx);
            RecordMem(idx, c, add);
        }


        //递归展开  

        int OutputCpu(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank);
        int OutputMem(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank);
        int OutputVm(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank);
        int OutputUser(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank);
        int RecursiveOutput(int entry_idx, int depth, const char* opt_name, int opt_name_len, Report& rp);


        //完整报告  
        int OutputReport(unsigned int flags = kOutFlagAll);
        int OutputOneRecord(int entry_idx);
        int OutputTempRecord(const char* opt_name, int opt_name_len);
        int OutputTempRecord(const char* opt_name);


    public:
        Report& compact_writer() { return compact_writer_; }
        RecordNode& node(int idx) { return nodes_[idx]; }
    
        double particle_for_ns(int t) { return  particle_for_ns_[t]; }




     //output interface
    public:
        void SetOutputFunc(ReportProc func) { output_ = func; }
    protected:
        void OutputAndClean(Report& s) { s.ClosingString(); output_(s); s.reset_offset(); }
        static void DefaultOutputFunc(const Report& rp) { printf("%s\n", rp.buff()); }
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
        double particle_for_ns_[kClockMAX];
    };

    template<int kInst, int kReserve, int kDeclare>
    Record<kInst, kReserve, kDeclare>::Record() : compact_writer_(compact_data_, compact_data_size())
    {
        memset(nodes_, 0, sizeof(nodes_));
        merge_leafs_size_ = 0;
        memset(particle_for_ns_, 0, sizeof(particle_for_ns_));
        declare_window_ = declare_begin_id();

        output_ = &Record::DefaultOutputFunc;  //set default log;

        static_assert(compact_data_size() > kCompactDataBuffMinSize, "");
        compact_data_[0] = '\0';
        unknown_desc_ = 0;
        compact_writer_.PushString("unknown");
        compact_writer_.PushChar('\0');
        reserve_desc_ = (int)compact_writer_.offset();
        compact_writer_.PushString("reserve");
        compact_writer_.PushChar('\0');
        no_name_space_ = (int)compact_writer_.offset();
        compact_writer_.PushString("null(name empty or over buffers)");
        no_name_space_len_ = (int)(compact_writer_.offset() - no_name_space_);
        compact_writer_.PushChar('\0');
        title_ = 0;

    };



    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::Init(const char* title)
    {
        if (title == NULL || compact_writer_.IsFull())
        {
            title_ = 0;
        }
        else
        {
            title_ = (int)compact_writer_.offset();
            compact_writer_.PushString(title);
            compact_writer_.PushChar('\0');
            compact_writer_.ClosingString();
        }
        zprof::Clock<> clk;
        clk.start();

        


        particle_for_ns_[kClockNULL] = 0;
        particle_for_ns_[kClockSystem] = GetInverseFrequency<kClockSystem>();
        particle_for_ns_[kClockClock] = GetInverseFrequency<kClockClock>();
        particle_for_ns_[kClockChrono] = GetInverseFrequency<kClockChrono>();
        particle_for_ns_[kClockSteadyChrono] = GetInverseFrequency<kClockSteadyChrono>();
        particle_for_ns_[kClockSystemChrono] = GetInverseFrequency<kClockSystemChrono>();
        particle_for_ns_[kClockSystemMS] = GetInverseFrequency<kClockSystemMS>();
        particle_for_ns_[kClockPureRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockVolatileRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockMFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockLockRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockRDTSCP] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockBTBFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();
        particle_for_ns_[kClockBTBMFenceRDTSC] = GetInverseFrequency<kClockPureRDTSC>();

        particle_for_ns_[kClockNULL] = GetInverseFrequency<zprof::kClockDefatultLevel >();

        for (int i = begin_id(); i < reserve_end_id(); i++)
        {
            Regist(i, "reserve", zprof::kClockDefatultLevel, false, false);
        }

        Regist(kInnerNull, "kInnerNull", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerInitTs, "kInnerInitTs", kClockSystemMS, true, true);
        Regist(kInnerResetTs, "kInnerResetTs", kClockSystemMS, true, true);
        Regist(kInnerOutputTs, "kInnerOutputTs", kClockSystemMS, true, true);
        Regist(kInnerInitCost, "kInnerInitCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerMergeCost, "kInnerMergeCost", zprof::kClockDefatultLevel, true, true);

        Regist(kInnerReportCost, "kInnerReportCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerSerializeCost, "kInnerSerializeCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerOutputCost, "kInnerOutputCost", zprof::kClockDefatultLevel, true, true);
    
        Regist(kInnerClockCost, "kInnerClockCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerRecordCost, "kInnerRecordCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerRecordSmoothCost, "kInnerRecordSmoothCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerRecordFullCost, "kInnerRecordFullCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerClockRecordCost, "kInnerClockRecordCost", zprof::kClockDefatultLevel, true, true);

        Regist(kInnerOriginInc, "kInnerOriginInc", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerAtomRelax, "kInnerAtomRelax", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerAtomCost, "kInnerAtomCost", zprof::kClockDefatultLevel, true, true);
        Regist(kInnerAtomSeqCost, "kInnerAtomSeqCost", zprof::kClockDefatultLevel, true, true);


        if (true)
        {
            RerecordUser(kInnerInitTs, zprof::Clock<>::SystemNowMS());
            RerecordUser(kInnerResetTs, zprof::Clock<>::SystemNowMS());
            RerecordUser(kInnerOutputTs, zprof::Clock<>::SystemNowMS());
        }

        if (true)
        {
            RecordVm(kInnerInitCost, GetSelfMem());
            RecordMem(kInnerInitCost, max_count(), sizeof(*this));
        }

        if (true)
        {
            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                zprof::Clock<> test_cost;
                test_cost.start();
                test_cost.stop_and_save();
                RecordCpu(kInnerNull, test_cost.cost());
            }
            RecordCpu(kInnerClockRecordCost, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                clk.save();
            }
            RecordCpu(kInnerClockCost, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                RecordCpuNoSM(kInnerNull, clk.stop_and_save().cost());
            }
            RecordCpu(kInnerRecordCost, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                RecordCpu(kInnerNull, 1, clk.stop_and_save().cost());
            }
            RecordCpu(kInnerRecordSmoothCost, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                RecordCpuFull(kInnerNull, 1, clk.stop_and_save().cost());
            }
            RecordCpu(kInnerRecordFullCost, 1000, clk.stop_and_save().cost());


            std::atomic<long long> atomll_test(0);
            volatile long long origin_feetch_add_test = 0;
            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                origin_feetch_add_test++;
            }
            RecordCpu(kInnerOriginInc, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_relaxed);
            }
            RecordCpu(kInnerAtomRelax, 1000, clk.stop_and_save().cost());

            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test++;
            }
            RecordCpu(kInnerAtomCost, 1000, clk.stop_and_save().cost());

        
            clk.start();
            for (int i = 0; i < 1000; i++)
            {
                atomll_test.fetch_add(1, std::memory_order_seq_cst);
            }
            RecordCpu(kInnerAtomSeqCost, 1000, clk.stop_and_save().cost());

            ResetNode(kInnerNull);
        }

        RecordCpu(kInnerInitCost, clk.stop_and_save().cost());

        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::BuildJumpPath()
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

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::Regist(int idx, const char* name, unsigned int clk, bool resident, bool re_reg)
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
        Rename(idx, name);
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

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::Rename(int idx, const char* name)
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
        compact_writer_.PushString(name);
        compact_writer_.PushChar('\0');
        compact_writer_.ClosingString();
        nodes_[idx].traits.name_len = (int)strlen(&compact_data_[nodes_[idx].traits.name]);
        if (nodes_[idx].traits.name_len == 0)
        {
            nodes_[idx].traits.name = no_name_space_;
            nodes_[idx].traits.name_len = no_name_space_len_;
        }
        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    const char* Record<kInst, kReserve, kDeclare>::Name(int idx)
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


    template<int kInst, int kReserve, int kDeclare>
    void Record<kInst, kReserve, kDeclare>::ResetChilds(int idx, int depth)
    {
        if (idx < begin_id() || idx >= end_id())
        {
            return ;
        }
        RecordNode& node = nodes_[idx];
        ResetCpu(idx);
        ResetMem(idx);
        ResetTimer(idx);
        ResetUser(idx);
        if (depth > kProfMaxDepth)
        {
            return;
        }
        for (int i = node.show.child; i < node.show.child + node.show.window; i++)
        {
            RecordNode& child = nodes_[i];
            if (child.show.upper == idx)
            {
               ResetChilds(i, depth + 1);
            }
        }
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::BindChilds(int idx, int cidx)
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
            return -3; //Regist method has memset all info ; 
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



    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::BindMerge(int to, int child)
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
            return -3; //Regist method has memset all info ; 
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


    template<int kInst, int kReserve, int kDeclare>
    void Record<kInst, kReserve, kDeclare>::DoMerge()
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
                        RecordCpuFull(id, append_cpu);
                    }
                    if (append_mem > 0)
                    {
                        RecordMem(id, 1, append_mem);
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
        RecordCpu(kInnerMergeCost, clk.stop_and_save().cost());
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputCpu(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank)
    {
        if (name == NULL  || name_len + name_blank > kProfDescMaxSize)
        {
            return -10;
        }
        double cpu_rate = particle_for_ns(node.traits.clk);
        zprof::Clock<> single_line_cost;
        single_line_cost.start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushBlank(name_blank);
        rp.PushString(UNWIND_STR(" |"));

        rp.PushString(UNWIND_STR("\tcpu*|-- "));
        if (true)
        {
            rp.PushHumanCount(node.cpu.c);
            rp.PushString(UNWIND_STR("c, "));
            rp.PushHumanTime((long long)(node.cpu.sum * cpu_rate / node.cpu.c));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.sum * cpu_rate));
        }


        if (node.cpu.min_u != LLONG_MAX && node.cpu.max_u > 0)
        {
            rp.PushString(UNWIND_STR(" --|\tmax-min:|-- "));
            rp.PushHumanTime((long long)(node.cpu.max_u * cpu_rate));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.min_u * cpu_rate));
        }


        if (node.cpu.dv > 0 || node.cpu.sm > 0)
        {
            rp.PushString(UNWIND_STR(" --|\tdv-sm:|-- "));
            rp.PushHumanTime((long long)(node.cpu.dv * cpu_rate / node.cpu.c));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.sm * cpu_rate));
        }


        if (node.cpu.h_sm > 0 || node.cpu.l_sm > 0)
        {
            rp.PushString(UNWIND_STR(" --|\th-l:|-- "));
            rp.PushHumanTime((long long)(node.cpu.h_sm * cpu_rate));
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanTime((long long)(node.cpu.l_sm * cpu_rate));
        }
        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());

        single_line_cost.start();
        OutputAndClean(rp);
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());

        return 0;
    }


    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputMem(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank)
    {
        if (name == NULL || name_len + name_blank > kProfDescMaxSize)
        {
            return -20;
        }
        zprof::Clock<> single_line_cost;
        single_line_cost.start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushBlank(name_blank);
        rp.PushString(UNWIND_STR(" |"));

        rp.PushString(UNWIND_STR("\tmem*|-- "));
        if (true)
        {
            rp.PushHumanCount(node.mem.c);
            rp.PushString(UNWIND_STR("c, "));
            rp.PushHumanMem(node.mem.sum / node.mem.c);
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanMem(node.mem.sum);
        }

        rp.PushString(UNWIND_STR(" --||-- "));
        if (node.mem.delta > 0)
        {
            rp.PushHumanMem(node.mem.sum - node.mem.delta);
            rp.PushString(UNWIND_STR(", "));
            rp.PushHumanMem(node.mem.delta);
        }
        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());


        single_line_cost.start();
        OutputAndClean(rp);
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());
        return 0;
    }
    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputVm(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank)
    {
        if (name == NULL || name_len + name_blank > kProfDescMaxSize)
        {
            return -30;
        }
        zprof::Clock<> single_line_cost;
        single_line_cost.start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushBlank(name_blank);
        rp.PushString(UNWIND_STR(" |"));


        rp.PushString(UNWIND_STR("\t vm*|-- "));
        if (true)
        {
            rp.PushHumanMem(node.vm.vm_size);
            rp.PushString(UNWIND_STR("(vm), "));
            rp.PushHumanMem(node.vm.rss_size);
            rp.PushString(UNWIND_STR("(rss), "));
            rp.PushHumanMem(node.vm.shr_size);
            rp.PushString(UNWIND_STR("(shr), "));
            rp.PushHumanMem(node.vm.rss_size - node.vm.shr_size);
            rp.PushString(UNWIND_STR("(uss)"));
        }

        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());

        single_line_cost.start();
        OutputAndClean(rp);
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());
        return 0;
    }
    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputUser(RecordNode& node, Report& rp, int entry_idx, int depth, const char* name, int name_len, int name_blank)
    {
        if (name == NULL || name_len + name_blank > kProfDescMaxSize)
        {
            return -40;
        }
        zprof::Clock<> single_line_cost;
        single_line_cost.start();
        rp.PushIndent(depth * 2);
        rp.PushString(UNWIND_STR("|"));
        rp.PushNumber((unsigned long long)entry_idx, 3);
        rp.PushString(UNWIND_STR("| "));
        rp.PushString(name, name_len);
        rp.PushBlank(name_blank);
        rp.PushString(UNWIND_STR(" |"));


        rp.PushString(UNWIND_STR("\tuser*|-- "));
        if (true)
        {
            rp.PushHumanCount(node.user.param1);
            rp.PushString(UNWIND_STR(" \t/ "));
            rp.PushHumanCount(node.user.param2);
            rp.PushString(UNWIND_STR(" \t/ "));
            rp.PushHumanCount(node.user.param3);
            rp.PushString(UNWIND_STR(" \t/ "));
            rp.PushHumanCount(node.user.param4);
        }

        rp.PushString(UNWIND_STR(" --|"));
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerSerializeCost, single_line_cost.cost());

        single_line_cost.start();
        OutputAndClean(rp);
        single_line_cost.stop_and_save();
        RecordCpuFull(kInnerOutputCost, single_line_cost.cost());
        return 0;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::RecursiveOutput(int entry_idx, int depth, const char* opt_name, int opt_name_len, Report& rp)
    {
        if (entry_idx >= end_id())
        {
            return -1;
        }

        if (rp.buff_len() <= kProfLineMinSize)
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
        if (node.traits.clk >= kClockMAX)
        {
            return 0;
        }


    
        zprof::Clock<> single_line_cost;

        const char* name = &compact_data_[node.traits.name];
        int name_len = node.traits.name_len;
        
        if (opt_name != NULL)
        {
            name = opt_name;
            name_len = opt_name_len;
        }

        int name_blank = (int)name_len + depth  + depth;
        name_blank = name_blank < kRecordFormatAlignSize ? kRecordFormatAlignSize - name_blank : 0;

        if (name_len + name_blank > kProfDescMaxSize)
        {
            return -5;
        }

        rp.reset_offset();


        if (node.cpu.c > 0)
        {
            OutputCpu(node, rp, entry_idx, depth, name, name_len, name_blank);
        }

        if (node.mem.c > 0)
        {
            OutputMem(node, rp, entry_idx, depth, name, name_len, name_blank);
        }

        if (node.vm.rss_size + node.vm.vm_size > 0)
        {
            OutputVm(node, rp, entry_idx, depth, name, name_len, name_blank);
        }

        if (node.user.param1 != 0 || node.user.param2 != 0 || node.user.param3 != 0 || node.user.param4 != 0)
        {
            OutputUser(node, rp, entry_idx, depth, name, name_len, name_blank);
        }

        if (depth > kProfMaxDepth)
        {
            rp.PushIndent(depth * 2);
            OutputAndClean(rp);
            return -4;
        }

        for (int i = node.show.child; i < node.show.child + node.show.window; i++)
        {
            RecordNode& child = nodes_[i];
            if (child.show.upper == entry_idx)
            {
                int ret = RecursiveOutput(i, depth + 1, NULL, 0, rp);
                if (ret < 0)
                {
                    return ret;
                }
            }
        }
        return 0;
    }






    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputOneRecord(int entry_idx)
    {
        StaticReport rp;
        int ret = RecursiveOutput(entry_idx, 0, NULL, 0, rp);
        (void)ret;
        return ret;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputTempRecord(const char* opt_name, int opt_name_len)
    {
        StaticReport rp;
        int ret = RecursiveOutput(0, 0, opt_name, opt_name_len, rp);
        ResetNode(0);//reset  
        return ret;
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputTempRecord(const char* opt_name)
    {
        return OutputTempRecord(opt_name, (int)strlen(opt_name));
    }

    template<int kInst, int kReserve, int kDeclare>
    int Record<kInst, kReserve, kDeclare>::OutputReport(unsigned int flags)
    {
        if (output_ == nullptr)
        {
            return -1;
        }
        RerecordUser(kInnerOutputTs, zprof::Clock<>::SystemNowMS());

        zprof::Clock<> clk;
        clk.start();
        StaticReport rp;

        rp.reset_offset();
        OutputAndClean(rp);


        rp.PushChar('=', 30);
        rp.PushChar(' ');
        rp.PushString(Title());
        rp.PushString(UNWIND_STR(" output report at: "));
        rp.PushNowDate();
        rp.PushString(UNWIND_STR(" dist start time:["));
        rp.PushHumanTime((Clock<>::SystemNowMS() - nodes_[kInnerInitTs].user.param1)*1000*1000);
        rp.PushString(UNWIND_STR("] dist reset time:["));
        rp.PushHumanTime((Clock<>::SystemNowMS() - nodes_[kInnerResetTs].user.param1) * 1000 * 1000);
        rp.PushChar(']');
        rp.PushChar(' ');

        rp.PushChar('=', 30);
        OutputAndClean(rp);

        rp.PushString(UNWIND_STR("| -- index -- | ---    cpu  ------------ | ----------   hits, avg, sum   ---------- | ---- max, min ---- | ------ dv, sm ------ |  --- hsm, lsm --- | "));
        OutputAndClean(rp);
        rp.PushString(UNWIND_STR("| -- index -- | ---    mem  ---------- | ----------   hits, avg, sum   ---------- | ------ last, delta ------ | "));
        OutputAndClean(rp);
        rp.PushString(UNWIND_STR("| -- index -- | ---    vm  ------------ | ----------   vm, rss, shr, uss   ------------------ | " ));
        OutputAndClean(rp);

        rp.PushString(UNWIND_STR("| -- index -- | ---    user  ----------- | -----------  hits, avg, sum   ---------- | "));
        OutputAndClean(rp);

        if (flags & kOutFlagInner)
        {
            rp.PushString(UNWIND_STR(PROF_LINE_FEED));
            for (int i = kInnerNull + 1; i < kInnerMax; i++)
            {
                int ret = RecursiveOutput(i, 0, NULL, 0, rp);
                (void)ret;
            }
        }

        if (flags & kOutFlagReserve)
        {
            rp.PushString(UNWIND_STR(PROF_LINE_FEED));
            for (int i = reserve_begin_id(); i < reserve_end_id(); i++)
            {
                int ret = RecursiveOutput(i, 0, NULL, 0, rp);
                (void)ret;
            }
        }
    
        if (flags & kOutFlagDelcare)
        {
            rp.PushString(UNWIND_STR(PROF_LINE_FEED));
            for (int i = declare_begin_id(); i < declare_window(); )
            {
                int ret = RecursiveOutput(i, 0, NULL, 0, rp);
                (void)ret;
                i += nodes_[i].show.jumps + 1;
            }
        }

        rp.reset_offset();
        rp.PushChar('=', 30);
        rp.PushChar('\t');
        rp.PushString(" end : ");
        rp.PushNowDate();
        rp.PushChar('\t');
        rp.PushChar('=', 30);
        OutputAndClean(rp);
        OutputAndClean(rp);

        RecordCpu(kInnerReportCost, clk.stop_and_save().cost());
        return 0;
    }

}

#endif
