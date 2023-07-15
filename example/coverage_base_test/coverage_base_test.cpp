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


template<typename ... Args>
FNLog::LogStream& ASSERT_ARGS_LOG(FNLog::LogStream&& ls, const std::string& head, Args&& ... args)
{
    ls << head << " ";
    std::initializer_list<int>{ (ls << args, '\0') ... };
    return ls;
}

#define ASSERT_TEST(expr, ...)  \
do \
{\
    if(expr) \
    { \
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogDebug(), expr_str, ##__VA_ARGS__, " ok."); \
    }\
    else \
    {\
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogError(), expr_str, ##__VA_ARGS__); \
        return -1; \
    }\
}\
while(0)

#define ASSERT_TEST_NOLOG(expr, ...)  \
do \
{\
    if(!(expr)) \
    {\
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogError(), expr_str, ##__VA_ARGS__); \
        return -1; \
    }\
}\
while(0)




#define START_PROF_COUNTER(T) Clock<T> var_##T;  var_##T.start();
#define RESTART_PROF_COUNTER(T) var_##T.start();
#define RECORD_PROF_COUNTER(T) ProfInst.record_cpu_full(ProfInst.declare_begin_id() + T, var_##T.stop_and_save().duration_ticks());;


static inline void OutputLog(const ProfSerializer& serializer)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(serializer.buff(), (int)serializer.offset());
}

int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    LogDebug() << " main begin test. ";

    //³õÊ¼»¯   
    PROF_INIT("inner prof");

    PROF_SET_OUTPUT(&OutputLog);

    ASSERT_TEST(get_self_mem().vm_size > 0);
    ASSERT_TEST(get_self_mem().rss_size > 0);
    ASSERT_TEST(get_self_mem().rss_size <= get_self_mem().vm_size);

    for (int i = CLOCK_NULL; i < CLOCK_MAX; i++)
    {
        ASSERT_TEST(ProfInst.particle_for_ns(i) > 0.1, "i=", i); //CPU 10Ghz; 0.1ns  
        ASSERT_TEST(ProfInst.particle_for_ns(i) < 1000*1000*1000, "i=", i); //the clock particle is second ?  
    }
    
    int compact_len = (int)ProfInst.compact_writer().offset();
    for (int i = CLOCK_NULL; i < CLOCK_MAX; i++)
    {
        int decl_id = ProfInst.declare_begin_id() + i;
        ASSERT_TEST(!ProfInst.node(decl_id).active);
        int ret = ProfInst.regist(decl_id, "reserve", i, true, false);
        ASSERT_TEST(ret == 0);
        ASSERT_TEST(ProfInst.node(decl_id).active);
        ASSERT_TEST(ProfInst.compact_writer().offset()== (size_t)compact_len);
        ASSERT_TEST(ProfInst.node(decl_id).traits.counter_type == i);
        ASSERT_TEST(ProfInst.node(decl_id).traits.resident == true);
    }

    
    for (int i = CLOCK_NULL; i < CLOCK_MAX; i++)
    {
        int decl_id = ProfInst.declare_begin_id() + i;
        char buf[100];
        sprintf(buf, "counter_%d", i);
        int buf_len = (int)strlen(buf);
        int compact_len = (int)ProfInst.compact_writer().offset();
        int ret = ProfInst.regist(decl_id, buf, i, false, true);
        ASSERT_TEST(ret == 0);
        ASSERT_TEST(ProfInst.node(decl_id).active);
        ASSERT_TEST(ProfInst.compact_writer().offset() == (size_t)compact_len + (size_t)buf_len + 1);
        ASSERT_TEST(ProfInst.node(decl_id).traits.counter_type == i);
        ASSERT_TEST(ProfInst.node(decl_id).traits.resident == false);


        int name_id = ProfInst.node(decl_id).traits.name;
        
        ASSERT_TEST(name_id >= 0);
        ASSERT_TEST(name_id <= (int)ProfInst.compact_writer().offset());

        const char* name = &ProfInst.compact_writer().buff()[name_id];

        ASSERT_TEST(strlen(name) == (size_t)ProfInst.node(decl_id).traits.name_len);
        ASSERT_TEST(strcmp(name, buf) == 0);
    }



    //

    if (true)
    {
        int low_ms = 400;
        int high_ms = 800;
        int sleep_count = 4;
        int sum_ms = low_ms * sleep_count + high_ms * sleep_count;
        int avg_ms = sum_ms / (sleep_count * 2);
        int dv_ms = 20; //clock 
        int dv_total_ms = dv_ms * sleep_count * 2;



        START_PROF_COUNTER(CLOCK_SYS);
        START_PROF_COUNTER(CLOCK_CLOCK);
        START_PROF_COUNTER(CLOCK_CHRONO);
        START_PROF_COUNTER(CLOCK_CHRONO_STEADY);
        START_PROF_COUNTER(CLOCK_CHRONO_SYS);
        START_PROF_COUNTER(CLOCK_RDTSC_PURE);
        START_PROF_COUNTER(CLOCK_RDTSC_NOFENCE);
        START_PROF_COUNTER(CLOCK_RDTSC);
        START_PROF_COUNTER(CLOCK_RDTSC_BTB);
        START_PROF_COUNTER(CLOCK_RDTSCP);
        START_PROF_COUNTER(CLOCK_RDTSC_MFENCE);
        START_PROF_COUNTER(CLOCK_RDTSC_MFENCE_BTB);
        START_PROF_COUNTER(CLOCK_RDTSC_LOCK);




        for (int i = 0; i < sleep_count; i++)
        {

            RESTART_PROF_COUNTER(CLOCK_SYS);
            RESTART_PROF_COUNTER(CLOCK_CLOCK);
            RESTART_PROF_COUNTER(CLOCK_CHRONO);
            RESTART_PROF_COUNTER(CLOCK_CHRONO_STEADY);
            RESTART_PROF_COUNTER(CLOCK_CHRONO_SYS);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_PURE);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_NOFENCE);
            RESTART_PROF_COUNTER(CLOCK_RDTSC);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_BTB);
            RESTART_PROF_COUNTER(CLOCK_RDTSCP);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_MFENCE);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_MFENCE_BTB);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_LOCK);

            std::this_thread::sleep_for(std::chrono::milliseconds(low_ms));
            RECORD_PROF_COUNTER(CLOCK_SYS);
            RECORD_PROF_COUNTER(CLOCK_CLOCK);
            RECORD_PROF_COUNTER(CLOCK_CHRONO);
            RECORD_PROF_COUNTER(CLOCK_CHRONO_STEADY);
            RECORD_PROF_COUNTER(CLOCK_CHRONO_SYS);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_PURE);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_NOFENCE);
            RECORD_PROF_COUNTER(CLOCK_RDTSC);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_BTB);
            RECORD_PROF_COUNTER(CLOCK_RDTSCP);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_MFENCE);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_MFENCE_BTB);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_LOCK);
        }





        for (int i = 0; i < sleep_count; i++)
        {

            RESTART_PROF_COUNTER(CLOCK_SYS);
            RESTART_PROF_COUNTER(CLOCK_CLOCK);
            RESTART_PROF_COUNTER(CLOCK_CHRONO);
            RESTART_PROF_COUNTER(CLOCK_CHRONO_STEADY);
            RESTART_PROF_COUNTER(CLOCK_CHRONO_SYS);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_PURE);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_NOFENCE);
            RESTART_PROF_COUNTER(CLOCK_RDTSC);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_BTB);
            RESTART_PROF_COUNTER(CLOCK_RDTSCP);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_MFENCE);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_MFENCE_BTB);
            RESTART_PROF_COUNTER(CLOCK_RDTSC_LOCK);

            std::this_thread::sleep_for(std::chrono::milliseconds(high_ms));

            RECORD_PROF_COUNTER(CLOCK_SYS);
            RECORD_PROF_COUNTER(CLOCK_CLOCK);
            RECORD_PROF_COUNTER(CLOCK_CHRONO);
            RECORD_PROF_COUNTER(CLOCK_CHRONO_STEADY);
            RECORD_PROF_COUNTER(CLOCK_CHRONO_SYS);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_PURE);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_NOFENCE);
            RECORD_PROF_COUNTER(CLOCK_RDTSC);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_BTB);
            RECORD_PROF_COUNTER(CLOCK_RDTSCP);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_MFENCE);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_MFENCE_BTB);
            RECORD_PROF_COUNTER(CLOCK_RDTSC_LOCK);
        }

        for (int i = CLOCK_NULL+1; i < CLOCK_MAX; i++)
        {
            int decl_id = ProfInst.declare_begin_id() + i;

            ASSERT_TEST(ProfInst.node(decl_id).cpu.c == sleep_count*2, "i=", i);

            double sum = ProfInst.node(decl_id).cpu.sum * ProfInst.particle_for_ns(i);
            double dv = ProfInst.node(decl_id).cpu.dv * ProfInst.particle_for_ns(i);
            double sm = ProfInst.node(decl_id).cpu.sm * ProfInst.particle_for_ns(i);
            double h_sm = ProfInst.node(decl_id).cpu.h_sm * ProfInst.particle_for_ns(i);
            double l_sm = ProfInst.node(decl_id).cpu.l_sm * ProfInst.particle_for_ns(i);
            double max_u = ProfInst.node(decl_id).cpu.max_u * ProfInst.particle_for_ns(i);
            double min_u = ProfInst.node(decl_id).cpu.min_u * ProfInst.particle_for_ns(i);

            ASSERT_TEST(std::abs(sum/1000.0/1000.0 - sum_ms) < dv_total_ms, "i=", i);
            ASSERT_TEST(dv / 1000.0 / 1000.0 < dv_total_ms + (high_ms - low_ms)*sleep_count, "i=", i);
            ASSERT_TEST(sm / 1000.0 / 1000.0 > low_ms, "i=", i);
            ASSERT_TEST(sm / 1000.0 / 1000.0 < high_ms, "i=", i);
            ASSERT_TEST(h_sm / 1000.0 / 1000.0 < high_ms, "i=", i);
            ASSERT_TEST(h_sm / 1000.0 / 1000.0 > avg_ms, "i=", i);
            ASSERT_TEST(l_sm / 1000.0 / 1000.0 < avg_ms, "i=", i);
            ASSERT_TEST(l_sm / 1000.0 / 1000.0 > low_ms, "i=", i);
            ASSERT_TEST(max_u >= min_u, "i=", i);
            ASSERT_TEST(max_u > 0, "i=", i);
            ASSERT_TEST(min_u > 0, "i=", i);
            ASSERT_TEST(max_u - min_u >= dv/(sleep_count*2), "i=", i);
        }




    }



    LogInfo() << "all test success";

    return 0;
}


