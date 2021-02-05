
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


#ifndef  TEST_H
#define TEST_H

#include "zperf.h"

enum MyEnum
{
    ENUM_PERF_TEST = PERF_RESERVE_TRACK_BEGIN,
    ENUM_BAT_ALLOC_FREE,
    ENUM_ONE_TICK,
    ENUM_ENTRY,
    ENUM_ALLOC_SUM,
    ENUM_ALLOC,
    ENUM_FREE_SUM,
    ENUM_FREE,
};


inline void regist_perf()
{
    PERF_FAST_REGIST_TRACK(ENUM_PERF_TEST);
    PERF_FAST_REGIST_TRACK(ENUM_ONE_TICK);
    PERF_FAST_REGIST_TRACK(ENUM_BAT_ALLOC_FREE);
    PERF_FAST_REGIST_TRACK(ENUM_ENTRY);
    PERF_FAST_REGIST_TRACK(ENUM_ALLOC_SUM);
    PERF_FAST_REGIST_TRACK(ENUM_ALLOC);
    PERF_FAST_REGIST_TRACK(ENUM_FREE_SUM);
    PERF_FAST_REGIST_TRACK(ENUM_FREE);

    PERF_BIND_CHILD(ENUM_ENTRY, ENUM_ALLOC_SUM);
    PERF_BIND_CHILD(ENUM_ENTRY, ENUM_FREE_SUM);
    PERF_BIND_CHILD(ENUM_ALLOC_SUM, ENUM_ALLOC);
    PERF_BIND_CHILD(ENUM_FREE_SUM, ENUM_FREE);

    PERF_BIND_MERGE(ENUM_BAT_ALLOC_FREE, ENUM_ONE_TICK);

}


void entry_mem_test();

#endif