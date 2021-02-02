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
    PerfInst.init_perf();
    regist_perf();

    if (true)
    {
        PerfDynLineGuard<> guard("start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";

    if (true)
    {
        PerfDynLine<> line("self use mem");
        line.call_mem(perf_self_memory_use());
    }

    if (true)
    {
        PerfDynLine<> line("PerfInst use mem");
        line.call_mem(sizeof(PerfInst));
    }


    double time = 0.0f;
    if (true)
    {
        PerfDynLineGuard<> guard("perf_now_sys bat 1000w", 1000*10000, 0);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_sys();
        }
    }
    if (true)
    {
        PerfDynLine<> dyn_time("perf_now_sys dis 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            dyn_time.begin_track();
            time += perf_now_sys();
            dyn_time.end_track();
        }
    }

    if (true)
    {
        PerfDynLineGuard<> guard("perf_now_clock bat 1000w", 1000 * 10000, 0);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_clock();
        }
    }
    if (true)
    {
        PerfDynLineGuard<> guard("perf_now_rdtscp bat 1000w", 1000 * 10000, 0);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_rdtscp();
        }
    }

    if (true)
    {
        PerfDynLineGuard<> guard("perf_now_sys bat 1000w", 1000 * 10000, 0);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_sys();
        }
    }

    if (true)
    {
        PerfDynLineGuard<PERF_TIME_SYS> guard("sleep 300ms: sys ", 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PerfDynLineGuard<PERF_TIME_RDTSCP> guard("sleep 300ms rdtscp ", 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PerfDynLineGuard<PERF_TIME_CLOCK> guard("sleep 300ms clock ", 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }


    if (true)
    {
        PerfDynLineGuard<> guard("call cpu 1000w", 1, 0);
        for (int i = 0; i < 10000000; i++)
        {
            PerfInst.call_cpu(ENUM_PERF_TEST, 10, 1000, 0);
        }
    }
    if (true)
    {
        PerfDynLineGuard<> guard("call cpu 1000w (without count)", 1, 0);
        for (int i = 0; i < 10000000; i++)
        {
            PerfInst.call_cpu(ENUM_PERF_TEST, 1000, 0);
        }
    }
    if (true)
    {
        PerfDynLineGuard<> guard("call mem 1000w ", 1, 0);
        for (int i = 0; i < 10000000; i++)
        {
            PerfInst.call_mem(ENUM_PERF_TEST, 1, 1000);
        }
    }


    if (true)
    {
        PerfDynLine<> line("call timer 10ms ");
        for (int i = 0; i < 100; i++)
        {
            line.begin_track();
            PERF_CALL_ONCE_TIMER(line.track_id(), line.perf_time().begin());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        line.begin_track();
        PERF_CALL_ONCE_TIMER(line.track_id(), line.perf_time().begin());
    }


    PerfInst.reset_childs(ENUM_ENTRY);
    entry_mem_test();
    PerfInst.update_merge();
    PERF_SERIALIZE_FN_LOG();

    PerfInst.reset_childs(ENUM_ENTRY);
    entry_mem_test();
    PerfInst.update_merge();
    PERF_SERIALIZE_FN_LOG();

    











    LogInfo() << "all test finish .";
    return 0;
}


