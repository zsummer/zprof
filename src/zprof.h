
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

//默认的全局实例ID   
#ifndef PROF_DEFAULT_INST_ID 
#define PROF_DEFAULT_INST_ID 0
#endif

//PROF保留条目 用于记录一些通用的环境性能数据   
#ifndef PROF_RESERVE_COUNT
#define PROF_RESERVE_COUNT 50
#endif 

//PROF主要条目 需要先注册名字和层级关联关系  
#ifndef PROF_DECLARE_COUNT
#define PROF_DECLARE_COUNT 260
#endif 




//默认的全局实例定义 如需扩展可更换INST ID使用额外的全局实例      
#define ProfInstType ProfRecord<PROF_DEFAULT_INST_ID, PROF_RESERVE_COUNT, PROF_DECLARE_COUNT>
#define ProfInst ProfInstType::instance()


//包装函数 根据模版参数在编译阶段直接使用不同的入口  从而减少常见使用场景下的运行时判断消耗.  
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


//RAII小函数  
//用于快速记录<注册条目>的性能信息  
template <long long COUNT = 1, ProfLevel PROF_LEVEL = PROF_LEVEL_NORMAL,
    ProfCounterType C = PROF_COUNTER_DEFAULT>
class ProfAutoRecord
{
public:
    //idx为<注册条目>的ID  
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



//RAII小函数  
//一次性记录并直接输出到日志 不需要提前注册任何条目  
//整体性能影响要稍微高于<注册条目>  但消耗部分并不影响记录本身. 使用在常见的一次性流程或者demo场景中.    
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



//接口入口   
//作为profile工具, 以宏的形式提供接口, 可在编译环境中随时关闭.   
//实际生产环境中 该工具对性能的整体影响非常小 可在生产环境正常开启  

#ifdef OPEN_ZPROF   

//--------
// 条目注册和关系绑定   
// -------
//注册条目  
#define PROF_REGIST_NODE(id, name, ct, resident, re_reg)  ProfInst.regist_node(id, name, ct, resident, re_reg)  

//快速注册条目: 提供默认计时方式, 默认该条目不开启常驻模式, 一旦调用clear相关接口该条目记录的信息会被清零.  默认该条目未被注册过 当前为新注册  
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  ProfInst.regist_node(id, name, PROF_COUNTER_DEFAULT,  false, false)

//快速注册条目: 同上, 名字也默认提供 即ID自身    
#define PROF_FAST_REGIST_NODE(id)  PROF_FAST_REGIST_NODE_ALIAS(id, #id)

//快速注册条目: 同上 但是为常驻条目 
#define PROF_FAST_REGIST_RESIDENT_NODE(id)  ProfInst.regist_node(id, #id, PROF_COUNTER_DEFAULT,  true, false)  

//绑定展示层级(父子)关系  
#define PROF_BIND_CHILD(id, cid)  ProfInst.bind_childs(id, cid) 

//绑定合并层级(cid->id)关系  合并关系中按照合并方向 合并的目标在前, 要搜集的在后并保持连续 可以获得性能上的跳点优化(也符合线性思维)    
#define PROF_BIND_MERGE(id, cid) ProfInst.bind_merge(cid, id)   

//通常合并关系和展示层级关系一致 这里同时绑定两者  
#define PROF_BIND_CHILD_AND_MERGE(id, cid) do {PROF_BIND_CHILD(id, cid); PROF_BIND_MERGE(id, cid); }while(0)

//注册子条目并绑定展示层级关系    
#define PROF_REG_AND_BIND_CHILD(id, cid)  do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_CHILD(id, cid); } while(0)   
//注册子条目并绑定合并层级关系  
#define PROF_REG_AND_BIND_MERGE(id, cid) do { PROF_FAST_REGIST_NODE(cid); PROF_BIND_MERGE(id, cid); } while(0)  
//注册子条目并同时绑定展示层级和合并层级  
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) do {PROF_FAST_REGIST_NODE(cid);  PROF_BIND_CHILD_AND_MERGE(id, cid); }while(0)


//--------
// PROF启停和输出     
// -------

//初始化全局实例并启动该实例  
#define PROF_INIT(desc) ProfInst.init_prof(desc)   

//[option] 对注册好的条目进行跳点优化; 不执行则不获得优化  
//放在注册完所有条目后执行, 否则优化只能覆盖执行时已经注册的条目(全量覆写型构建跳点, 无副作用)  
#define PROF_INIT_JUMP_COUNT() ProfInst.init_jump_count()  

//注册输出 默认使用printf  
#define PROF_SET_LOG(log_fun) ProfInst.set_default_log_func(log_fun)

//重置(清零)idx条目以及递归重置其所有子条目  
#define PROF_RESET_CHILD(idx) ProfInst.reset_childs(idx)  

//执行性能数据的层级合并 
//合并层级进行了扁平压缩 
#define PROF_UPDATE_MERGE() ProfInst.update_merge()  

//清零<保留条目>信息(常驻条目除外)  
#define PROF_CLEAN_RESERVE() ProfInst.clean_reserve_info()  
//清零<注册条目>信息(常驻条目除外)  
#define PROF_CLEAN_DECLARE() ProfInst.clean_declare_info()  


//--------
// PROF记录       
// -------

//记录性能消耗信息 平均耗时约为4ns    
#define PROF_CALL_CPU_SAMPLE(idx, cost) ProfInst.call_cpu(idx, cost)   

//记录性能消耗信息(携带总耗时和执行次数) 平均耗时约为6ns      
//COUNT为常数 cost为总耗时, 根据记录等级选择性存储 平滑数据, 抖动偏差 等     ProfLevel:PROF_LEVEL_NORMAL  
#define PROF_CALL_CPU_WRAP(idx, COUNT, cost, PROF_LEVEL)  \
        ProfRecordWrap<ProfCountIsGreatOne<COUNT>::is_bat, PROF_LEVEL>((int)(idx), (long long)(COUNT), (long long)cost)  
//记录性能消耗信息: 同上, 但count非常数  
#define PROF_CALL_CPU_DYN_WRAP(idx, count, cost, PROF_LEVEL)  \
        ProfRecordWrap<true, PROF_LEVEL>((int)(idx), (long long)(count), (long long)cost)

//同PROF_CALL_CPU_SAMPLE  
#define PROF_CALL_CPU(idx, cost) PROF_CALL_CPU_WRAP((idx), 1, (cost), PROF_LEVEL_NORMAL)

//记录内存字节数    
//输出日志时 进行可读性处理 带k,m,g等单位  
#define PROF_CALL_MEM(idx, count, mem) ProfInst.call_mem(idx, count, mem)  

//记录系统内存信息 包含vm, rss等  
#define PROF_CALL_VM(idx, vm) ProfInst.call_vm(idx, vm)
#define PROF_REFRESH_MEM(idx, count, mem) ProfInst.refresh_mem(idx, count, mem)

//记录定时器 比较特殊.  根据调用的前后间隔进行累加  
#define PROF_CALL_TIMER(idx, stamp) ProfInst.call_timer(idx, stamp)  

//记录用户自定义信息 没有额外处理   
#define PROF_CALL_USER(idx, count, add) ProfInst.call_user(idx, count, add)


//-------手动计时器-----------
//定义一个计时器  
#define PROF_DEFINE_COUNTER(var)  ProfCounter<> var

//定义一个带起始时间戳的计时器(通常场景很少用这个)  
#define PROF_DEFINE_COUNTER_INIT(tc, start)  ProfCounter<> tc(start)  

//设置当前时间为 定时器开始时间    
#define PROF_START_COUNTER(var) var.start()   

//重新设置当前时间为 定时器开始时间    
#define PROF_RESTART_COUNTER(var) var.start()   

//设置当前时间为定时器结束时间  
#define PROF_STOP_AND_SAVE_COUNTER(var) var.stop_and_save()  

//设置当前时间为定时器结束时间 并写入idx对应的条目中  
#define PROF_STOP_AND_RECORD(idx, var) PROF_CALL_CPU_WRAP((idx), 1, (var).stop_and_save().cycles(), PROF_LEVEL_NORMAL)



//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时候写入记录条目)-----------
#define PROF_DEFINE_AUTO_RECORD(var, idx) ProfAutoRecord<> var(idx)   



//-------以下非线程安全 使用了全局buffer进行序列化 ------   
 
//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ANON_RECORD(var, desc) ProfAutoAnonRecord<> var(desc)
//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_MULTI_ANON_RECORD(var, count, desc) ProfAutoAnonRecord<count> var(desc)
//-------自动计时器(raii包装, 定义时记录开始时间, 销毁时直接输出性能信息到日志)-----------
#define PROF_DEFINE_AUTO_ADVANCE_ANON_RECORD(var, count, level, ct, desc) ProfAutoAnonRecord<count, level, ct> var(desc)



//使用特殊条目<0>进行一次性输出  
//用于立刻输出性能信息而不是走报告输出  
#define PROF_OUTPUT_DEFAULT_LOG(desc)        ProfSerializeBuffer buffer(ProfInst.serialize_buffer(), ProfInstType::max_serialize_buff_size()); \
                                                              ProfInst.serialize_root(ProfInstType::INNER_PROF_NULL, 0, desc, strlen(desc), buffer, NULL);\
                                                              ProfInst.reset_node(ProfInstType::INNER_PROF_NULL);

#define PROF_OUTPUT_MULTI_COUNT_CPU(desc, count, num)  do {ProfRecordWrap<true, PROF_LEVEL_FAST>((int)ProfInstType::INNER_PROF_NULL, (long long)(count), (long long)num);  PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_USER(desc, count, num) do {PROF_CALL_USER(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_MULTI_COUNT_MEM(desc, count, num) do {PROF_CALL_MEM(ProfInstType::INNER_PROF_NULL, count, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_CPU(desc, num)   do {PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_USER(desc, num) do {PROF_CALL_USER(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)
#define PROF_OUTPUT_SINGLE_MEM(desc, num) do {PROF_CALL_MEM(ProfInstType::INNER_PROF_NULL, 1, num);PROF_OUTPUT_DEFAULT_LOG(desc);} while(0)

//输出当前进程的vm/rss信息 
#define PROF_OUTPUT_SELF_MEM(desc) do{PROF_CALL_VM(ProfInstType::INNER_PROF_NULL, prof_get_mem_use()); PROF_OUTPUT_DEFAULT_LOG(desc);}while(0)



#else
#define PROF_REGIST_NODE(id, name, pt, resident, force)
#define PROF_FAST_REGIST_NODE_ALIAS(id, name)  
#define PROF_FAST_REGIST_NODE(id) 

#define PROF_FAST_REGIST_RESIDENT_NODE(id)  
#define PROF_BIND_CHILD(id, cid) 
#define PROF_BIND_MERGE(id, cid) 
#define PROF_BIND_CHILD_AND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD(id, cid)  
#define PROF_REG_AND_BIND_MERGE(id, cid) 
#define PROF_REG_AND_BIND_CHILD_AND_MERGE(id, cid) 

#define PROF_INIT(desc) 
#define PROF_INIT_JUMP_COUNT()
#define PROF_SET_LOG(log_fun) 

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
