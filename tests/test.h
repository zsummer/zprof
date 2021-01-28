
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
    ENUM_BAT,
    ENUM_IND_SUM,
    ENUM_IND,
    ENUM_EMPTY,
    ENUM_ENTRY,
    ENUM_ALLOC_SUM,
    ENUM_ALLOC,
    ENUM_FREE_SUM,
    ENUM_FREE,
};

#define REGIST_NODE(id)  PerfInst.regist_node(id, #id, false)
#define BIND_CHILD(id, cid)  PerfInst.add_node_child(id, cid)
inline void regist_perf()
{
    REGIST_NODE(ENUM_BAT);
    REGIST_NODE(ENUM_IND_SUM);
    REGIST_NODE(ENUM_IND);
    REGIST_NODE(ENUM_EMPTY);
    REGIST_NODE(ENUM_ENTRY);
    REGIST_NODE(ENUM_ALLOC_SUM);
    REGIST_NODE(ENUM_ALLOC);
    REGIST_NODE(ENUM_FREE_SUM);
    REGIST_NODE(ENUM_FREE);

    BIND_CHILD(ENUM_IND_SUM, ENUM_IND);

    BIND_CHILD(ENUM_EMPTY, ENUM_ENTRY);
    BIND_CHILD(ENUM_ENTRY, ENUM_ALLOC_SUM);
    BIND_CHILD(ENUM_ENTRY, ENUM_FREE_SUM);
    BIND_CHILD(ENUM_ALLOC_SUM, ENUM_ALLOC);
    BIND_CHILD(ENUM_FREE_SUM, ENUM_FREE);
}

void entry_cpu_test();
void entry_mem_test();

#endif