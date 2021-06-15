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

#include "fn_log.h"
#include "test.h"
#include <unordered_map>
#include <unordered_set>
#include <random>
#include "zhash_map.h"
typedef char s8;
typedef unsigned char u8;
typedef short int s16;
typedef unsigned short int u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;
typedef unsigned long pointer;
typedef float f32;

enum MyTestEnum
{
    MY_DECLARE_BEGIN = PerfInstType::node_declare_begin_id(),
    NORMAL_NODE, 

    PARRENT_1,
    CHILD_1,
    CHILD_2,
};


int main(int argc, char *argv[])
{
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1, PERF_CPU_NORMAL, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }
    LogDebug() << " main begin test. ";


    //初始化   
    PERF_INIT("inner perf");

    //动态创建一行记录 并保存当前内存
    PERF_DEFINE_AUTO_RECORD_SELF_MEM("this app used memory");

    //动态创建一行记录 记录当前指令耗时  
    if (true)
    {
        //使用默认计数器记录一段代码耗时  
        //- 定义计数器 
        PERF_DEFINE_COUNTER(cost);
        //- 启动计数器  
        PERF_START_COUNTER(cost);
        //--------------------
        delete new char[1024];
        //--------------------
        //- 结束计数器   
        PERF_STOP_AND_SAVE_COUNTER(cost);
        
        //动态创建一行CPU耗时记录, (counter对应使用的计数器版本)    
        PERF_DEFINE_REGISTER(reg, "new and delete 1k bytes cost", PERF_COUNTER_DEFAULT);

        //并记录原始计数结果
        PERF_CALL_CPU(reg.node_id(), cost.cycles());
    }

    //动态创建一行记录 并统计和记录当前语句段的消耗 效果等同上述拆分步骤(但是会包含整段代码所有用时)      
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(rec, 1, PERF_CPU_NORMAL, "new and delete 1k bytes cost");
        delete new char[1024];
    }

    //静态使用, 需要先定义枚举
    //注册(初始化)记录数据,(建议统一初始化)   
    PERF_REGIST_NODE(NORMAL_NODE, "normal", PERF_COUNTER_RDTSC, false, false);

    //统计数据 
    for(int i=0; i<100; i++)
    {
        //自动统计当前语句段中的CPU消耗  
        PERF_DEFINE_AUTO_RECORD(cost, NORMAL_NODE);
        delete new char[1024];
    }


    //层次化构建记录之间的关系, 并合并子记录数据到父记录上   
 
    //注册  
    PERF_REGIST_NODE(PARRENT_1, "parrent", PERF_COUNTER_RDTSC, false, false);
    PERF_REGIST_NODE(CHILD_1, "CHILD_1", PERF_COUNTER_RDTSC, false, false);
    PERF_REGIST_NODE(CHILD_2, "CHILD_2", PERF_COUNTER_RDTSC, false, false);

    //指定序列化时的父子关系(可多层嵌套)  
    PERF_BIND_CHILD(PARRENT_1, CHILD_1);
    PERF_BIND_CHILD(PARRENT_1, CHILD_2);

    //指定合并关系和合并方向(与父子关系无依赖, 但需要配合merge调用才能发生合并行为)   
    PERF_BIND_MERGE(PARRENT_1, CHILD_1);
    PERF_BIND_MERGE(PARRENT_1, CHILD_2);

    //模拟使用   
    for (int root_tick = 0; root_tick < 100; root_tick++)
    {
        for (int i = 0; i < 100; i++)
        {
            if (rand()%100 > 80)
            {
                PERF_DEFINE_COUNTER(cost);
                PERF_START_COUNTER(cost);
                delete new char[10 * 1024];
                PERF_CALL_CPU_WRAP(CHILD_1, 1, cost.stop_and_save().cycles(), PERF_CPU_FULL);
            }
            else
            {
                PERF_DEFINE_COUNTER(cost);
                PERF_START_COUNTER(cost);
                delete new char[64];
                PERF_CALL_CPU_WRAP(CHILD_2, 1, cost.stop_and_save().cycles(), PERF_CPU_FULL);
            }
        }
        //每帧合并一次 
        PERF_UPDATE_MERGE();
    }

    //序列化打印所有记录  
    PERF_SERIALIZE_FN_LOG();

    //定时清空无指定驻留的数据, 开始新一轮测试
    PERF_CLEAN_DECLARE();



    //单独使用性能计数器  
    if (true)
    {
        PerfCounter<PERF_COUNTER_RDTSC> counter;
        //record empty cost 
        counter.start();
        counter.stop_and_save();
        long long empty_cost = counter.cycles();
        long long empty_cost_ns = counter.duration_ns();
        
        //all in L1 
        volatile int val1 = 54321;
        volatile int val2 = 12345;
        volatile int result = 0;
        int rd = rand()%2;

        //get ternary operator cost 
        counter.start();
        result = rd == 0 ? val1 : val2;
        counter.stop_and_save();
        (void)result;
        
        LogInfo() << "empty cost:" << empty_cost << "cycles," << empty_cost_ns << "ns,"
            << "ternary operator:" << counter.cycles() << "cycles, " << counter.duration_ns() << "ns.";
    }


    return 0;
}


