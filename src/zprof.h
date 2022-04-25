
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




#define ProfInstType ProfRecord<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT>
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




template <long long COUNT = 1LL, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoAnonRecord
{
public:
    static const size_t DESC_SIZE = 100;
public:
    ProfAutoAnonRecord(const char* desc)
    {
        strncpy(desc_, desc, DESC_SIZE);
        desc_[DESC_SIZE - 1] = '\0';
        counter_.start();
    }
    ~ProfAutoAnonRecord()
    {
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>(ProfInstType::INNER_PROF_NULL, COUNT, counter_.save().cycles());
        ProfSerializeBuffer buffer(ProfInst.serialize_buffer(), ProfInstType::max_serialize_buff_size()); 
        ProfInst.serialize_root(ProfInstType::INNER_PROF_NULL, 0, desc_, strlen(desc_), buffer, NULL);
        ProfInst.reset_node(ProfInstType::INNER_PROF_NULL);
    }

    ProfCounter<C>& counter() { return counter_; }
private:
    ProfCounter<C> counter_;
    char desc_[DESC_SIZE];
};




#ifdef OPEN_ZPROF

#define PROF_REGIST_NODE(id, name, ct, resident, re_reg)  ProfInst.regist_node(id, name, ct, resident, re_reg)
#define PROF_FAST_REGIST_NODE(id)  ProfInst.regist_node(id, #id, PROF_COUNTER_DEFAULT,  false, false)
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.regist_node(id, name, PROF_COUNTER_DEFAULT,  false, false)
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.regist_node(id, #id, PROF_COUNTER_DEFAULT,  true, false)
#define PROF_BIND_CHILD(id, cid)  ProfInst.bind_childs(id, cid)
#define PROF_BIND_MERGE(id, cid) ProfInst.bind_merge(cid, id)
#define PROF_BIND_CHILD_AND_MERGE(id, cid) do {PROF_BIND_CHILD(id, cid); PROF_BIND_MERGE(id, cid); }while(0)
#define PROF_REG_AND_BIND_CHILD(id, cid)  do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_CHILD(id, cid); } while(0)
#define PROF_REG_AND_BIND_MERGE(id, cid) do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_MERGE(id, cid); } while(0)
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) do {PROF_FAST_REGIST_NODE(cid);  PROF_BIND_CHILD_AND_MERGE(id, cid); }while(0)






#define PROF_INIT(desc) ProfInst.init_prof(desc)
#define PROF_INIT_JUMP_COUNT() ProfInst.init_jump_count()
#define PROF_RESET_CHILD(idx) ProfInst.reset_childs(idx)
#define PROF_UPDATE_MERGE() ProfInst.update_merge()
#define PROF_CLEAN_RESERVE() ProfInst.clean_reserve_info()
#define PROF_CLEAN_DECLARE() ProfInst.clean_declare_info()

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


#define PROF_DEFINE_COUNTER(var)  ProfCounter<> var
#define PROF_DEFINE_COUNTER_INIT(tc, start)  ProfCounter<> tc(start)
#define PROF_START_COUNTER(var) var.start()
#define PROF_RESTART_COUNTER(var) var.start()
#define PROF_STOP_AND_SAVE_COUNTER(var) var.stop_and_save()
#define PROF_STOP_AND_RECORD(idx, var) PROF_CALL_CPU_WRAP((idx), 1, (var).stop_and_save().cycles(), PROF_LEVEL_NORMAL)

#define PROF_DEFINE_AUTO_RECORD(var, idx) ProfAutoRecord<> var(idx)
#define PROF_DEFINE_AUTO_ANON_RECORD(var, desc) ProfAutoAnonRecord<> var(desc)
#define PROF_DEFINE_AUTO_MULTI_ANON_RECORD(var, count, desc) ProfAutoAnonRecord<count> var(desc)
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, count, level, ct, desc) ProfAutoAnonRecord<count, level, ct> var(desc)


#define PROF_OUTPUT_DEFAULT_LOG(desc)        ProfSerializeBuffer buffer(ProfInst.serialize_buffer(), ProfInstType::max_serialize_buff_size()); \
                                                              ProfInst.serialize_root(ProfInstType::INNER_PROF_NULL, 0, desc, strlen(desc), buffer, NULL);\
                                                              ProfInst.reset_node(ProfInstType::INNER_PROF_NULL);

#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  do {ProfRecordWrap<true, PROF_LEVEL_FAST>((int)ProfInstType::INNER_PROF_NULL, (long long)(count), (long long)num);  PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) do {PROF_CALL_USER(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) do {PROF_CALL_MEM(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   do {PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_USER(desc, num) do {PROF_CALL_USER(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) do {PROF_CALL_MEM(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SELF_MEM(desc) do{PROF_CALL_VM(ProfInstType::INNER_PROF_NULL, prof_get_mem_use()); PROF_OUTPUT_DEFAULT_LOG(desc);}while(0)



#else
#define PROF_REGIST_NODE(id, name, pt, resident, force)
#define PROF_FAST_REGIST_NODE(id) 
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  
#define PROF_BIND_CHILD(id, cid) 
#define PROF_BIND_MERGE(id, cid) 
#define PROF_BIND_CHILD_AND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD(id, cid)  
#define PROF_REG_AND_BIND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) 

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

#define PROF_DEFINE_COUNTER(var)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  
#define PROF_START_COUNTER(var) 
#define PROF_RESTART_COUNTER(var) 
#define PROF_STOP_AND_SAVE_COUNTER(var) 
#define PROF_STOP_AND_RECORD(idx, var) 

#define PROF_DEFINE_AUTO_RECORD(var, idx) 
#define PROF_DEFINE_AUTO_ANON_RECORD(desc, idx) 
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, count, level, ct, desc) 

#define PROF_OUTPUT_DEFAULT_LOG(desc) 

#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) 
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) 
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   
#define PROF_OUTPUT_SINGLE_USER(desc, num)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) 
#define PROF_OUTPUT_SELF_MEM(desc) 

#endif




#define PROF_SERIALIZE_FN_LOG()    ProfInst.serialize(0xff, NULL)






#endif
