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

#include "fn_log.h"
#include "zprof.h"
#include <unordered_map>
#include <unordered_set>
#include <random>


template<typename ... Args>
FNLog::LogStream& ASSERT_ARGS_LOG(FNLog::LogStream&& ls, const std::string& head, Args&& ... args)
{
    ls << head << " ";
    std::initializer_list<int>{ (ls << args, '\0') ... };
    return ls;
}

#define ASSERT_TEST(expr, ...)  \
do \
{\
    if(expr) \
    { \
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogDebug(), expr_str, ##__VA_ARGS__, " ok."); \
    }\
    else \
    {\
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogError(), expr_str, ##__VA_ARGS__); \
        return -1; \
    }\
}\
while(0)

#define ASSERT_TEST_NOLOG(expr, ...)  \
do \
{\
    if(!(expr)) \
    {\
        std::string expr_str = #expr; \
        ASSERT_ARGS_LOG(LogError(), expr_str, ##__VA_ARGS__); \
        return -1; \
    }\
}\
while(0)



static inline void OutputLog(const zprof::Report& rp)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
}

int check_merge_to_count(int id)
{
    int ret = 0;
    for (int i = 0; i < ProfInst.merge_leafs_size(); i++)
    {
        int leaf_id = ProfInst.merge_leafs()[i];
        if (ProfInst.node(leaf_id).merge.to == id)
        {
            ret++;
        }
    }
    return ret;
}

enum ProfMergeEnum
{
    PME_BEGIN = ProfInstType::declare_begin_id() + 10 - ((ProfInstType::declare_begin_id() + 10)%10),

    PME_CHILD_CHILD_01,
    PME_CHILD_CHILD_02,
    PME_CHILD_CHILD_11,
    PME_CHILD_CHILD_12,
    PME_CHILD_01 = PME_BEGIN + 10,
    PME_CHILD_02,
    PME_PARENT_01 = PME_BEGIN + 20,
    PME_PARENT_02,
    PME_CHILD_05 = PME_BEGIN +30,
    PME_CHILD_06,
    PME_CHILD_15,
    PME_CHILD_16,
};

int main(int argc, char *argv[])
{
    FNLog::FastStartDebugLogger();
    LogDebug() << " main begin test. ";

    //³õÊ¼»¯   
    PROF_INIT("inner prof");
    PROF_SET_OUTPUT(&OutputLog);

    PROF_FAST_REGIST_NODE(PME_CHILD_CHILD_01);
    PROF_FAST_REGIST_NODE(PME_CHILD_CHILD_02);
    PROF_FAST_REGIST_NODE(PME_CHILD_CHILD_11);
    PROF_FAST_REGIST_NODE(PME_CHILD_CHILD_12);
    PROF_FAST_REGIST_NODE(PME_CHILD_01);
    PROF_FAST_REGIST_NODE(PME_CHILD_02);
    PROF_FAST_REGIST_NODE(PME_PARENT_01);
    PROF_FAST_REGIST_NODE(PME_PARENT_02);
    PROF_FAST_REGIST_NODE(PME_CHILD_05);
    PROF_FAST_REGIST_NODE(PME_CHILD_06);
    PROF_FAST_REGIST_NODE(PME_CHILD_15);
    PROF_FAST_REGIST_NODE(PME_CHILD_16);


    PROF_BIND_CHILD_AND_MERGE(PME_CHILD_01, PME_CHILD_CHILD_01);
    PROF_BIND_CHILD_AND_MERGE(PME_CHILD_01, PME_CHILD_CHILD_02);

    PROF_BIND_CHILD_AND_MERGE(PME_CHILD_02, PME_CHILD_CHILD_11);
    PROF_BIND_CHILD_AND_MERGE(PME_CHILD_02, PME_CHILD_CHILD_12);

    PROF_BIND_CHILD_AND_MERGE(PME_PARENT_01, PME_CHILD_01);
    PROF_BIND_CHILD_AND_MERGE(PME_PARENT_02, PME_CHILD_02);

    PROF_BIND_CHILD_AND_MERGE(PME_PARENT_01, PME_CHILD_05);
    PROF_BIND_CHILD_AND_MERGE(PME_PARENT_01, PME_CHILD_06);

    PROF_BIND_CHILD_AND_MERGE(PME_PARENT_02, PME_CHILD_15);
    PROF_BIND_CHILD_AND_MERGE(PME_PARENT_02, PME_CHILD_16);


    PROF_RECORD_USER(PME_CHILD_CHILD_01, 1, 1);
    PROF_RECORD_USER(PME_CHILD_CHILD_02, 1, 1);
    PROF_RECORD_USER(PME_CHILD_CHILD_11, 1, 1);
    PROF_RECORD_USER(PME_CHILD_CHILD_12, 1, 1);



    PROF_RECORD_USER(PME_CHILD_05, 1, 1);
    PROF_RECORD_USER(PME_CHILD_06, 1, 1);
    PROF_RECORD_USER(PME_CHILD_15, 1, 1);
    PROF_RECORD_USER(PME_CHILD_16, 1, 1);

    
    ASSERT_TEST(check_merge_to_count(PME_CHILD_01) == 2);
    ASSERT_TEST(ProfInst.node(PME_CHILD_01).merge.childs == 2);

    ASSERT_TEST(check_merge_to_count(PME_CHILD_02) == 2);
    ASSERT_TEST(ProfInst.node(PME_CHILD_02).merge.childs == 2);

    ASSERT_TEST(check_merge_to_count(PME_PARENT_01) == 2);
    ASSERT_TEST(ProfInst.node(PME_PARENT_01).merge.childs == 3);

    ASSERT_TEST(check_merge_to_count(PME_PARENT_02) == 2);
    ASSERT_TEST(ProfInst.node(PME_PARENT_02).merge.childs == 3);


    PROF_DO_MERGE();


    ASSERT_TEST(ProfInst.node(PME_CHILD_01).merge.childs == 2);
    ASSERT_TEST(ProfInst.node(PME_CHILD_01).merge.merged == 0);
    ASSERT_TEST(ProfInst.node(PME_CHILD_01).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_CHILD_01).user.sum == 2);

    ASSERT_TEST(ProfInst.node(PME_CHILD_02).merge.childs == 2);
    ASSERT_TEST(ProfInst.node(PME_CHILD_02).merge.merged == 0, ProfInst.node(PME_CHILD_02).merge.merged);
    ASSERT_TEST(ProfInst.node(PME_CHILD_02).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_CHILD_02).user.sum == 2);

    ASSERT_TEST(ProfInst.node(PME_PARENT_01).merge.childs == 3);
    ASSERT_TEST(ProfInst.node(PME_PARENT_01).merge.merged == 0, ProfInst.node(PME_PARENT_01).merge.merged);
    ASSERT_TEST(ProfInst.node(PME_PARENT_01).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_PARENT_01).user.sum == 4);

    ASSERT_TEST(ProfInst.node(PME_PARENT_02).merge.childs == 3);
    ASSERT_TEST(ProfInst.node(PME_PARENT_02).merge.merged == 0);
    ASSERT_TEST(ProfInst.node(PME_PARENT_02).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_PARENT_02).user.sum == 4);


    PROF_DO_MERGE();
    PROF_OUTPUT_REPORT(zprof::OUT_FLAG_DELCARE);

    ASSERT_TEST(ProfInst.node(PME_CHILD_01).merge.childs == 2);
    ASSERT_TEST(ProfInst.node(PME_CHILD_01).merge.merged == 0);
    ASSERT_TEST(ProfInst.node(PME_CHILD_01).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_CHILD_01).user.sum == 2);

    ASSERT_TEST(ProfInst.node(PME_CHILD_02).merge.childs == 2);
    ASSERT_TEST(ProfInst.node(PME_CHILD_02).merge.merged == 0, ProfInst.node(PME_CHILD_02).merge.merged);
    ASSERT_TEST(ProfInst.node(PME_CHILD_02).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_CHILD_02).user.sum == 2);

    ASSERT_TEST(ProfInst.node(PME_PARENT_01).merge.childs == 3);
    ASSERT_TEST(ProfInst.node(PME_PARENT_01).merge.merged == 0, ProfInst.node(PME_PARENT_01).merge.merged);
    ASSERT_TEST(ProfInst.node(PME_PARENT_01).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_PARENT_01).user.sum == 4);

    ASSERT_TEST(ProfInst.node(PME_PARENT_02).merge.childs == 3);
    ASSERT_TEST(ProfInst.node(PME_PARENT_02).merge.merged == 0);
    ASSERT_TEST(ProfInst.node(PME_PARENT_02).user.t_u == 0);
    ASSERT_TEST(ProfInst.node(PME_PARENT_02).user.sum == 4);


    PROF_OUTPUT_REPORT(zprof::OUT_FLAG_DELCARE);

    LogInfo() << "test success.";

    return 0;
}


