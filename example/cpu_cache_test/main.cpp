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
#include "test.h"
#include <unordered_map>
#include <unordered_set>
#include "zhash_map.h"
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




int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");

    if (true)
    {
        PROF_DEFINE_AUTO_SINGLE_RECORD(guard, 1, PROF_LEVEL_NORMAL, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. "; 
    volatile double cycles = 0.0f;

    //cat /sys/devices/system/cpu/cpu1/cache/index0/coherency_line_size 
    for (int stride = 64; stride <=512; stride *=2)
    {
        long long begin_cicle = 0;
        long long end_cicle = 0;

        long long max_bat = 27;
        
        char* org_mem = new char[(1 << max_bat) * 8 + 1024 * 4 * 2];
        char* align_mem_begin = org_mem + 1024 * 4 - ((u64)org_mem) % (1024 * 4);

        double last_cicles = 0;
        volatile register char* load_mem_p = 0;
        for (int bat = 0; bat < 27; bat++)
        {
            char* align_mem_end = align_mem_begin + (1 << bat) * 8;
            if (true)
            {
                char* p = align_mem_begin;
                while (true)
                {
                    if (p + stride * 2 >= align_mem_end)
                    {
                        *(char**)p = align_mem_begin;
                        break;
                    }

                    * (char**)p = p + stride;
                    p = p + stride;
                }
            }




#define	ONE	load_mem_p = *(char**)load_mem_p;
#define	FIVE	ONE ONE ONE ONE ONE
#define	TEN	FIVE FIVE
#define	FIFTY	TEN TEN TEN TEN TEN
#define	HUNDRED	FIFTY FIFTY
#define FIVE_HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED
            if (true)
            {
                load_mem_p = align_mem_begin;
                long long total = 0;
                long long count = 0;
                for (size_t i = 0; i < 100; i++)
                {
                    begin_cicle = prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();
                    FIVE_HUNDRED;
                    end_cicle = prof_get_time_cycle<PROF_COUNTER_RDTSC_BTB>();
                    total += end_cicle - begin_cicle;
                    count += 500;
                }

                if (bat == 0)
                {
                    last_cicles = total * 1.0 /count;
                }
                if (last_cicles > 0.1 && (total / count)/ last_cicles > 1.5)
                {
                    LogAlarm() << "stride:" << stride << " loop bytes:" << align_mem_end - align_mem_begin << " used cicles:" << total / count << ", p=" << (void*)load_mem_p;
                }
                else
                {
                    LogInfo() << "stride:" << stride << " loop bytes:" << align_mem_end - align_mem_begin << " used cicles:" << total / count << ", p=" << (void*)load_mem_p;
                }
                last_cicles = total *1.0 / count;
            }
        }
        delete[] org_mem;
    }

    
    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


