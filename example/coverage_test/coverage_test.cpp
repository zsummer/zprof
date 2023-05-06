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


static inline void FNLogFunc(const ProfSerializer& serializer)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(serializer.buff(), (int)serializer.offset());
}

int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");
    PROF_SET_OUTPUT(FNLogFunc);

    //ProfInst.init("inner prof");
    regist_prof();
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;


    
    if (false)
    {
        //const int test_len = (int)(ProfInst.compact_buffer().buff_len() - ProfInst.compact_buffer().offset() - 100);
        const int test_len = 50 * 1000;
        char* buf = new char[test_len];
        for (size_t i = 0; i < test_len; i++)
        {
            buf[i] ='0';
        }
        buf[test_len -1] = '\0';
        
        ProfInst.regist_node(ProfInst.node_reserve_begin_id(), buf, PROF_COUNTER_DEFAULT, false, true);
        PROF_CALL_CPU(ProfInst.node_reserve_begin_id(), 0);
        delete[]buf;
        //PROF_CALL_CPU()
        
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
        PROF_CALL_CPU(ENUM_MERGE_CHILD1, 100);
        PROF_CALL_CPU(ENUM_MERGE_CHILD1, 106);
        PROF_CALL_CPU(ENUM_MERGE_CHILD2, 200);
        PROF_CALL_CPU(ENUM_MERGE_CHILD2, 300);

        PROF_CALL_MEM(ENUM_MERGE_CHILD1, 1, 100);
        PROF_CALL_MEM(ENUM_MERGE_CHILD1, 1, 106);
        PROF_CALL_MEM(ENUM_MERGE_CHILD2, 1, 200);
        PROF_CALL_MEM(ENUM_MERGE_CHILD2, 1, 300);

        PROF_UPDATE_MERGE();
        PROF_OUTPUT_REPORT();

        PROF_CALL_CPU(ENUM_MERGE_CHILD1, 100);
        PROF_CALL_CPU(ENUM_MERGE_CHILD1, 106);
        PROF_CALL_CPU(ENUM_MERGE_CHILD2, 200);
        PROF_CALL_CPU(ENUM_MERGE_CHILD2, 300);

        PROF_CALL_MEM(ENUM_MERGE_CHILD1, 1, 100);
        PROF_CALL_MEM(ENUM_MERGE_CHILD1, 1, 106);
        PROF_CALL_MEM(ENUM_MERGE_CHILD2, 1, 200);
        PROF_CALL_MEM(ENUM_MERGE_CHILD2, 1, 300);

        PROF_UPDATE_MERGE();
        PROF_OUTPUT_REPORT();


        PROF_CLEAN_DECLARE();
        PROF_UPDATE_MERGE();
        PROF_OUTPUT_REPORT();


    }
    if (true)
    {
        PROF_CALL_USER(ENUM_ENTRY, 1, 100);
        PROF_CALL_USER(ENUM_ENTRY, 1, 102);
        PROF_CALL_USER(ENUM_ENTRY, 1, 103);
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
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_SYS bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_SYS>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_CLOCK bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_CLOCK>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_CONNTER_CHRONO bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_CONNTER_CHRONO>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_RDTSC_PURE bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_PURE>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_RDTSC_NOFENCE bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_RDTSC(lfence) bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_RDTSC_BTB(lfence) bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();
        }
    }

    if (false)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 20, "PROF_COUNTER_RDTSCP bat 20");
        for (size_t i = 0; i < 20; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSCP>();
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_RDTSC_MFENCE bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE>();
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_RDTSC_MFENCE_BTB bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE_BTB>();
        }
    }



    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000, "PROF_COUNTER_RDTSC_LOCK bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_LOCK>();
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
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_SYS>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_SYS dis 1000w");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_CLOCK>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_CLOCK dis 1000w");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_CONNTER_CHRONO>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_CONNTER_CHRONO dis 1000w");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_PURE>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC_PURE dis 1000w");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC_NOFENCE dis 1000w");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC(lfence) dis 1000w");
    }
    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC_BTB (lfence) dis 1000w");
    }
    if (false)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 20; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSCP>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSCP dis 20");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC_MFENCE dis 1000w");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_MFENCE_BTB>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC_MFENCE_BTB dis 1000w");
    }



    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC_LOCK>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, cost.stop_and_save().cycles());
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC_LOCK dis 1000w");
    }



    if (true)
    {
        ProfCounter<PROF_COUNTER_DEFAULT> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC>();
            PROF_CALL_CPU_WRAP(ProfInstType::INNER_PROF_NULL, 1, cost.stop_and_save().cycles(), PROF_LEVEL_FAST);
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC dis 1000w | PROF_LEVEL_FAST | DEFAULT");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_SYS> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC>();
            PROF_CALL_CPU_WRAP(ProfInstType::INNER_PROF_NULL, 1, cost.stop_and_save().cycles(), PROF_LEVEL_FAST);
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC dis 1000w | PROF_LEVEL_FAST | PROF_COUNTER_SYS");
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_NOFENCE> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC>();
            PROF_CALL_CPU_WRAP(ProfInstType::INNER_PROF_NULL, 1, cost.stop_and_save().cycles(), PROF_LEVEL_FAST);
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC dis 1000w | PROF_LEVEL_FAST | PROF_COUNTER_RDTSC_NOFENCE");
    }




    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 1000 * 10000 * 10, "PROF_DEFINE_AUTO_ANON_RECORD: PROF_COUNTER_RDTSC(lfence)*10 bat 1000w");
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            for (size_t i = 0; i < 10; i++)
            {
                cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC>();
            }
        }
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC_BTB> cost;
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        for (size_t i = 0; i < 1000 * 10000; i++)
        {
            PROF_START_COUNTER(cost);
            for (size_t i = 0; i < 10; i++)
            {
                cycles += prof_get_time_cycle<PROF_COUNTER_RDTSC>();
            }
            PROF_CALL_CPU_WRAP(ProfInstType::INNER_PROF_NULL, 1, cost.stop_and_save().cycles(), PROF_LEVEL_FAST);
        }
        PROF_OUTPUT_DEFAULT_LOG("PROF_COUNTER_RDTSC(lfence) * 10 dis 1000w");
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
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_CALL_CPU_SAMPLE 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_SAMPLE(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_CALL_CPU_SAMPLE 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_SAMPLE(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_CALL_CPU 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_LEVEL_FAST 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_WRAP(ENUM_PROF_TEST, 1, 1000, PROF_LEVEL_FAST);
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_LEVEL_FAST 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_WRAP(ENUM_PROF_TEST, 10, 1000, PROF_LEVEL_FAST);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_LEVEL_NORMAL 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_WRAP(ENUM_PROF_TEST, 1, 1000, PROF_LEVEL_NORMAL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_LEVEL_NORMAL 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_WRAP(ENUM_PROF_TEST, 10, 1000, PROF_LEVEL_NORMAL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_LEVEL_FULL 1000w (without count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_WRAP(ENUM_PROF_TEST, 1, 1000, PROF_LEVEL_FULL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "PROF_LEVEL_FULL 1000w (with count)");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_CPU_WRAP(ENUM_PROF_TEST, 10, 1000, PROF_LEVEL_FULL);
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "call mem 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_MEM(ENUM_PROF_TEST, 1, 1000);
        }
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start = prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>() - start);
        }
        PROF_OUTPUT_DEFAULT_LOG("call empty rdtsc btb");
    }
    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start = prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>();
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>() - start);
        }
        PROF_OUTPUT_DEFAULT_LOG("call nofence empty");

    }

    
    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        cycles = 3.1415926;
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start = prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();
            cycles = cycles / start + i;
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>() - start);
        }
        PROF_OUTPUT_DEFAULT_LOG("call fdiv");
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        cycles = 3.1415926;
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start = prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>();
            cycles = cycles / start + i;
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, prof_get_time_cycle<PROF_COUNTER_RDTSC_NOFENCE>() - start);
        }
        PROF_OUTPUT_DEFAULT_LOG("call fdiv no any fence");
    }
    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        cycles = 3.1415926;
        long long start = 0;
        for (int i = 0; i < 10000000; i++)
        {
            start = prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();
            cycles = cycles * start;
            PROF_CALL_CPU(ProfInstType::INNER_PROF_NULL, prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>() - start);
        }
        PROF_OUTPUT_DEFAULT_LOG("call fmul by rdtsc btb");
    }


    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "call atom++ 1000w ");
        std::atomic<long long> latom;
        for (int i = 0; i < 10000000; i++)
        {
            latom++;
        }
        PROF_CALL_CPU(0, latom.load());
    }
 


    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000000, "call timer 1000w ");
        for (int i = 0; i < 10000000; i++)
        {
            PROF_CALL_TIMER(ENUM_PROF_TEST, 1000);
        }
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        ProfCounter<PROF_COUNTER_DEFAULT> counter;
        for (int i = 0; i < 100; i++)
        {
            counter.start();
            PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        counter.start();
        PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
        PROF_OUTPUT_DEFAULT_LOG("call timer sleep_for 10ms ");
    }

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        ProfCounter<PROF_COUNTER_DEFAULT> counter;
        for (int i = 0; i < 100; i++)
        {
            counter.start();
            PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
            std::this_thread::sleep_for(std::chrono::nanoseconds(10 * 1000 * 1000));
        }
        counter.start();
        PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
        PROF_OUTPUT_DEFAULT_LOG("call timer sleep_for 10*1000*1000ns(10ms) ");
    }

#ifndef WIN32
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 10 * 1000 * 1000;
    PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
    ProfCounter<PROF_COUNTER_DEFAULT> counter;
    for (int i = 0; i < 100; i++)
    {
        counter.start();
        PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
        nanosleep(&tim, &tim2);
    }
    counter.start();
    PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
    PROF_OUTPUT_DEFAULT_LOG("call timer nanosleep 10*1000*1000ns(10ms)");

#endif // WIN32

    if (true)
    {
        PROF_RESET_CHILD(ProfInstType::INNER_PROF_NULL);
        ProfCounter<PROF_COUNTER_DEFAULT> counter;
        for (int i = 0; i < 20; i++)
        {
            counter.start();
            PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        counter.start();
        PROF_CALL_TIMER(ProfInstType::INNER_PROF_NULL, counter.start_val());
        PROF_OUTPUT_DEFAULT_LOG("call timer sleep_for 100ms ");
    }

    PROF_RESET_CHILD(ENUM_ENTRY);
    entry_mem_test();
    PROF_UPDATE_MERGE();
    PROF_OUTPUT_REPORT();
    PROF_OUTPUT_SELF_MEM("now memory");


    PROF_RESET_CHILD(ENUM_ENTRY);
    entry_mem_test();

    PROF_UPDATE_MERGE();
    PROF_OUTPUT_REPORT();

    for (size_t i = 0; i < 1000; i++)
    {
        auto void_call = [](const ProfSerializer& serializer) {};
        ProfInst.output_report(0xff);
    }

    if (true)
    {
        ProfCounter<PROF_COUNTER_RDTSC> rdtsc;
        ProfCounter<PROF_COUNTER_SYS> sys;
        ProfCounter<PROF_COUNTER_CLOCK> linux_clock;
        ProfCounter<PROF_CONNTER_CHRONO> chrono_clock;
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
    for (size_t i = 0; i < 1; i++)
    {
        PROF_UPDATE_MERGE();
        PROF_OUTPUT_REPORT();
    }
    for (size_t i = 0; i < 1000; i++)
    {
        ProfInst.output_report(0xff);
    }

    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


