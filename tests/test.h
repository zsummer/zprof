
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
    ENUM_PERF_TEST = PERF_DECLARE_BEGIN,
    ENUM_BAT_ALLOC_FREE,
    ENUM_ONE_TICK,
    ENUM_ENTRY,
    ENUM_ALLOC_SUM,
    ENUM_ALLOC,
    ENUM_FREE_SUM,
    ENUM_FREE,


    ENUM_MERGE_ROOT,
    ENUM_MERGE_PARRENT,
    ENUM_MERGE_CHILD1,
    ENUM_MERGE_CHILD2,
};


inline void regist_perf()
{
    PERF_FAST_REGIST_NODE(ENUM_PERF_TEST);
    PERF_FAST_REGIST_NODE(ENUM_ONE_TICK);
    PERF_FAST_REGIST_NODE(ENUM_BAT_ALLOC_FREE);
    PERF_FAST_REGIST_NODE(ENUM_ENTRY);
    PERF_FAST_REGIST_NODE(ENUM_ALLOC_SUM);
    PERF_FAST_REGIST_NODE(ENUM_ALLOC);
    PERF_FAST_REGIST_NODE(ENUM_FREE_SUM);
    PERF_FAST_REGIST_NODE(ENUM_FREE);
    PERF_FAST_REGIST_NODE(ENUM_MERGE_ROOT);
    PERF_FAST_REGIST_NODE(ENUM_MERGE_PARRENT);
    PERF_FAST_REGIST_NODE(ENUM_MERGE_CHILD1);
    PERF_FAST_REGIST_NODE(ENUM_MERGE_CHILD2);

    PERF_BIND_CHILD(ENUM_ENTRY, ENUM_ALLOC_SUM);
    PERF_BIND_CHILD(ENUM_ENTRY, ENUM_FREE_SUM);
    PERF_BIND_CHILD(ENUM_ALLOC_SUM, ENUM_ALLOC);
    PERF_BIND_CHILD(ENUM_FREE_SUM, ENUM_FREE);

    PERF_BIND_MERGE(ENUM_ONE_TICK, ENUM_BAT_ALLOC_FREE);

    PERF_BIND_CHILD_AND_MERGE(ENUM_MERGE_ROOT, ENUM_MERGE_PARRENT);
    PERF_BIND_CHILD_AND_MERGE(ENUM_MERGE_PARRENT, ENUM_MERGE_CHILD1);
    PERF_BIND_CHILD_AND_MERGE(ENUM_MERGE_PARRENT, ENUM_MERGE_CHILD2);



}


void entry_mem_test();

#endif