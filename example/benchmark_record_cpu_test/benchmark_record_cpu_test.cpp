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
#include <unordered_map>
#include <unordered_set>
#include <random>
#include "zprof.h"

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

enum MyTestEnum
{
    MY_DECLARE_BEGIN = ProfInstType::declare_begin_id(),
    NORMAL_NODE, 

    PARRENT_1,
    CHILD_1,
    CHILD_2,
};


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
    volatile long long cost = 123;
    ProfInst.RecordCpuNoSM(1, 0x11223344);
    ProfInst.RecordCpuNoSM(1, cost);

    ProfInst.RecordCpu(1, 0x11223333);
    ProfInst.RecordCpu(1, cost);
    ProfInst.RecordCpuFull(1, 0x11223322);
    ProfInst.RecordCpuFull(1, cost);
    (void)cost;

    return 0;
}


