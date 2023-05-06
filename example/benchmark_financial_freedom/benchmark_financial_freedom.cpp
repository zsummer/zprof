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



int main(int argc, char *argv[])
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
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist2(1, 2);

    double bring_billion_count = 0;
    double test_count = 1;

    for (; test_count < 100; test_count++)
    {
        int init_money = 2000;
        int current_money = init_money;
        int stake = 1;
        int fail_by_more_rate = 2;
        int current_step = 0;
        int max_stake = 0;
        while (current_money > 0)
        {
            if (dist2(rng) != 2)
            {
                current_money -= stake;
                if (current_money <= 0)
                {
                    break; //
                }
                stake *= fail_by_more_rate;
                max_stake = max_stake > stake ? max_stake : stake;
            }
            else
            {
                current_money += stake * 2;
                stake = 1;
            }
            current_step++;
            if (current_step % 500000 == 0 || current_money >= 10000 * 10000)
            {
                LogDebug() << "第" << current_step << "轮, 财富:" << current_money << ", 获利:" << current_money - init_money
                    << ", 最大单次赌注:" << max_stake << ", 最大连续亏损:" << max_stake * 2;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (current_money >= 10000 * 10000)
                {
                    bring_billion_count++;
                    break;
                }
            }

        }

        if (current_money <= 0)
        {
            LogError() << "第" << current_step << "轮 破产了!";
        }
        LogInfo() << "总" << current_step << "轮, 财富:" << current_money << ", 获利:" << current_money - init_money
            << ", 最大单次赌注:" << max_stake << ", 最大连续亏损:" << max_stake * 2;
    }
    
    LogInfo() << "开启人生次数" << test_count << ", 财富自由概率:" << bring_billion_count / test_count *100 <<"%";
    return 0;
}


