/*
* breeze License
* Copyright (C) 2014-2017 YaweiZhang <yawei.zhang@foxmail.com>.
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
#include "zperf.h"
#include "test.h"
                         

int main(int argc, char *argv[])
{
    PerfTime log_start_use;
    FNLog::FastStartDebugLogger();
    LogDebug() << " main begin test. use time:" << log_start_use.end_tick().duration_second();

    PerfTime get_time_use;
    double time = 0.0f;
    for (size_t i = 0; i < 10000000; i++)
    {
        time += perf_now_ns();
    }
    LogDebug() << "get now time bat 10,000,000  use:" << get_time_use.end_tick().duration_second() <<" s, sum test val:" << time;

    double d = 0.0;
    for (size_t i = 0; i < 10000000; i++)
    {
        get_time_use.begin_tick();
        time += perf_now_ns();
        d += get_time_use.end_tick().duration();
    }
    LogDebug() << "get now time 10,000,000 sum use:" << d /1000.0/1000.0/1000.0 << ", sum test val:" << time;



    entry_cpu_test();
    PerfTime get_now_use;
    for (size_t i = 0; i < 10000000; i++)
    {
        get_now_use.begin_tick();
        time += perf_now_ns();
        PerfInst.call_cpu(1, 1, get_now_use.end_tick().duration(), 0);
    }

    LogDebug() << "get now time 10,000,000 sum use:" << PerfInst.node(1).cpu.call_use_time /1000.0/1000.0/1000.0 << ", sum test val:" << time;
    LogDebug() << "perf record node size:" << sizeof(PerfInst) << "bytes";


    
    PerfInst.add_node_child(0, 1);


    PerfInst.regist_node(5, "entry", false);
    PerfInst.regist_node(6, "alloc", false);
    PerfInst.regist_node(7, "free", false);
    PerfInst.add_node_child(5, 6);
    PerfInst.add_node_child(5, 7);
    PerfInst.regist_node(8, "empty", false);
    PerfInst.add_node_child(8, 5);

    entry_mem_test();
    PerfInst.call_mem(8, 1, perf_self_memory_use());
    for (int i = 0; i < PerfInst.node_count(); i++)
    {
        if (PerfInst.node(i).active && !PerfInst.node(i).is_child)
        {
            LogDebug() << PerfInst.serialize(i);
        }
    }

    PerfInst.reset_childs(8);
    entry_mem_test();
    PerfInst.call_mem(8, 1, perf_self_memory_use());
    for (int i = 0; i < PerfInst.node_count(); i++)
    {
        if (PerfInst.node(i).active && !PerfInst.node(i).is_child)
        {
            LogDebug() << PerfInst.serialize(i);
        }
    }


    PerfTime sleep_use;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    LogDebug() << "sleep 300ms use:" << sleep_use.end_tick().duration_second()  ;



    



    LogInfo() << "all test finish .";
    return 0;
}


