
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

#include "zprof.h"
#include "test.h"

int fence_func()
{
    return rand()%5;
}

int test_record_cpu()
{
    PROF_RECORD_CPU(ProfInstType::reserve_begin_id(), 0x12345);
    volatile int ret = fence_func();
    PROF_RECORD_CPU_SAMPLE(ProfInstType::reserve_begin_id(), 0x54321);
    return ret;
}

void entry_mem_test()
{
    PROF_DEFINE_COUNTER(cost);
    PROF_START_COUNTER(cost);
    

    for (size_t i = 0; i < 10000; i++)
    {
        char* ptr = new char[100];
        delete[] ptr;
    }
    PROF_RECORD_CPU_WRAP(ENUM_BAT_ALLOC_FREE, 10000, cost.save().cost(), zprof::kRecordLevelNormal);

    PROF_DEFINE_AUTO_RECORD(guard, ENUM_ENTRY);

    for (size_t i = 0; i < 10000; i++)
    {
        PROF_START_COUNTER(cost);
        char* ptr = new char[10];
        PROF_RECORD_CPU_WRAP(ENUM_ALLOC, 1, cost.save().cost(), zprof::kRecordLevelNormal);
        PROF_RECORD_MEM(ENUM_ALLOC, 1, 10);
        PROF_START_COUNTER(cost);
        delete[] ptr;
        PROF_RECORD_CPU_WRAP(ENUM_FREE, 1, cost.save().cost(), zprof::kRecordLevelNormal);
        PROF_RECORD_MEM(ENUM_FREE, 1, 10);
    }
    test_record_cpu();
}
