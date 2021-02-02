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
                         




int main(int argc, char *argv[])
{
    PERF_INIT();
    regist_perf();

    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";

    if (true)
    {
       PERF_DEFINE_DYN_STAMP(dyn, "self use mem");
       PERF_DYN_CALL_MEM(dyn, perf_self_memory_use());
    }

    if (true)
    {
       PERF_DEFINE_DYN_STAMP(dyn, "PerfInst use mem");
       PERF_DYN_CALL_MEM(dyn, sizeof(PerfInst));
    }


    double time = 0.0f;
    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD_WITH_C(guard, "perf_now_sys bat 1000w", 1000*10000);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_sys();
        }
    }
    if (true)
    {
        PERF_DEFINE_DYN_STAMP(dyn_time, "perf_now_sys dis 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_DYN_BEGIN_STAMP(dyn_time);
            time += perf_now_sys();
            PERF_DYN_END_STAMP(dyn_time);
        }
    }

    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD_WITH_C(guard, "perf_now_clock bat 1000w", 1000 * 10000);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_clock();
        }
    }
    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD_WITH_C(guard, "perf_now_rdtsc bat 1000w", 1000 * 10000);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_rdtsc();
        }
    }

    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD_WITH_C(guard, "perf_now_sys bat 1000w", 1000 * 10000);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            time += perf_now_sys();
        }
    }

    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD(guard, "sleep 300ms: sys ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD(guard, "sleep 300ms rdtscp ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD(guard, "sleep 300ms clock ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }


    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD_WITH_C(guard, "call cpu 1000w", 10000000);
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_CPU_WITH_C(ENUM_PERF_TEST, 10, 1000);
        }
    }
    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD_WITH_C(guard, "call cpu 1000w (without count)", 10000000);
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_CPU(ENUM_PERF_TEST, 1000);
        }
    }
    if (true)
    {
        PERF_DEFINE_DYN_STAMP_GUARD_WITH_C(guard, "call mem 1000w ", 10000000);
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_MEM(ENUM_PERF_TEST, 1000);
        }
    }


 




    if (true)
    {
       PERF_DEFINE_DYN_STAMP(dyn, "call timer 10ms ");
        for (int i = 0; i < 100; i++)
        {
            PERF_DYN_BEGIN_STAMP(dyn);
            PERF_CALL_TIMER(dyn.track_id(), dyn.perf_time().begin());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        PERF_DYN_BEGIN_STAMP(dyn);
        PERF_CALL_TIMER(dyn.track_id(), dyn.perf_time().begin());
    }

    PERF_RESET_CHILD(ENUM_ENTRY);
    entry_mem_test();
    PERF_UPDATE_MERGE();
    PERF_SERIALIZE_FN_LOG();

    PERF_RESET_CHILD(ENUM_ENTRY);
    entry_mem_test();
    PerfInst.update_merge();
    PERF_SERIALIZE_FN_LOG();

    
    if (true)
    {
        PerfTime<PERF_TIME_RDTSC> rdtsc;
        PerfTime<PERF_TIME_SYS> sys;
        PerfTime<PERF_TIME_CLOCK> clock;
        rdtsc.begin_track();
        sys.begin_track();
        clock.begin_track();
        for (int i = 0; i < 1000*10000; i++)
        {
            rdtsc.end_track();
        }
        rdtsc.end_track();
        sys.end_track();
        clock.end_track();
        LogDebug() << "rdtsc stamp use:" << human_time_format(rdtsc.duration_ns()) << ", inner count:" << human_count_format(rdtsc.duration());
        LogDebug() << "sys stamp use:" << human_time_format(sys.duration_ns()) << ", inner count:" << human_count_format(sys.duration());
        LogDebug() << "clock stamp use:" << human_time_format(clock.duration_ns()) << ", inner count:" << human_count_format(clock.duration());

    }
    









    LogInfo() << "all test finish .";
    return 0;
}


