
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


#ifndef  TEST_H
#define TEST_H

#include "zperf.h"

enum MyEnum
{
    ENUM_BAT = PERF_USER_NODE_BEGIN,
    ENUM_IND_SUM,
    ENUM_IND,
    ENUM_EMPTY,
    ENUM_ENTRY,
    ENUM_ALLOC_SUM,
    ENUM_ALLOC,
    ENUM_FREE_SUM,
    ENUM_FREE,
};


inline void regist_perf()
{
    sprintf(PerfInst.mutable_desc(), "test");
    REGIST_NODE(ENUM_BAT, "bat");
    REGIST_NODE(ENUM_IND_SUM, "ind sum");
    REGIST_NODE_AUTO(ENUM_IND);
    REGIST_NODE_AUTO(ENUM_EMPTY);
    REGIST_NODE_AUTO(ENUM_ENTRY);
    REGIST_NODE_AUTO(ENUM_ALLOC_SUM);
    REGIST_NODE_AUTO(ENUM_ALLOC);
    REGIST_NODE_AUTO(ENUM_FREE_SUM);
    REGIST_NODE_AUTO(ENUM_FREE);

    BIND_CHILD(ENUM_IND_SUM, ENUM_IND);

    BIND_CHILD(ENUM_EMPTY, ENUM_ENTRY);
    BIND_CHILD(ENUM_ENTRY, ENUM_ALLOC_SUM);
    BIND_CHILD(ENUM_ENTRY, ENUM_FREE_SUM);
    BIND_CHILD(ENUM_ALLOC_SUM, ENUM_ALLOC);
    BIND_CHILD(ENUM_FREE_SUM, ENUM_FREE);

#ifndef WIN32
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(2, &set);
    sched_setaffinity(0, sizeof(set), &set);
#endif
}


void entry_mem_test();

#endif