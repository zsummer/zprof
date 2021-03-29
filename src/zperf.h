
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


#include "zperf_record.h"

#ifndef ZPERF_H
#define ZPERF_H

#ifndef PERF_DEFAULT_INST_ID 
#define PERF_DEFAULT_INST_ID 0
#endif

#ifndef PERF_RESERVE_COUNT
#define PERF_RESERVE_COUNT 50
#endif 


#ifndef PERF_DECLARE_COUNT
#define PERF_DECLARE_COUNT 200
#endif 

#ifndef PERF_ANON_COUNT
#define PERF_ANON_COUNT 200
#endif


#define PerfInstType PerfRecord<PERF_DEFAULT_INST_ID, PERF_RESERVE_COUNT, PERF_DECLARE_COUNT, PERF_ANON_COUNT>
#define PerfInst PerfInstType::instance()



template<bool IS_BAT, PerfCPURecType CPU_REC_TYPE>
inline void PerfRecordWrap(int idx, long long count, long long cost)
{

}


template<>
inline void PerfRecordWrap<true, PERF_CPU_NORMAL>(int idx, long long count, long long cost)
{
    PerfInst.call_cpu(idx, count, cost);
}

template<>
inline void PerfRecordWrap<false, PERF_CPU_NORMAL>(int idx, long long count, long long cost)
{
    (void)count;
    PerfInst.call_cpu(idx, cost);
}
template<>
inline void PerfRecordWrap<true, PERF_CPU_FAST>(int idx, long long count, long long cost)
{
    PerfInst.call_cpu_no_sm(idx, count, cost);
}
template<>
inline void PerfRecordWrap<false, PERF_CPU_FAST>(int idx, long long count, long long cost)
{
    (void)count;
    PerfInst.call_cpu_no_sm(idx, cost);
}

template<>
inline void PerfRecordWrap<true, PERF_CPU_FULL>(int idx, long long count, long long cost)
{
    PerfInst.call_cpu_full(idx, count, cost);
}
template<>
inline void PerfRecordWrap<false, PERF_CPU_FULL>(int idx, long long count, long long cost)
{
    (void)count;
    PerfInst.call_cpu_full(idx, cost);
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
        PerfRecordWrap<PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE>(idx_, COUNT, counter_.save().cycles());
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
        this_id_ = PerfInst.new_anon_node_id();
        PerfInst.regist_node(this_id_, desc, T, false);
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
        PerfRecordWrap<PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE >(this_id_, COUNT, counter_.save().cycles());
    }


    void record_mem(long long mem)
    {
        PerfInst.call_mem(this_id_, 1, mem);
    }
    void refresh_mem(long long mem)
    {
        PerfInst.refresh_mem(this_id_, 1, mem);
    }
    int node_id() { return this_id_; }
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
        PerfRecordWrap<PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE>(reg_.node_id(), COUNT, reg_.counter().save().cycles());
    }

    PerfRegister<C>& reg() { return reg_; }
private:
    PerfRegister<C> reg_;
};





#ifdef OPEN_ZPERF

#define PERF_REGIST_NODE(id, name, c, re_reg)  PerfInst.regist_node(id, name, c, re_reg)
#define PERF_FAST_REGIST_NODE(id)  PerfInst.regist_node(id, #id, PERF_COUNTER_DEFAULT, false)
#define PERF_BIND_CHILD(id, cid)  PerfInst.bind_childs(id, cid)
#define PERF_BIND_MERGE(id, cid) PerfInst.bind_merge(cid, id)
#define PERF_BIND_CHILD_AND_MERGE(id, cid) do {PERF_BIND_CHILD(id, cid); PERF_BIND_MERGE(id, cid); }while(0)




#define PERF_INIT(desc) PerfInst.init_perf(desc)
#define PERF_RESET_CHILD(idx) PerfInst.reset_childs(idx)
#define PERF_UPDATE_MERGE() PerfInst.update_merge()
#define PERF_RESET_DECLARE() PerfInst.reset_declare_info()
#define PERF_RESET_ANON() PerfInst.reset_anon_info()

#define PERF_CALL_CPU_SAMPLE(idx, cost) PerfInst.call_cpu(idx, cost)
#define PERF_CALL_CPU_WRAP(idx, COUNT, cost, CPU_REC_TYPE)  \
        PerfRecordWrap<PerfCountIsGreatOne<COUNT>::is_bat, CPU_REC_TYPE>((int)(idx), (long long)(COUNT), (long long)cost)
#define PERF_CALL_CPU(idx, cost) PERF_CALL_CPU_WRAP((idx), 1, (cost), PERF_CPU_NORMAL)
#define PERF_CALL_MEM(idx, count, mem) PerfInst.call_mem(idx, count, mem)
#define PERF_REFRESH_MEM(idx, count, mem) PerfInst.refresh_mem(idx, count, mem)
#define PERF_CALL_TIMER(idx, stamp) PerfInst.call_timer(idx, stamp)
#define PERF_CALL_USER(idx, count, add) PerfInst.call_user(idx, count, add)


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
#define PERF_DEFINE_AUTO_RECORD_SELF_MEM(desc) do{ PerfRegister<> __temp_perf_record_mem__(desc); PERF_CALL_MEM(__temp_perf_record_mem__.node_id(), 1, perf_get_mem_use()); }while(0)




#else
#define PERF_REGIST_NODE(id, name, pt, force)
#define PERF_FAST_REGIST_NODE(id) 
#define PERF_BIND_CHILD(id, cid) 
#define PERF_BIND_MERGE(id, cid) 
#define PERF_BIND_CHILD_AND_MERGE(id, cid) 

#define PERF_INIT(desc) 
#define PERF_RESET_CHILD(idx) 
#define PERF_UPDATE_MERGE() 

#define PERF_CALL_CPU_SAMPLE(idx, cost) 
#define PERF_CALL_CPU(idx, cost) 
#define PERF_CALL_CPU_WRAP(idx, COUNT, cost, CPU_REC_TYPE) 
#define PERF_CALL_MEM(idx, count, mem) 
#define PERF_REFRESH_MEM(idx, count, mem) 
#define PERF_CALL_TIMER(idx, stamp) 
#define PERF_CALL_USER(idx, count, add)

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




#define PERF_SERIALIZE_FN_LOG()    PerfInst.serialize([](const PerfSerializeBuffer& buffer) \
                {LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, FNLog::LOG_PREFIX_NULL).write_buffer(buffer.buff(), (int)buffer.offset()); })




#endif
