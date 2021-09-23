
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


#include "zprof_record.h"

#ifndef ZPROF_H
#define ZPROF_H

#ifndef PROF_DEFAULT_INST_ID 
#define PROF_DEFAULT_INST_ID 0
#endif

#ifndef PROF_RESERVE_COUNT
#define PROF_RESERVE_COUNT 50
#endif 


#ifndef PROF_DECLARE_COUNT
#define PROF_DECLARE_COUNT 260
#endif 

#ifndef PROF_ANON_COUNT
#define PROF_ANON_COUNT 200
#endif


#define ProfInstType ProfRecord<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT, PROF_ANON_COUNT>
#define ProfInst ProfInstType::instance()



template<bool IS_BAT, ProfLevel PROF_LEVEL>
inline void ProfRecordWrap(int idx, long long count, long long cost)
{

}


template<>
inline void ProfRecordWrap<true, PROF_LEVEL_NORMAL>(int idx, long long count, long long cost)
{
    ProfInst.call_cpu(idx, count, cost);
}

template<>
inline void ProfRecordWrap<false, PROF_LEVEL_NORMAL>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.call_cpu(idx, cost);
}
template<>
inline void ProfRecordWrap<true, PROF_LEVEL_FAST>(int idx, long long count, long long cost)
{
    ProfInst.call_cpu_no_sm(idx, count, cost);
}
template<>
inline void ProfRecordWrap<false, PROF_LEVEL_FAST>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.call_cpu_no_sm(idx, cost);
}

template<>
inline void ProfRecordWrap<true, PROF_LEVEL_FULL>(int idx, long long count, long long cost)
{
    ProfInst.call_cpu_full(idx, count, cost);
}
template<>
inline void ProfRecordWrap<false, PROF_LEVEL_FULL>(int idx, long long count, long long cost)
{
    (void)count;
    ProfInst.call_cpu_full(idx, cost);
}

template<long long COUNT>
struct ProfCountIsGreatOne
{
    static const bool is_bat = COUNT > 1;
};


template <long long COUNT = 1, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoRecord
{
public:
    ProfAutoRecord(int idx)
    {
        idx_ = idx;
        counter_.start();
    }
    ~ProfAutoRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>(idx_, COUNT, counter_.save().cycles());
    }
    ProfCounter<C>& counter() { return counter_; }
private:
    ProfCounter<C> counter_;
    int idx_;
};



template <ProfCounterType T = PROF_COUNTER_DEFAULT>
class ProfRegister
{
public:
    ProfRegister(const char* desc)
    {
        this_id_ = ProfInst.new_anon_node_id();
        ProfInst.regist_node(this_id_, desc, T, true, false);
    }

    ~ProfRegister()
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

    template <long long COUNT = 1LL, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL>
    void record_current()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL >(this_id_, COUNT, counter_.save().cycles());
    }

    void record_mem(long long mem)
    {
        ProfInst.call_mem(this_id_, 1, mem);
    }
    void refresh_mem(long long mem)
    {
        ProfInst.refresh_mem(this_id_, 1, mem);
    }
    void call_vm(const ProfVM& vm)
    {
        ProfInst.call_vm(this_id_, vm);
    }
    int node_id() { return this_id_; }
    ProfCounter<T>& counter() { return counter_; }

private:
    int this_id_;
    ProfCounter<T> counter_;
};

template <long long COUNT = 1LL, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoSingleRecord
{
public:
    ProfAutoSingleRecord(const char* desc):reg_(desc)
    {
        reg_.start();
    }
    ~ProfAutoSingleRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>(reg_.node_id(), COUNT, reg_.counter().save().cycles());
    }

    ProfRegister<C>& reg() { return reg_; }
private:
    ProfRegister<C> reg_;
};





#ifdef OPEN_ZPROF

#define PROF_REGIST_NODE(id, name, c, resident, re_reg)  ProfInst.regist_node(id, name, c, resident, re_reg)
#define PROF_FAST_REGIST_NODE(id)  ProfInst.regist_node(id, #id, PROF_COUNTER_DEFAULT,  false, false)
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.regist_node(id, name, PROF_COUNTER_DEFAULT,  false, false)
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.regist_node(id, #id, PROF_COUNTER_DEFAULT,  true, false)
#define PROF_BIND_CHILD(id, cid)  ProfInst.bind_childs(id, cid)
#define PROF_BIND_MERGE(id, cid) ProfInst.bind_merge(cid, id)
#define PROF_BIND_CHILD_AND_MERGE(id, cid) do {PROF_BIND_CHILD(id, cid); PROF_BIND_MERGE(id, cid); }while(0)




#define PROF_INIT(desc) ProfInst.init_prof(desc)
#define PROF_INIT_JUMP_COUNT() ProfInst.init_jump_count()
#define PROF_RESET_CHILD(idx) ProfInst.reset_childs(idx)
#define PROF_UPDATE_MERGE() ProfInst.update_merge()
#define PROF_CLEAN_RESERVE() ProfInst.clean_reserve_info()
#define PROF_CLEAN_DECLARE() ProfInst.clean_declare_info()
#define PROF_CLEAN_ANON() ProfInst.clean_anon_info()

#define PROF_CALL_CPU_SAMPLE(idx, cost) ProfInst.call_cpu(idx, cost)
#define PROF_CALL_CPU_WRAP(idx, COUNT, cost, PROF_LEVEL)  \
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>((int)(idx), (long long)(COUNT), (long long)cost)
#define PROF_CALL_CPU_DYN_WRAP(idx, count, cost, PROF_LEVEL)  \
        ProfRecordWrap<true, PROF_LEVEL>((int)(idx), (long long)(count), (long long)cost)
#define PROF_CALL_CPU(idx, cost) PROF_CALL_CPU_WRAP((idx), 1, (cost), PROF_LEVEL_NORMAL)
#define PROF_CALL_MEM(idx, count, mem) ProfInst.call_mem(idx, count, mem)
#define PROF_CALL_VM(idx, vm) ProfInst.call_vm(idx, vm)
#define PROF_REFRESH_MEM(idx, count, mem) ProfInst.refresh_mem(idx, count, mem)
#define PROF_CALL_TIMER(idx, stamp) ProfInst.call_timer(idx, stamp)
#define PROF_CALL_USER(idx, count, add) ProfInst.call_user(idx, count, add)


#define PROF_DEFINE_COUNTER(c)  ProfCounter<> c
#define PROF_DEFINE_COUNTER_INIT(tc, start)  ProfCounter<> tc(start)
#define PROF_START_COUNTER(c) c.start()
#define PROF_RESTART_COUNTER(c) c.start()
#define PROF_STOP_AND_SAVE_COUNTER(c) c.stop_and_save()
#define PROF_STOP_AND_RECORD(idx, c) PROF_CALL_CPU_WRAP((idx), 1, (c).stop_and_save().cycles(), PROF_LEVEL_NORMAL)

#define PROF_DEFINE_AUTO_RECORD(c, idx) ProfAutoRecord<> c(idx)


#define PROF_DEFINE_REGISTER(reg, desc, counter) ProfRegister<counter> reg(desc);  
#define PROF_DEFINE_REGISTER_DEFAULT(reg, desc) ProfRegister<> reg(desc);  
#define PROF_REGISTER_START(reg) reg.start()
#define PROF_REGISTER_RECORD(reg) reg.record_current()
#define PROF_REGISTER_RECORD_WRAP(reg, COUNT, PROF_LEVEL) reg.record_current<COUNT, PROF_LEVEL>()
#define PROF_REGISTER_REC_MEM(reg, add) reg.record_mem(add)
#define PROF_REGISTER_REFRESH_MEM(reg, add) reg.refresh_mem(add)
#define PROF_REGISTER_REFRESH_VM(reg, vm) reg.call_vm(vm)


#define PROF_DEFINE_AUTO_SINGLE_RECORD(rec, COUNT, PROF_LEVEL, desc) ProfAutoSingleRecord<COUNT, PROF_LEVEL, PROF_COUNTER_DEFAULT> rec(desc)
#define PROF_DEFINE_AUTO_RECORD_SELF_MEM(desc) do{ ProfRegister<> __temp_prof_record_mem__(desc); PROF_CALL_VM(__temp_prof_record_mem__.node_id(), prof_get_mem_use()); }while(0)




#else
#define PROF_REGIST_NODE(id, name, pt, resident, force)
#define PROF_FAST_REGIST_NODE(id) 
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  
#define PROF_BIND_CHILD(id, cid) 
#define PROF_BIND_MERGE(id, cid) 
#define PROF_BIND_CHILD_AND_MERGE(id, cid) 

#define PROF_INIT(desc) 
#define PROF_INIT_JUMP_COUNT()
#define PROF_RESET_CHILD(idx) 
#define PROF_UPDATE_MERGE() 
#define PROF_RESET_RESERVE()
#define PROF_RESET_DECLARE() 
#define PROF_RESET_ANON() 

#define PROF_CALL_CPU_SAMPLE(idx, cost) 
#define PROF_CALL_CPU(idx, cost) 
#define PROF_CALL_CPU_WRAP(idx, COUNT, cost, PROF_LEVEL) 
#define PROF_CALL_CPU_DYN_WRAP(idx, count, cost, PROF_LEVEL)
#define PROF_CALL_MEM(idx, count, mem) 
#define PROF_CALL_VM(idx, vm) 
#define PROF_REFRESH_MEM(idx, count, mem) 
#define PROF_CALL_TIMER(idx, stamp) 
#define PROF_CALL_USER(idx, count, add)

#define PROF_DEFINE_COUNTER(c)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  
#define PROF_START_COUNTER(c) 
#define PROF_RESTART_COUNTER(c) 
#define PROF_STOP_AND_SAVE_COUNTER(c) 
#define PROF_STOP_AND_RECORD(idx, c) 

#define PROF_DEFINE_AUTO_RECORD(c, idx) 


#define PROF_DEFINE_REGISTER(reg, desc, counter) 
#define PROF_DEFINE_REGISTER_DEFAULT(reg, desc) 
#define PROF_REGISTER_START(reg) 
#define PROF_REGISTER_RECORD(reg) 
#define PROF_REGISTER_RECORD_WRAP(reg, COUNT, PROF_LEVEL) 
#define PROF_REGISTER_REC_MEM(reg, add) 
#define PROF_REGISTER_REFRESH_MEM(reg, add) 
#define PROF_REGISTER_REFRESH_VM(reg, add) 


#define PROF_DEFINE_AUTO_SINGLE_RECORD(rec, COUNT, PROF_LEVEL, desc) 
#define PROF_DEFINE_AUTO_RECORD_SELF_MEM(desc) 

#endif




#define PROF_SERIALIZE_FN_LOG()    ProfInst.serialize(0xff, [](const ProfSerializeBuffer& buffer) \
                {LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(buffer.buff(), (int)buffer.offset()); })




#endif
