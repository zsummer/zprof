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

#include "fn_log.h"
#include "zprof.h"
#include <unordered_map>
#include <unordered_set>
#include <random>
 
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
    MY_DECLARE_BEGIN = ProfInstType::node_declare_begin_id(),
    NORMAL_NODE, 

    PARRENT_1,
    CHILD_1,
    CHILD_2,
};

static inline void FNLogFunc(const ProfSerializeBuffer& buffer)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(buffer.buff(), (int)buffer.offset());
}


int main(int argc, char *argv[])
{
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }
    LogDebug() << " main begin test. ";


    //初始化   
    PROF_INIT("inner prof");
    PROF_SET_OUTPUT(&FNLogFunc);

    //记录当前内存
    PROF_OUTPUT_SELF_MEM("this app used memory");

    //动态创建一行记录 记录当前指令耗时  
    if (true)
    {
        //使用默认计数器记录一段代码耗时  
        //- 定义计数器 
        PROF_DEFINE_COUNTER(cost);
        //- 启动计数器  
        PROF_START_COUNTER(cost);
        //--------------------
        delete new char[1024];
        //--------------------
        //- 结束计数器   
        PROF_STOP_AND_SAVE_COUNTER(cost);
        
        //直接输出CPU耗时记录, (counter对应使用的计数器版本)    
        PROF_OUTPUT_SINGLE_CPU("new and delete 1k bytes cost", cost.cycles());
    }

    //创建一行临时记录 并统计和记录当前语句段的消耗 效果等同上述拆分步骤(但是会包含整段代码所有用时)      
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(rec, "new and delete 1k bytes cost");
        delete new char[1024];
    }

    //静态使用, 需要先定义枚举
    //注册(初始化)记录数据,(建议统一初始化)   
    PROF_REGIST_NODE(NORMAL_NODE, "normal", PROF_COUNTER_RDTSC, false, false);

    //统计数据 
    for(int i=0; i<100; i++)
    {
        //自动统计当前语句段中的CPU消耗  
        PROF_DEFINE_AUTO_RECORD(cost, NORMAL_NODE);
        delete new char[1024];
    }


    //层次化构建记录之间的关系, 并合并子记录数据到父记录上   
 
    //注册  
    PROF_REGIST_NODE(PARRENT_1, "parrent", PROF_COUNTER_RDTSC, false, false);
    PROF_REGIST_NODE(CHILD_1, "CHILD_1", PROF_COUNTER_RDTSC, false, false);
    PROF_REGIST_NODE(CHILD_2, "CHILD_2", PROF_COUNTER_RDTSC, false, false);

    //指定序列化时的父子关系(可多层嵌套)  
    PROF_BIND_CHILD(PARRENT_1, CHILD_1);
    PROF_BIND_CHILD(PARRENT_1, CHILD_2);

    //指定合并关系和合并方向(与父子关系无依赖, 但需要配合merge调用才能发生合并行为)   
    PROF_BIND_MERGE(PARRENT_1, CHILD_1);
    PROF_BIND_MERGE(PARRENT_1, CHILD_2);

    //模拟使用   
    for (int root_tick = 0; root_tick < 100; root_tick++)
    {
        for (int i = 0; i < 100; i++)
        {
            if (rand()%100 > 80)
            {
                PROF_DEFINE_COUNTER(cost);
                PROF_START_COUNTER(cost);
                delete new char[10 * 1024];
                PROF_CALL_CPU_WRAP(CHILD_1, 1, cost.stop_and_save().cycles(), PROF_LEVEL_FULL);
            }
            else
            {
                PROF_DEFINE_COUNTER(cost);
                PROF_START_COUNTER(cost);
                delete new char[64];
                PROF_CALL_CPU_WRAP(CHILD_2, 1, cost.stop_and_save().cycles(), PROF_LEVEL_FULL);
            }
        }
        //每帧合并一次 
        PROF_UPDATE_MERGE();
    }

    //序列化打印所有记录  
    PROF_OUTPUT_REPORT();

    //定时清空无指定驻留的数据, 开始新一轮测试
    PROF_CLEAN_DECLARE();


    return 0;
}


