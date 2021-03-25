
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

#ifndef PERF_TRACK_COUNT
#define PERF_TRACK_COUNT 200
#endif 

#ifndef PERF_TRACK_DYN_COUNT
#define PERF_TRACK_DYN_COUNT 200
#endif


#define PerfInst PerfRecord<PERF_DEFAULT_INST_ID, PERF_TRACK_COUNT, PERF_TRACK_DYN_COUNT>::instance()






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

#define PERF_REGIST_TRACK(id, name, c, re_reg)  PerfInst.regist_track(id, name, c, re_reg)
#define PERF_FAST_REGIST_TRACK(id)  PerfInst.regist_track(id, #id, PERF_COUNTER_DEFAULT, false)
#define PERF_BIND_CHILD(id, cid)  PerfInst.add_track_child(id, cid)
#define PERF_BIND_MERGE(id, cid) PerfInst.add_merge_to(cid, id)
#define PERF_BIND_CHILD_AND_MERGE(id, cid) do {PERF_BIND_CHILD(id, cid); PERF_BIND_MERGE(id, cid); }while(0)




#define PERF_INIT(desc) PerfInst.init_perf(desc)
#define PERF_RESET_CHILD(idx) PerfInst.reset_childs(idx)
#define PERF_UPDATE_MERGE() PerfInst.update_merge()
#define PERF_CLEAR_DECLARE() PerfInst.reset_declare_info()

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
#define PERF_DEFINE_AUTO_RECORD_SELF_MEM(desc) do{ PerfRegister<> __temp_perf_record_mem__(desc); PERF_CALL_MEM(__temp_perf_record_mem__.track_id(), 1, perf_self_memory_use()); }while(0)




#else
#define PERF_REGIST_TRACK(id, name, pt, force)
#define PERF_FAST_REGIST_TRACK(id) 
#define PERF_BIND_CHILD(id, cid) 
#define PERF_BIND_MERGE(id, cid) 
#define PERF_BIND_CHILD_AND_MERGE(id, cid) 

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
