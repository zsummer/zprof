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

    
    //单独使用性能计数器  
    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_NOFENCE> counter;
        //record empty cost 
        counter.start();
        counter.stop_and_save();
        long long empty_cost = counter.cycles();
        long long empty_cost_ns = counter.duration_ns();
        empty_cost = counter.cycles();
        empty_cost_ns = counter.duration_ns();

        //all in L1 
        volatile int val[] = { 54321, 12345, 0, rand() % 2 };

        //get ternary operator cost 
        counter.start();
        val[2] = val[3] == 0 ? val[0] : val[1];
        val[1] = val[2] == 0 ? val[0] : val[1];
        counter.stop_and_save();

        LogInfo() << "no fence rdtsc: empty cost:" << empty_cost << "cycles," << empty_cost_ns << "ns,"
            << "\t\tternary operator:" << counter.cycles() - empty_cost << "cycles, " << counter.duration_ns() - empty_cost_ns << "ns." <<"salt:" << val[2];
    }

    //单独使用性能计数器  
    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC> counter;
        //record empty cost 
        counter.start();
        counter.stop_and_save();
        long long empty_cost = counter.cycles();
        long long empty_cost_ns = counter.duration_ns();
        empty_cost = counter.cycles();
        empty_cost_ns = counter.duration_ns();
        //all in L1 
        volatile int val[] = { 54321, 12345, 0, rand() % 2 };

        //get ternary operator cost 
        counter.start();
        val[2] = val[3] == 0 ? val[0] : val[1];
        val[1] = val[2] == 0 ? val[0] : val[1];
        counter.stop_and_save();

        LogInfo() << "load fence rdtsc: empty cost:" << empty_cost << "cycles," << empty_cost_ns << "ns,"
            << "\t\tternary operator:" << counter.cycles() - empty_cost << "cycles, " << counter.duration_ns() - empty_cost_ns << "ns." << "salt:" << val[2];
    }

    //单独使用性能计数器  
    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> counter;
        //record empty cost 
        counter.start();
        counter.stop_and_save();
        long long empty_cost = counter.cycles();
        long long empty_cost_ns = counter.duration_ns();
        empty_cost = counter.cycles();
        empty_cost_ns = counter.duration_ns();
        //all in L1 
        volatile int val[] = { 54321, 12345, 0, rand() % 2 };

        //get ternary operator cost 
        counter.start();
        val[2] = val[3] == 0 ? val[0] : val[1];
        val[1] = val[2] == 0 ? val[0] : val[1];
        counter.stop_and_save();

        LogInfo() << "load fence btb rdtsc: empty cost:" << empty_cost << "cycles," << empty_cost_ns << "ns,"
            << "\t\tternary operator:" << counter.cycles() - empty_cost << "cycles, " << counter.duration_ns() - empty_cost_ns << "ns." << "salt:" << val[2];
    }

    //单独使用性能计数器  
    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_MFENCE> counter;
        //record empty cost 
        counter.start();
        counter.stop_and_save();
        long long empty_cost = counter.cycles();
        long long empty_cost_ns = counter.duration_ns();
        empty_cost = counter.cycles();
        empty_cost_ns = counter.duration_ns();
        //all in L1 
        volatile int val[] = { 54321, 12345, 0, rand() % 2 };

        //get ternary operator cost 
        counter.start();
        val[2] = val[3] == 0 ? val[0] : val[1];
        val[1] = val[2] == 0 ? val[0] : val[1];
        counter.stop_and_save();

        LogInfo() << "load&store fence rdtsc: empty cost:" << empty_cost << "cycles," << empty_cost_ns << "ns,"
            << "\t\tternary operator:" << counter.cycles() - empty_cost << "cycles, " << counter.duration_ns() - empty_cost_ns << "ns." << "salt:" << val[2];
    }

    //单独使用性能计数器  
    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_MFENCE_BTB> counter;
        //record empty cost 
        counter.start();
        counter.stop_and_save();
        long long empty_cost = counter.cycles();
        long long empty_cost_ns = counter.duration_ns();
        empty_cost = counter.cycles();
        empty_cost_ns = counter.duration_ns();
        //all in L1 
        volatile int val[] = { 54321, 12345, 0, rand() % 2 };

        //get ternary operator cost 
        counter.start();
        val[2] = val[3] == 0 ? val[0] : val[1];
        val[1] = val[2] == 0 ? val[0] : val[1];
        counter.stop_and_save();

        LogInfo() << "load&store fence back to back rdtsc: empty cost:" << empty_cost << "cycles," << empty_cost_ns << "ns,"
            << "\t\tternary operator:" << counter.cycles() - empty_cost << "cycles, " << counter.duration_ns() - empty_cost_ns << "ns." << "salt:" << val[2];
    }

    //单独使用性能计数器  
    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_LOCK> counter;
        //record empty cost 
        counter.start();
        counter.stop_and_save();
        long long empty_cost = counter.cycles();
        long long empty_cost_ns = counter.duration_ns();
        empty_cost = counter.cycles();
        empty_cost_ns = counter.duration_ns();

        //all in L1 
        volatile int val[] = { 54321, 12345, 0, rand() % 2 };

        //get ternary operator cost 
        counter.start();
        val[2] = val[3] == 0 ? val[0] : val[1];
        val[1] = val[2] == 0 ? val[0] : val[1];
        counter.stop_and_save();

        LogInfo() << "lock rdtsc: empty cost:" << empty_cost << "cycles," << empty_cost_ns << "ns,"
            << "\t\tternary operator:" << counter.cycles() - empty_cost << "cycles, " << counter.duration_ns() - empty_cost_ns << "ns." << "salt:" << val[2];
    }



    return 0;
}


