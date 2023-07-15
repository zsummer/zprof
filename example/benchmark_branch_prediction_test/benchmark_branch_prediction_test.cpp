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


void test()
{

}

int main(int argc, char *argv[])
{
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }
    LogDebug() << " main begin test. ";


    //³õÊ¼»¯   
    PROF_INIT("inner prof");

#ifdef STACK_MEM
    const int ARRAY_SIZE = 8 * 1024 / sizeof(int);
    const int THRESHLOD = 1024;
    int check_array[ARRAY_SIZE];
#else
    const int ARRAY_SIZE = 128*1024 * 1024 / sizeof(int);
    const int THRESHLOD = 1024;
    int* check_array = new int[ARRAY_SIZE];
#endif

    for (size_t i = 0; i < ARRAY_SIZE; i++)
    {
        check_array[i] = rand()% THRESHLOD;
    }

    Clock<CLOCK_RDTSC_BTB> cost;
    volatile int val = 0;
    val++;
    volatile int loops = ARRAY_SIZE;
    

    if (true)
    {
        cost.start();
        for (int i = 0; i < loops; i++)
        {
            val += check_array[i] > THRESHLOD / 2 ? check_array[i] : 0;
            val -= check_array[i] <= THRESHLOD / 2 ? check_array[i] : 0;
        }
        cost.stop_and_save();
        LogInfo() << "un sorted array ternary operator cost:" << cost.duration_ticks() * 1.0 / ARRAY_SIZE << "cycles" << ", val=" << val;
    }



    if (true)
    {
        cost.start();
        for (int i = 0; i < loops; i++)
        {
            if (check_array[i] > THRESHLOD / 2)
            {
                val += check_array[i];
            }
            else
            {
                val -= check_array[i];
            }
        }
        cost.stop_and_save();
        LogInfo() << "un sorted array cost:" << cost.duration_ticks() * 1.0 / ARRAY_SIZE << "cycles" << ", val=" << val;
    }
    if (true)
    {
        cost.start();
        for (int i = 0; i < loops; i++)
        {
            if (check_array[i] > THRESHLOD / 2)
            {
                val += check_array[i];
                if (val == 0)
                {
                    LogInfo() << "side effect" << val;
                }
            }
            else
            {
                val -= check_array[i];
            }
        }
        cost.stop_and_save();
        LogInfo() << "un sorted array two jmp cost:" << cost.duration_ticks() * 1.0 / ARRAY_SIZE << "cycles" << ", val=" << val;
    }



    std::sort(&check_array[0], &check_array[0] + ARRAY_SIZE);

    if (true)
    {
        cost.start();
        for (int i = 0; i < loops; i++)
        {
            val += check_array[i] > THRESHLOD / 2 ? check_array[i] : 0;
            val -= check_array[i] <= THRESHLOD / 2 ? check_array[i] : 0;
        }
        cost.stop_and_save();
        LogInfo() << "sorted array ternary operator cost:" << cost.duration_ticks() * 1.0 / ARRAY_SIZE << "cycles" << ", val=" << val;
    }


    if (true)
    {
        cost.start();
        for (int i = 0; i < loops; i++)
        {
            if (check_array[i] > THRESHLOD / 2)
            {
                val+= check_array[i];
            }
            else
            {
                val -= check_array[i];
            }
        }
        cost.stop_and_save();
        LogInfo() << "sorted array cost:" << cost.duration_ticks() * 1.0 / ARRAY_SIZE << "cycles" << ", val=" << val;
    }
    if (true)
    {
        cost.start();
        for (int i = 0; i < loops; i++)
        {
            if (check_array[i] > THRESHLOD / 2)
            {
                val += check_array[i];
                if (val == 0)
                {
                    LogInfo() << "side effect" << val;
                }
            }
            else
            {
                val -= check_array[i];
            }

        }
        cost.stop_and_save();
        LogInfo() << "sorted array two jmp cost:" << cost.duration_ticks() * 1.0 / ARRAY_SIZE << "cycles" << ", val=" << val;
    }

    return 0;
}


