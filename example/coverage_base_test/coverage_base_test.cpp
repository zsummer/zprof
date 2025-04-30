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
FNLog::LogStream& ASSERT_ARGS_LOG(FNLog::LogStream& ls, const std::string& head, Args&& ... args)
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




#define START_PROF_COUNTER(T) zprof::Clock<zprof::T> var_##T;  var_##T.Start();
#define RESTART_PROF_COUNTER(T) var_##T.Start();
#define RECORD_PROF_COUNTER(T) ProfInst.RecordCpuFull(ProfInst.declare_begin_id() + zprof::T, var_##T.StopAndSave().cost());;


static inline void OutputLog(const zprof::Report& rp)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
}

int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    LogDebug() << " main begin test. ";

    //³õÊ¼»¯   
    PROF_INIT("inner prof");

    PROF_SET_OUTPUT(&OutputLog);

    ASSERT_TEST(zprof::GetSelfMem().vm_size > 0);
    ASSERT_TEST(zprof::GetSelfMem().rss_size > 0);
    ASSERT_TEST(zprof::GetSelfMem().rss_size <= zprof::GetSelfMem().vm_size);

    for (int i = zprof::kClockNULL; i < zprof::kClockMAX; i++)
    {
        ASSERT_TEST(ProfInst.particle_for_ns(i) > 0.1, "i=", i); //CPU 10Ghz; 0.1ns  
        ASSERT_TEST(ProfInst.particle_for_ns(i) < 1000*1000*1000, "i=", i); //the clock particle is second ?  
    }
    
    int compact_len = (int)ProfInst.compact_writer().offset();
    for (int i = zprof::kClockNULL; i < zprof::kClockMAX; i++)
    {
        int decl_id = ProfInst.declare_begin_id() + i;
        ASSERT_TEST(!ProfInst.node(decl_id).active);
        int ret = ProfInst.Regist(decl_id, "reserve", i, true, false);
        ASSERT_TEST(ret == 0);
        ASSERT_TEST(ProfInst.node(decl_id).active);
        ASSERT_TEST(ProfInst.compact_writer().offset()== (size_t)compact_len);
        ASSERT_TEST(ProfInst.node(decl_id).traits.clk == i);
        ASSERT_TEST(ProfInst.node(decl_id).traits.resident == true);
    }

    
    for (int i = zprof::kClockNULL; i < zprof::kClockMAX; i++)
    {
        int decl_id = ProfInst.declare_begin_id() + i;
        char buf[100];
        sprintf(buf, "counter_%d", i);
        int buf_len = (int)strlen(buf);
        int compact_len = (int)ProfInst.compact_writer().offset();
        int ret = ProfInst.Regist(decl_id, buf, i, false, true);
        ASSERT_TEST(ret == 0);
        ASSERT_TEST(ProfInst.node(decl_id).active);
        ASSERT_TEST(ProfInst.compact_writer().offset() == (size_t)compact_len + (size_t)buf_len + 1);
        ASSERT_TEST(ProfInst.node(decl_id).traits.clk == i);
        ASSERT_TEST(ProfInst.node(decl_id).traits.resident == false);


        int name_id = ProfInst.node(decl_id).traits.name;
        
        ASSERT_TEST(name_id >= 0);
        ASSERT_TEST(name_id <= (int)ProfInst.compact_writer().offset());

        const char* name = &ProfInst.compact_writer().buff()[name_id];

        ASSERT_TEST(strlen(name) == (size_t)ProfInst.node(decl_id).traits.name_len);
        ASSERT_TEST(strcmp(name, buf) == 0);
    }


    //format
    if (true)
    {
        char buf[100] = { 0 };
        zprof::Report r(buf, 100);
        r.PushHumanMem(2 * 1024 * 1024 + 1009*1024);
        r.ClosingString();
        LogInfo() << buf;

        buf[99] = '\0';
        
    }

    //

    if (true)
    {
        constexpr int low_ms = 400;
        constexpr int high_ms = 800;
        constexpr int sleep_count = 4;
        constexpr int sum_ms = low_ms * sleep_count + high_ms * sleep_count;
        constexpr int avg_ms = sum_ms / (sleep_count * 2);
        constexpr int dv_ms = 25; //clock 
        constexpr int dv_total_ms = dv_ms * sleep_count * 2;



        START_PROF_COUNTER(kClockSystem);
        START_PROF_COUNTER(kClockClock);
        START_PROF_COUNTER(kClockChrono);
        START_PROF_COUNTER(kClockSteadyChrono);
        START_PROF_COUNTER(kClockSystemChrono);
        START_PROF_COUNTER(kClockSystemMS);
        START_PROF_COUNTER(kClockVolatileRDTSC);
        START_PROF_COUNTER(kClockPureRDTSC);
        START_PROF_COUNTER(kClockFenceRDTSC);
        START_PROF_COUNTER(kClockBTBFenceRDTSC);
        START_PROF_COUNTER(kClockRDTSCP);
        START_PROF_COUNTER(kClockMFenceRDTSC);
        START_PROF_COUNTER(kClockBTBMFenceRDTSC);
        START_PROF_COUNTER(kClockLockRDTSC);




        for (int i = 0; i < sleep_count; i++)
        {

            RESTART_PROF_COUNTER(kClockSystem);
            RESTART_PROF_COUNTER(kClockClock);
            RESTART_PROF_COUNTER(kClockChrono);
            RESTART_PROF_COUNTER(kClockSteadyChrono);
            RESTART_PROF_COUNTER(kClockSystemChrono);
            RESTART_PROF_COUNTER(kClockSystemMS);
            RESTART_PROF_COUNTER(kClockVolatileRDTSC);
            RESTART_PROF_COUNTER(kClockPureRDTSC);
            RESTART_PROF_COUNTER(kClockFenceRDTSC);
            RESTART_PROF_COUNTER(kClockBTBFenceRDTSC);
            RESTART_PROF_COUNTER(kClockRDTSCP);
            RESTART_PROF_COUNTER(kClockMFenceRDTSC);
            RESTART_PROF_COUNTER(kClockBTBMFenceRDTSC);
            RESTART_PROF_COUNTER(kClockLockRDTSC);

            std::this_thread::sleep_for(std::chrono::milliseconds(low_ms));
            RECORD_PROF_COUNTER(kClockSystem);
            RECORD_PROF_COUNTER(kClockClock);
            RECORD_PROF_COUNTER(kClockChrono);
            RECORD_PROF_COUNTER(kClockSteadyChrono);
            RECORD_PROF_COUNTER(kClockSystemChrono);
            RECORD_PROF_COUNTER(kClockSystemMS);
            RECORD_PROF_COUNTER(kClockVolatileRDTSC);
            RECORD_PROF_COUNTER(kClockPureRDTSC);
            RECORD_PROF_COUNTER(kClockFenceRDTSC);
            RECORD_PROF_COUNTER(kClockBTBFenceRDTSC);
            RECORD_PROF_COUNTER(kClockRDTSCP);
            RECORD_PROF_COUNTER(kClockMFenceRDTSC);
            RECORD_PROF_COUNTER(kClockBTBMFenceRDTSC);
            RECORD_PROF_COUNTER(kClockLockRDTSC);
        }





        for (int i = 0; i < sleep_count; i++)
        {

            RESTART_PROF_COUNTER(kClockSystem);
            RESTART_PROF_COUNTER(kClockClock);
            RESTART_PROF_COUNTER(kClockChrono);
            RESTART_PROF_COUNTER(kClockSteadyChrono);
            RESTART_PROF_COUNTER(kClockSystemChrono);
            RESTART_PROF_COUNTER(kClockSystemMS);
            RESTART_PROF_COUNTER(kClockVolatileRDTSC);
            RESTART_PROF_COUNTER(kClockPureRDTSC);
            RESTART_PROF_COUNTER(kClockFenceRDTSC);
            RESTART_PROF_COUNTER(kClockBTBFenceRDTSC);
            RESTART_PROF_COUNTER(kClockRDTSCP);
            RESTART_PROF_COUNTER(kClockMFenceRDTSC);
            RESTART_PROF_COUNTER(kClockBTBMFenceRDTSC);
            RESTART_PROF_COUNTER(kClockLockRDTSC);

            std::this_thread::sleep_for(std::chrono::milliseconds(high_ms));

            RECORD_PROF_COUNTER(kClockSystem);
            RECORD_PROF_COUNTER(kClockClock);
            RECORD_PROF_COUNTER(kClockChrono);
            RECORD_PROF_COUNTER(kClockSteadyChrono);
            RECORD_PROF_COUNTER(kClockSystemChrono);
            RECORD_PROF_COUNTER(kClockSystemMS);
            RECORD_PROF_COUNTER(kClockVolatileRDTSC);
            RECORD_PROF_COUNTER(kClockPureRDTSC);
            RECORD_PROF_COUNTER(kClockFenceRDTSC);
            RECORD_PROF_COUNTER(kClockBTBFenceRDTSC);
            RECORD_PROF_COUNTER(kClockRDTSCP);
            RECORD_PROF_COUNTER(kClockMFenceRDTSC);
            RECORD_PROF_COUNTER(kClockBTBMFenceRDTSC);
            RECORD_PROF_COUNTER(kClockLockRDTSC);
        }

        for (int i = zprof::kClockNULL+1; i < zprof::kClockMAX; i++)
        {
            int decl_id = ProfInst.declare_begin_id() + i;

            ASSERT_TEST(ProfInst.node(decl_id).cpu.c == sleep_count*2, "i=", i, " c=", ProfInst.node(decl_id).cpu.c);

            double sum = ProfInst.node(decl_id).cpu.sum * ProfInst.particle_for_ns(i);
            double dv = ProfInst.node(decl_id).cpu.dv * ProfInst.particle_for_ns(i);
            double sm = ProfInst.node(decl_id).cpu.sm * ProfInst.particle_for_ns(i);
            double h_sm = ProfInst.node(decl_id).cpu.h_sm * ProfInst.particle_for_ns(i);
            double l_sm = ProfInst.node(decl_id).cpu.l_sm * ProfInst.particle_for_ns(i);
            double max_u = ProfInst.node(decl_id).cpu.max_u * ProfInst.particle_for_ns(i);
            double min_u = ProfInst.node(decl_id).cpu.min_u * ProfInst.particle_for_ns(i);
            LogInfo() << "sum=" << sum << "," << "sum_ms=" << sum_ms << "," << "dv=" << dv << "," << "dv_total_ms=" << dv_total_ms << "," 
                << "sm=" << sm << "," << "high_ms=" << high_ms << "," << "low_ms=" << low_ms << "," << "avg_ms=" << avg_ms 
                << "," << "h_sm=" << h_sm << "," << "l_sm=" << l_sm << "," << "max_u=" << max_u << "," << "min_u=" << min_u << "," << "sleep_count=" << sleep_count << ",";
            ASSERT_TEST(std::abs(sum/1000.0/1000.0 - sum_ms) < dv_total_ms, "i=", i);
            ASSERT_TEST(dv / 1000.0 / 1000.0 < dv_total_ms + (high_ms - low_ms)*sleep_count, "i=", i);
            ASSERT_TEST(sm / 1000.0 / 1000.0 > low_ms, "i=", i);
            ASSERT_TEST(sm / 1000.0 / 1000.0 < high_ms, "i=", i);
            ASSERT_TEST(h_sm / 1000.0 / 1000.0 < high_ms, "i=", i);
            ASSERT_TEST(h_sm / 1000.0 / 1000.0 > avg_ms, "i=", i);
            ASSERT_TEST(l_sm / 1000.0 / 1000.0 < avg_ms, "i=", i);
            ASSERT_TEST(l_sm / 1000.0 / 1000.0 >= low_ms, "i=", i);
            ASSERT_TEST(max_u >= min_u, "i=", i);
            ASSERT_TEST(max_u > 0, "i=", i);
            ASSERT_TEST(min_u > 0, "i=", i);
            ASSERT_TEST(max_u - min_u >= dv/(sleep_count*2), "i=", i);
        }




    }



    LogInfo() << "all test success";

    return 0;
}


