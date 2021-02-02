
/*
* zperf License
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
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
    PERF_DEFINE_STAMP(perf_time);
    for (size_t i = 0; i < 10000; i++)
    {
        char* ptr = new char[100];
        delete[] ptr;
    }
    PERF_CALL_MULTI_CPU_REAL(ENUM_BAT_ALLOC_FREE, 10000, perf_time);

    PERF_TIME_GUARD(guard, ENUM_ENTRY);

    for (size_t i = 0; i < 10000; i++)
    {
        PERF_BEGIN_STAMP(perf_time);
        char* ptr = new char[10];
        PERF_CALL_ONCE_CPU_REAL(ENUM_ALLOC, perf_time);
        PERF_CALL_ONCE_MEM(ENUM_ALLOC, 10);
        PERF_BEGIN_STAMP(perf_time);
        delete[] ptr;
        PERF_CALL_ONCE_CPU_REAL(ENUM_FREE, perf_time);
        PERF_CALL_ONCE_MEM(ENUM_FREE, 10);
    }

}
