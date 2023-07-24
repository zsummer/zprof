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
#include "test.h"
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
struct EntityCell
{
    s32 red_cell_;
    s32 orange_cell_;
    s32 green_cell_;
    std::unordered_set<u64> player_refs;
    std::unordered_set<u64> npc_refs;
};

inline s32 AdaptAxisIndex(float axis, int aligning) { return (int)axis / aligning; }
inline u64 AdaptStoreIndex(s32 x, s32 y) { return (((u64)(u32)x) << 32) + (u32)y; }
inline u64 AdaptStoreIndex(float x, float y, int aligning) { return AdaptStoreIndex(AdaptAxisIndex(x, aligning), AdaptAxisIndex(y, aligning)); }

class FastHash
{
public:
    u64 operator()(u64 input)
    {
        return (((input >> 32) * 73856093) ^ ((input & 0xffffffff) * 19349663));
    }
};


static inline void FNLogFunc(const zprof::Report& rp)
{
    /*
    if (rp.offset() > 200)
    {
        printf("%s\n", rp.buff());
        printf("buff size:%d\n", (int)rp.offset());
    }
    */
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
}

int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");
    PROF_SET_OUTPUT(FNLogFunc);

    //ProfInst.init("inner prof");
    regist_prof();

    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin");
    PROF_OUTPUT_SYS_MEM("sys use mem");

    
    if (false)
    {
        //const int test_len = (int)(ProfInst.compact_writer().buff_len() - ProfInst.compact_writer().offset() - 100);
        const int test_len = 50 * 1000;
        char* buf = new char[test_len];
        for (size_t i = 0; i < test_len; i++)
        {
            buf[i] ='0';
        }
        buf[test_len -1] = '\0';
        
        ProfInst.regist(ProfInst.reserve_begin_id(), buf, zprof::CLOCK_DEFAULT, false, true);
        PROF_RECORD_CPU(ProfInst.reserve_begin_id(), 0);
        delete[]buf;
        //PROF_RECORD_CPU()
        
    }
#ifndef WIN32
    if (true)
    {
        int infotype = 0x80000007;
        int a = 0;
        int b = 0;
        int c = 0;
        int d = 0;

        __asm__("cpuid"
            : "=a" (a), "=b" (b), "=c" (c), "=d" (d)   // The output variables. EAX -> a and vice versa.
            : "0" (infotype));                         // Put the infotype into EAX.
        LogDebug() << "edx:" << (void*)(unsigned long long)d << ", bit 8 if true is  invariant TSC.";
    }

#endif // WIN32


    if (true)
    {
        PROF_RECORD_CPU(ENUM_MERGE_CHILD1, 100);
        PROF_RECORD_CPU(ENUM_MERGE_CHILD1, 106);
        PROF_RECORD_CPU(ENUM_MERGE_CHILD2, 200);
        PROF_RECORD_CPU(ENUM_MERGE_CHILD2, 300);

        PROF_RECORD_MEM(ENUM_MERGE_CHILD1, 1, 100);
        PROF_RECORD_MEM(ENUM_MERGE_CHILD1, 1, 106);
        PROF_RECORD_MEM(ENUM_MERGE_CHILD2, 1, 200);
        PROF_RECORD_MEM(ENUM_MERGE_CHILD2, 1, 300);

        PROF_DO_MERGE();
        PROF_OUTPUT_REPORT();

        PROF_RECORD_CPU(ENUM_MERGE_CHILD1, 100);
        PROF_RECORD_CPU(ENUM_MERGE_CHILD1, 106);
        PROF_RECORD_CPU(ENUM_MERGE_CHILD2, 200);
        PROF_RECORD_CPU(ENUM_MERGE_CHILD2, 300);

        PROF_RECORD_MEM(ENUM_MERGE_CHILD1, 1, 100);
        PROF_RECORD_MEM(ENUM_MERGE_CHILD1, 1, 106);
        PROF_RECORD_MEM(ENUM_MERGE_CHILD2, 1, 200);
        PROF_RECORD_MEM(ENUM_MERGE_CHILD2, 1, 300);

        PROF_DO_MERGE();
        PROF_OUTPUT_REPORT();


        PROF_RESET_DECLARE();
        PROF_DO_MERGE();
        PROF_OUTPUT_REPORT();


    }
    if (true)
    {
        PROF_RECORD_USER(ENUM_ENTRY, 1, 100);
        PROF_RECORD_USER(ENUM_ENTRY, 1, 102);
        PROF_RECORD_USER(ENUM_ENTRY, 1, 103);
    }




    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(ot, "self use mem");
    }


    if (true)
    {
       PROF_DEFINE_AUTO_ANON_RECORD(ot, "ProfInst use mem");
       PROF_OUTPUT_SINGLE_MEM("ProfInst use mem", sizeof(ProfInst));
    }

    LogDebug() << "bat parent";

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_SYS bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_SYS>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_CLOCK bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_CLOCK>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_CHRONO bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_CHRONO>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_RDTSC_PURE bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_RDTSC_NOFENCE bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_PURE_RDTSC>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "T_CLOCK_RDTSC_PURE(lfence) bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_RDTSC_BTB(lfence) bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>();
        }
    }

    if (false)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 20, "CLOCK_RDTSCP bat 20");
        for (size_t i = 0; i < 20; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_RDTSCP>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_RDTSC_MFENCE bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_MFENCE_RDTSC>();
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_RDTSC_MFENCE_BTB bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += zprof::get_tick<zprof::T_CLOCK_BTB_MFENCE_RDTSC>();
        }
    }



    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "CLOCK_RDTSC_LOCK bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles +=zprof::get_tick<zprof::T_CLOCK_LOCK_RDTSC>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10 * 10000, "c clock bat 10w");
        for (size_t i = 0; i < 10 * 10000; i++)
        {
            cycles += clock();
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "c++ system_clock bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += std::chrono::system_clock().now().time_since_epoch().count();
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "c++ steady_clock bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += std::chrono::steady_clock().now().time_since_epoch().count();
        }
    }


    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "c time bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += time(NULL);
        }
    }


    LogDebug() << " dis parent";

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_SYS>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_SYS dis 1000w");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_CLOCK>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_CLOCK dis 1000w");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_CHRONO>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_CHRONO dis 1000w");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_RDTSC_PURE dis 1000w");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_PURE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_RDTSC_NOFENCE dis 1000w");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("T_CLOCK_RDTSC_PURE(lfence) dis 1000w");
    }
    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_RDTSC_BTB (lfence) dis 1000w");
    }
    if (false)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 20; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += zprof::get_tick<zprof::T_CLOCK_RDTSCP>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_RDTSCP dis 20");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += zprof::get_tick<zprof::T_CLOCK_MFENCE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_RDTSC_MFENCE dis 1000w");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_BTB_MFENCE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_RDTSC_MFENCE_BTB dis 1000w");
    }



    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_LOCK_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL, cost.stop_and_save().cost());
        }
        PROF_OUTPUT_TEMP_RECORD("CLOCK_RDTSC_LOCK dis 1000w");
    }



    if (true)
    {
        zprof::Clock<> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
            PROF_RECORD_CPU_WRAP(ProfInstType::INNER_NULL, 1, cost.stop_and_save().cost(), zprof::RECORD_LEVEL_FAST);
        }
        PROF_OUTPUT_TEMP_RECORD("T_CLOCK_RDTSC_PURE dis 1000w | RECORD_LEVEL_FAST | DEFAULT");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_SYS> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
            PROF_RECORD_CPU_WRAP(ProfInstType::INNER_NULL, 1, cost.stop_and_save().cost(), zprof::RECORD_LEVEL_FAST);
        }
        PROF_OUTPUT_TEMP_RECORD("T_CLOCK_RDTSC_PURE dis 1000w | RECORD_LEVEL_FAST | CLOCK_SYS");
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_PURE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles +=zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
            PROF_RECORD_CPU_WRAP(ProfInstType::INNER_NULL, 1, cost.stop_and_save().cost(), zprof::RECORD_LEVEL_FAST);
        }
        PROF_OUTPUT_TEMP_RECORD("T_CLOCK_RDTSC_PURE dis 1000w | RECORD_LEVEL_FAST | CLOCK_RDTSC_NOFENCE");
    }




    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000 * 10, "PROF_DEFINE_AUTO_ANON_RECORD: T_CLOCK_RDTSC_PURE(lfence)*10 bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            for (size_t i = 0; i < 10; i++)
            {
                cycles +=zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
            }
        }
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_BTB_FENCE_RDTSC> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            for (size_t i = 0; i < 10; i++)
            {
                cycles +=zprof::get_tick<zprof::T_CLOCK_FENCE_RDTSC>();
            }
            PROF_RECORD_CPU_WRAP(ProfInstType::INNER_NULL, 1, cost.stop_and_save().cost(), zprof::RECORD_LEVEL_FAST);
        }
        PROF_OUTPUT_TEMP_RECORD("T_CLOCK_RDTSC_PURE(lfence) * 10 dis 1000w");
    }

    PROF_BUILD_JUMP_PATH();

    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "sleep 300ms: sys ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "sleep 300ms rdtscp ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "sleep 300ms clock ");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_RECORD_CPU_SAMPLE 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_SAMPLE(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_RECORD_CPU_SAMPLE 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_SAMPLE(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_RECORD_CPU 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "RECORD_LEVEL_FAST 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_WRAP(ENUM_PROF_TEST, 1, 1000, zprof::RECORD_LEVEL_FAST);
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "RECORD_LEVEL_FAST 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_WRAP(ENUM_PROF_TEST, 10, 1000, zprof::RECORD_LEVEL_FAST);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "RECORD_LEVEL_NORMAL 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_WRAP(ENUM_PROF_TEST, 1, 1000, zprof::RECORD_LEVEL_NORMAL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "RECORD_LEVEL_NORMAL 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_WRAP(ENUM_PROF_TEST, 10, 1000, zprof::RECORD_LEVEL_NORMAL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "RECORD_LEVEL_FULL 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_WRAP(ENUM_PROF_TEST, 1, 1000, zprof::RECORD_LEVEL_FULL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "RECORD_LEVEL_FULL 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_CPU_WRAP(ENUM_PROF_TEST, 10, 1000, zprof::RECORD_LEVEL_FULL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "call mem 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_MEM(ENUM_PROF_TEST, 1, 1000);
        }
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start =zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL,zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>() - start);
        }
        PROF_OUTPUT_TEMP_RECORD("call empty rdtsc btb");
    }
    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start =zprof::get_tick<zprof::T_CLOCK_PURE_RDTSC>();
            PROF_RECORD_CPU(ProfInstType::INNER_NULL,zprof::get_tick<zprof::T_CLOCK_PURE_RDTSC>() - start);
        }
        PROF_OUTPUT_TEMP_RECORD("call nofence empty");

    }

    
    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        cycles = 3.1415926;
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start =zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>();
            cycles = cycles / start + i;
            PROF_RECORD_CPU(ProfInstType::INNER_NULL,zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>() - start);
        }
        PROF_OUTPUT_TEMP_RECORD("call fdiv");
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        cycles = 3.1415926;
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start =zprof::get_tick<zprof::T_CLOCK_PURE_RDTSC>();
            cycles = cycles / start + i;
            PROF_RECORD_CPU(ProfInstType::INNER_NULL,zprof::get_tick<zprof::T_CLOCK_PURE_RDTSC>() - start);
        }
        PROF_OUTPUT_TEMP_RECORD("call fdiv no any fence");
    }
    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        cycles = 3.1415926;
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start =zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>();
            cycles = cycles * start;
            PROF_RECORD_CPU(ProfInstType::INNER_NULL,zprof::get_tick<zprof::T_CLOCK_BTB_FENCE_RDTSC>() - start);
        }
        PROF_OUTPUT_TEMP_RECORD("call fmul by rdtsc btb");
    }


    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "call atom++ 1000w ");
        std::atomic<long long> latom;
        for (int i = 0; i < 10000000; i++)
        {
            latom++;
        }
        PROF_RECORD_CPU(0, latom.load());
    }
 


    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "call timer 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_RECORD_TIMER(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        zprof::Clock<> cost;
        for (int i = 0; i < 100; i++)
        {
            cost.start();
            PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        cost.start();
        PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
        PROF_OUTPUT_TEMP_RECORD("call timer sleep_for 10ms ");
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        zprof::Clock<> cost;
        for (int i = 0; i < 100; i++)
        {
            cost.start();
            PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
            std::this_thread::sleep_for(std::chrono::nanoseconds(10 * 1000 * 1000));
        }
        cost.start();
        PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
        PROF_OUTPUT_TEMP_RECORD("call timer sleep_for 10*1000*1000ns(10ms) ");
    }

#ifndef WIN32
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 10 * 1000 * 1000;
    PROF_RESET_CHILD(ProfInstType::INNER_NULL);
    zprof::Clock<> cost;
    for (int i = 0; i < 100; i++)
    {
        cost.start();
        PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
        nanosleep(&tim, &tim2);
    }
    cost.start();
    PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
    PROF_OUTPUT_TEMP_RECORD("call timer nanosleep 10*1000*1000ns(10ms)");

#endif // WIN32

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_NULL);
        zprof::Clock<> cost;
        for (int i = 0; i < 20; i++)
        {
            cost.start();
            PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        cost.start();
        PROF_RECORD_TIMER(ProfInstType::INNER_NULL, cost.get_begin());
        PROF_OUTPUT_TEMP_RECORD("call timer sleep_for 100ms ");
    }

    PROF_RESET_CHILD(ENUM_ENTRY);
    entry_mem_test();
    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();
    PROF_OUTPUT_SELF_MEM("now memory");
    PROF_OUTPUT_SYS_MEM("now sys use mem");

    PROF_RESET_CHILD(ENUM_ENTRY);
    entry_mem_test();

    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT();

    for (size_t i = 0; i < 1000; i++)
    {
        //ProfInst.output_report(0xff);
    }

    if (true)
    {
        zprof::Clock<zprof::T_CLOCK_FENCE_RDTSC> rdtsc;
        zprof::Clock<zprof::T_CLOCK_SYS> sys;
        zprof::Clock<zprof::T_CLOCK_CLOCK> linux_clock;
        zprof::Clock<zprof::T_CLOCK_CHRONO> chrono_clock;
        zprof::Clock<zprof::T_CLOCK_SYS_MS> ms_clock;
        rdtsc.start();
        sys.start();
        linux_clock.start();
        chrono_clock.start();
        ms_clock.start();
        clock_t c_clock = clock();
        long long steady_clock = std::chrono::steady_clock().now().time_since_epoch().count();
        long long sys_clock = std::chrono::system_clock().now().time_since_epoch().count();
        
        for (int i = 0; i < 1000*10000; i++)
        {
            cycles += rdtsc.save().cost();
        }
        rdtsc.save();
        sys.save();
        linux_clock.save();
        chrono_clock.save();
        ms_clock.save();
        c_clock = clock() - c_clock;
        steady_clock = std::chrono::steady_clock().now().time_since_epoch().count() - steady_clock;
        sys_clock = std::chrono::system_clock().now().time_since_epoch().count() - sys_clock;

        LogDebug() << "rdtsc stamp use:" << human_time_format(rdtsc.cost_ns()) << ", inner count:" << human_count_format(rdtsc.cost());
        LogDebug() << "sys stamp use:" << human_time_format(sys.cost_ns()) << ", inner count:" << human_count_format(sys.cost());
        LogDebug() << "clock stamp use:" << human_time_format(linux_clock.cost_ns()) << ", inner count:" << human_count_format(linux_clock.cost());
        LogDebug() << "chrono high stamp use:" << human_time_format(chrono_clock.cost_ns()) << ", inner count:" << human_count_format(chrono_clock.cost());
        LogDebug() << "c clock stamp use:" << (c_clock * 1.0 / CLOCKS_PER_SEC * 1000) << "ms" << ", inner count:" << human_count_format(c_clock);
        LogDebug() << "steady_clock stamp use:" << steady_clock * 1.0 << "c" << ", inner count:" << human_count_format(steady_clock);
        LogDebug() << "sys_clock stamp use:" << sys_clock * 1.0 << "c" << ", inner count:" << human_count_format(sys_clock);
        LogDebug() << "ms_clock stamp use:" << ms_clock.cost() << "c" << ", inner count:" << human_count_format(ms_clock.cost());

    }
    for (size_t i = 0; i < 1; i++)
    {
        PROF_DO_MERGE();
        PROF_OUTPUT_REPORT();
    }
    for (size_t i = 0; i < 1000; i++)
    {
        //ProfInst.output_report(0xff);
    }

    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


