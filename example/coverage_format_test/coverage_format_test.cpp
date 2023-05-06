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



int main(int argc, char* argv[])
{
    PROF_INIT("inner prof");
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;


    constexpr int DATA_COUNT = 1000;
    constexpr int SUPPERSAMPLING = 5;
    static char str_buff[DATA_COUNT][50] = { 0 };
    static int   int_array[DATA_COUNT] = { 0 };
    static float   float_array[DATA_COUNT] = { 0.0f };

    for (int& val : int_array)
    {
        val = rand() - 100;
    }
    for (float& val : float_array)
    {
        val = rand() * 1.0f * rand()/rand() - rand();
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "fn write long long");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                FNLog::write_dec_unsafe<0>(str_buff[i], (long long)int_array[i]);
            }
        }
    }

    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "sprintf %d");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                sprintf(str_buff[i], "%d", int_array[i]);
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "sscanf %d");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                sscanf(str_buff[i], "%d", &int_array[i]);
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "atoi");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                int_array[i] = atoi(str_buff[i]);
            }
        }
    }


    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "fn write double");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                FNLog::write_double_unsafe(str_buff[i], float_array[i]);
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "fn write float");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                FNLog::write_float_unsafe(str_buff[i], float_array[i]);
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "sprintf %g");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                sprintf(str_buff[i], "%g", float_array[i]);
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "sscanf %f");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                sscanf(str_buff[i], "%f", &float_array[i]);
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, SUPPERSAMPLING * DATA_COUNT, "atof ");
        for (int ss = 0; ss < SUPPERSAMPLING; ss++)
        {
            for (int i = 0; i < DATA_COUNT; i++)
            {
                float_array[i] = (float)atof(str_buff[i]);
            }
        }
    }



    PROF_SERIALIZE_FN_LOG();

    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


