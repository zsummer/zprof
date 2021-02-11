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
    PERF_INIT("inner perf");
    //PerfInst.init_perf("inner perf");
    regist_perf();

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1, PERF_CPU_NORMAL, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(ot, 1, PERF_CPU_NORMAL, "self use mem");
        PERF_REGISTER_REC_MEM(ot.reg(), perf_self_memory_use());
    }

    if (true)
    {
       PERF_DEFINE_AUTO_SINGLE_RECORD(ot, 1, PERF_CPU_NORMAL, "PerfInst use mem");
       PERF_REGISTER_REC_MEM(ot.reg(), sizeof(PerfInst));
    }


    volatile double cycles = 0.0f;
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_sys bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += perf_tsc_sys();
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_clock bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += perf_tsc_clock();
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_rdtsc(lfence) bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += perf_tsc_rdtsc();
        }
    }

    
    if (false)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_rdtscp bat 1000");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += perf_tsc_rdtscp();
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_rdtsc_nofence bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += perf_tsc_rdtsc_nofence();
        }
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_mfence bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += perf_tsc_mfence();
        }
    }


    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10 * 10000, PERF_CPU_NORMAL, "c clock bat 10w");
        for (size_t i = 0; i < 10 * 10000; i++)
        {
            cycles += clock();
        }
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "c++ system_clock bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += std::chrono::system_clock().now().time_since_epoch().count();
        }
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "c++ steady_clock bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += std::chrono::steady_clock().now().time_since_epoch().count();
        }
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_chrono 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += perf_tsc_chrono();
        }
    }


    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "c time bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += time(NULL);
        }
    }

    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(rec, "perf_tsc_sys dis 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_REGISTER_START(rec);
            cycles += perf_tsc_sys();
            PERF_REGISTER_RECORD(rec);
        }
    }
    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(rec, "perf_tsc_clock dis 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_REGISTER_START(rec);
            cycles += perf_tsc_clock();
            PERF_REGISTER_RECORD(rec);
        }
    }
    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(rec, "perf_tsc_rdtsc dis 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_REGISTER_START(rec);
            cycles += perf_tsc_rdtsc();
            PERF_REGISTER_RECORD(rec);
        }
    }
    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(rec, "perf_tsc_rdtsc dis 1000w | fast default");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_REGISTER_START(rec);
            cycles += perf_tsc_rdtsc();
            PERF_REGISTER_RECORD_WRAP(rec, 1, PERF_CPU_FAST);
        }
    }
    if (true)
    {
        PERF_DEFINE_REGISTER(rec, "perf_tsc_rdtsc | fast nofence", PERF_COUNTER_RDTSC_NOFENCE);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_REGISTER_START(rec);
            cycles += perf_tsc_rdtsc();
            PERF_REGISTER_RECORD_WRAP(rec, 1, PERF_CPU_FAST);
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1000 * 10000, PERF_CPU_NORMAL, "perf_tsc_rdtsc(lfence)*10 bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            for (size_t i = 0; i < 10; i++)
            {
                cycles += perf_tsc_rdtsc();
            }
        }
    }
    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(rec, "perf_tsc_rdtsc *10| dis fast default 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_REGISTER_START(rec);
            for (size_t i = 0; i < 10; i++)
            {
                cycles += perf_tsc_rdtsc();
            }
            PERF_REGISTER_RECORD_WRAP(rec, 1, PERF_CPU_FAST);
        }
    }
    if (true)
    {
        PERF_DEFINE_REGISTER(rec, "perf_tsc_rdtsc *10| dis fast nofence 1000w", PERF_COUNTER_RDTSC_NOFENCE);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PERF_REGISTER_START(rec);
            for (size_t i = 0; i < 10; i++)
            {
                cycles += perf_tsc_rdtsc();
            }
            PERF_REGISTER_RECORD_WRAP(rec, 1, PERF_CPU_FAST);
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1, PERF_CPU_NORMAL, "sleep 300ms: sys ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1, PERF_CPU_NORMAL, "sleep 300ms rdtscp ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 1, PERF_CPU_NORMAL, "sleep 300ms clock ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10000000, PERF_CPU_FAST, "call cpu fast 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_CPU(ENUM_PERF_TEST, 1000);
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10000000, PERF_CPU_FAST, "call cpu fast wrap 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_CPU_WRAP(ENUM_PERF_TEST, 1, 1000, PERF_CPU_FAST);
        }
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10000000, PERF_CPU_FAST, "call cpu fast wrap 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_CPU_WRAP(ENUM_PERF_TEST, 10, 1000, PERF_CPU_FAST);
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10000000, PERF_CPU_FAST, "call cpu normal wrap 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_CPU_WRAP(ENUM_PERF_TEST, 10, 1000, PERF_CPU_NORMAL);
        }
    }
    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10000000, PERF_CPU_FAST, "call cpu full wrap 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_CPU_WRAP(ENUM_PERF_TEST, 10, 1000, PERF_CPU_FULL);
        }
    }

    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10000000, PERF_CPU_NORMAL, "call mem 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_MEM(ENUM_PERF_TEST, 1, 1000);
        }
    }


 


    if (true)
    {
        PERF_DEFINE_AUTO_SINGLE_RECORD(guard, 10000000, PERF_CPU_NORMAL, "call timer 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PERF_CALL_TIMER(ENUM_PERF_TEST, 1000);
        }
    }

    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(ot, "call timer sleep_for 10ms ");
        for (int i = 0; i < 100; i++)
        {
            PERF_REGISTER_START(ot);
            PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        PERF_REGISTER_START(ot);
        PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
    }

    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(ot, "call timer sleep_for 10*1000*1000ns(10ms) ");
        for (int i = 0; i < 100; i++)
        {
            PERF_REGISTER_START(ot);
            PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
            std::this_thread::sleep_for(std::chrono::nanoseconds(10 * 1000 * 1000));
        }
        PERF_REGISTER_START(ot);
        PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
    }

#ifndef WIN32
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 10 * 1000 * 1000;
    PERF_DEFINE_REGISTER_DEFAULT(ot, "call timer nanosleep 10*1000*1000ns(10ms) ");
    for (int i = 0; i < 100; i++)
    {
        PERF_REGISTER_START(ot);
        PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
        nanosleep(&tim, &tim2);
    }
    PERF_REGISTER_START(ot);
    PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
#endif // WIN32

    if (true)
    {
        PERF_DEFINE_REGISTER_DEFAULT(ot, "call timer sleep_for 100ms ");
        for (int i = 0; i < 20; i++)
        {
            PERF_REGISTER_START(ot);
            PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        PERF_REGISTER_START(ot);
        PERF_CALL_TIMER(ot.track_id(), ot.counter().start_val());
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
        PerfCounter<PERF_COUNTER_RDTSC> rdtsc;
        PerfCounter<PERF_COUNTER_SYS> sys;
        PerfCounter<PERF_COUNTER_CLOCK> linux_clock;
        PerfCounter<PERF_CONNTER_CHRONO> chrono_clock;
        rdtsc.start();
        sys.start();
        linux_clock.start();
        chrono_clock.start();
        clock_t c_clock = clock();
        long long steady_clock = std::chrono::steady_clock().now().time_since_epoch().count();
        long long sys_clock = std::chrono::system_clock().now().time_since_epoch().count();
        
        for (int i = 0; i < 1000*10000; i++)
        {
            cycles += rdtsc.save().cycles();
        }
        rdtsc.save();
        sys.save();
        linux_clock.save();
        chrono_clock.save();
        c_clock = clock() - c_clock;
        steady_clock = std::chrono::steady_clock().now().time_since_epoch().count() - steady_clock;
        sys_clock = std::chrono::system_clock().now().time_since_epoch().count() - sys_clock;

        LogDebug() << "rdtsc stamp use:" << human_time_format(rdtsc.duration_ns()) << ", inner count:" << human_count_format(rdtsc.cycles());
        LogDebug() << "sys stamp use:" << human_time_format(sys.duration_ns()) << ", inner count:" << human_count_format(sys.cycles());
        LogDebug() << "clock stamp use:" << human_time_format(linux_clock.duration_ns()) << ", inner count:" << human_count_format(linux_clock.cycles());
        LogDebug() << "chrono high stamp use:" << human_time_format(chrono_clock.duration_ns()) << ", inner count:" << human_count_format(chrono_clock.cycles());
        LogDebug() << "c clock stamp use:" << (c_clock * 1.0 / CLOCKS_PER_SEC * 1000) << "ms" << ", inner count:" << human_count_format(c_clock);
        LogDebug() << "steady_clock stamp use:" << steady_clock * 1.0 << "c" << ", inner count:" << human_count_format(steady_clock);
        LogDebug() << "sys_clock stamp use:" << sys_clock * 1.0 << "c" << ", inner count:" << human_count_format(sys_clock);

    }
    









    LogInfo() << "all test finish .";
    return 0;
}


