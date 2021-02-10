
/*
* zperf License
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

#include "zperf.h"
#include "test.h"


void entry_mem_test()
{
    PERF_DEFINE_COUNTER(counter);
    PERF_START_COUNTER(counter);

    for (size_t i = 0; i < 10000; i++)
    {
        char* ptr = new char[100];
        delete[] ptr;
    }
    PERF_CALL_CPU_WRAP(ENUM_BAT_ALLOC_FREE, 10000, counter.save().cycles(), false);

    PERF_DEFINE_AUTO_RECORD(guard, ENUM_ENTRY);

    for (size_t i = 0; i < 10000; i++)
    {
        PERF_START_COUNTER(counter);
        char* ptr = new char[10];
        PERF_CALL_CPU_WRAP(ENUM_ALLOC, 1, counter.save().cycles(), false);
        PERF_CALL_MEM(ENUM_ALLOC, 1, 10);
        PERF_START_COUNTER(counter);
        delete[] ptr;
        PERF_CALL_CPU_WRAP(ENUM_FREE, 1, counter.save().cycles(), false);
        PERF_CALL_MEM(ENUM_FREE, 1, 10);
    }

}
