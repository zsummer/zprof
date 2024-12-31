
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


#ifndef  TEST_H
#define TEST_H

#include "zprof.h"

enum MyEnum
{
    ENUM_PROF_TEST = ProfInstType::declare_begin_id(),
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
    ENUM_MERGE_CHILD3,
};


inline void regist_prof()
{
    PROF_FAST_REGIST_NODE(ENUM_PROF_TEST);
    PROF_FAST_REGIST_NODE(ENUM_ONE_TICK);
    PROF_FAST_REGIST_NODE(ENUM_BAT_ALLOC_FREE);
    PROF_FAST_REGIST_NODE(ENUM_ENTRY);
    PROF_FAST_REGIST_NODE(ENUM_ALLOC_SUM);
    PROF_FAST_REGIST_NODE(ENUM_ALLOC);
    PROF_FAST_REGIST_NODE(ENUM_FREE_SUM);
    PROF_FAST_REGIST_NODE(ENUM_FREE);
    PROF_FAST_REGIST_NODE(ENUM_MERGE_ROOT);
    PROF_FAST_REGIST_NODE(ENUM_MERGE_PARRENT);
    PROF_FAST_REGIST_NODE(ENUM_MERGE_CHILD1);
    PROF_FAST_REGIST_NODE(ENUM_MERGE_CHILD2);
    PROF_FAST_REGIST_NODE(ENUM_MERGE_CHILD3);

    PROF_BIND_CHILD(ENUM_ENTRY, ENUM_ALLOC_SUM);
    PROF_BIND_CHILD(ENUM_ENTRY, ENUM_FREE_SUM);
    PROF_BIND_CHILD(ENUM_ALLOC_SUM, ENUM_ALLOC);
    PROF_BIND_CHILD(ENUM_FREE_SUM, ENUM_FREE);

    PROF_BIND_MERGE(ENUM_ONE_TICK, ENUM_BAT_ALLOC_FREE);

    PROF_BIND_CHILD_AND_MERGE(ENUM_MERGE_ROOT, ENUM_MERGE_PARRENT);
    PROF_BIND_CHILD_AND_MERGE(ENUM_MERGE_PARRENT, ENUM_MERGE_CHILD1);
    PROF_BIND_CHILD_AND_MERGE(ENUM_MERGE_PARRENT, ENUM_MERGE_CHILD2);
    PROF_BIND_CHILD_AND_MERGE(ENUM_MERGE_PARRENT, ENUM_MERGE_CHILD3);


    PROF_BUILD_JUMP_PATH();
}


void entry_mem_test();


inline std::string human_time_format(long long cycles)
{
    char buff[50];
    zprof::Report rp(buff, sizeof(buff));
    rp.PushHumanTime(cycles);
    rp.ClosingString();
    return buff;
}
inline std::string human_count_format(long long cycles)
{
    char buff[50];
    zprof::Report rp(buff, sizeof(buff));
    rp.PushHumanCount(cycles);
    rp.ClosingString();
    return buff;
}

#endif