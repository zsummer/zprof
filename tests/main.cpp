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
    LogDebug() << " main begin test. use time:" << human_time_format(log_start_use.end_tick().duration());
    LogDebug() << "perf record node size:" << human_mem_format(sizeof(PerfInst));

    regist_perf();

    PerfTime get_time_use;
    double time = 0.0f;
    for (size_t i = 0; i < 10000000; i++)
    {
        time += perf_now_sys();
    }
    PerfInst.call_cpu(ENUM_BAT, 10000000, get_time_use.end_tick().duration(), 0);

    entry_cpu_test();

    entry_mem_test();
    PerfInst.call_mem(ENUM_EMPTY, 1, perf_self_memory_use());

    PERF_SERIALIZE_FN_LOG();

    PerfInst.reset_childs(ENUM_EMPTY);
    entry_mem_test();
    PerfInst.call_mem(ENUM_EMPTY, 1, perf_self_memory_use());

    PERF_SERIALIZE_FN_LOG();



    PerfTime sleep_use;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    LogDebug() << "sleep 300ms use:" << human_time_format(sleep_use.end_tick().duration_ns())  ;



    



    LogInfo() << "all test finish .";
    return 0;
}


