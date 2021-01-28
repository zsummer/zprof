
/*
* zperf License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
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

#include "zperf.h"

void entry_cpu_test()
{
    PerfTime entry_use;
    PerfInst.regist_node(0, "entry", false);
    PerfInst.regist_node(1, "perf_now_ns", false);
    double sum = 0.0;
    PerfTime get_now_use;
    for (size_t i = 0; i < 10000000; i++)
    {
        get_now_use.begin_tick();
        sum += perf_now_ns();
        PerfInst.call_cpu(1, 1, get_now_use.end_tick().duration(), 0);
    }
    PerfInst.call_cpu(0, 1, entry_use.end_tick().duration(), 0);
}

void entry_mem_test()
{

    PerfInst.regist_node(5, "entry", false);
    PerfInst.regist_node(6, "alloc", false);
    PerfInst.regist_node(7, "free", false);
    PerfTime use;
    PerfTime alloc_use;
    for (size_t i = 0; i < 10000; i++)
    {
        alloc_use.begin_tick();
        char* ptr = new char[10];
        PerfInst.call_mem(6, 1, 10);
        PerfInst.call_cpu(6, 1, alloc_use.end_tick().duration(), 10);
        alloc_use.begin_tick();
        delete[] ptr;
        PerfInst.call_mem(7, 1, 10);
        PerfInst.call_cpu(7, 1, alloc_use.end_tick().duration(), 10);
    }
    PerfInst.call_cpu(5, 1, use.end_tick().duration(), 0);
}
