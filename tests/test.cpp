
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
    PERF_FUNC_GUARD(ENUM_EMPTY, 0);
    PerfTime<> alloc_use;
    for (size_t i = 0; i < 10000; i++)
    {
        alloc_use.begin_track();
        char* ptr = new char[10];
        PERF_CALL_ONCE_CPU_REAL(ENUM_ALLOC, alloc_use, 10);
        PERF_CALL_ONCE_MEM(ENUM_ALLOC, 10);
        alloc_use.begin_track();
        delete[] ptr;
        PERF_CALL_ONCE_CPU_REAL(ENUM_FREE, alloc_use, 10);
        PERF_CALL_ONCE_MEM(ENUM_FREE, 10);
    }

    alloc_use.begin_track();
    for (size_t i = 0; i < 10000; i++)
    {
        char* ptr = new char[10];
        delete[] ptr;
    }
    PERF_CALL_ONCE_CPU_REAL(ENUM_ENTRY, alloc_use, 10);
}
