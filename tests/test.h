
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
    ENUM_PERF_TEST = PERF_USER_TRACK_BEGIN,
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
    REGIST_TRACK_AUTO(ENUM_PERF_TEST);
    REGIST_TRACK_AUTO(ENUM_ONE_TICK);
    REGIST_TRACK_AUTO(ENUM_BAT_ALLOC_FREE);
    REGIST_TRACK_AUTO(ENUM_ENTRY);
    REGIST_TRACK_AUTO(ENUM_ALLOC_SUM);
    REGIST_TRACK_AUTO(ENUM_ALLOC);
    REGIST_TRACK_AUTO(ENUM_FREE_SUM);
    REGIST_TRACK_AUTO(ENUM_FREE);

    BIND_CHILD(ENUM_ENTRY, ENUM_ALLOC_SUM);
    BIND_CHILD(ENUM_ENTRY, ENUM_FREE_SUM);
    BIND_CHILD(ENUM_ALLOC_SUM, ENUM_ALLOC);
    BIND_CHILD(ENUM_FREE_SUM, ENUM_FREE);

    BIND_MERGE(ENUM_BAT_ALLOC_FREE, ENUM_ONE_TICK);

}


void entry_mem_test();

#endif