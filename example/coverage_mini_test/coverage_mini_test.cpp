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


static inline void OutputLog(const zprof::Report& rp)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
}

int main(int argc, char *argv[])
{
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }
    LogDebug() << " main begin test. ";

    //初始化   
    PROF_INIT("inner prof");


    PROF_OUTPUT_SELF_MEM("default fn log out  test(debug)");

    ProfInst.set_output(&OutputLog);
    PROF_OUTPUT_SELF_MEM("specify fn log out  test(info)");
    ProfInst.set_output(NULL);
    PROF_OUTPUT_SELF_MEM("specify None log out  test(no log)");
    ProfInst.set_output(&OutputLog);

    //序列化打印所有记录  
    PROF_OUTPUT_REPORT();

    //定时清空无指定驻留的数据, 开始新一轮测试
    PROF_RESET_DECLARE();


    static int RENAME_ID = ProfInstType::declare_begin_id() + 10;
    PROF_FAST_REGIST_NODE(RENAME_ID);
    for (int i = 0; i < ProfInst.compact_data_size(); i++)
    {
        int ret = ProfInst.rename(RENAME_ID, "RENAME_ID");
        if (ret != 0)
        {
            LogInfo() << "rename full. rename count:" << i;
            PROF_RECORD_VM(RENAME_ID, zprof::get_self_mem());
            PROF_OUTPUT_RECORD(RENAME_ID);
        }
    }
    PROF_RECORD_VM(RENAME_ID, zprof::get_self_mem());
    PROF_OUTPUT_RECORD(RENAME_ID);

    for (unsigned int i = 0; i < 64; i++)
    {
        char buf[20];
        sprintf(buf, "2^%u", i);
        PROF_OUTPUT_SINGLE_CPU(buf, (unsigned long long)pow(2LLU, i));
    }
    return 0;
}


