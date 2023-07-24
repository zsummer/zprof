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


#include "zprof.h"
#include <thread> //use this_thread::sleep  
#include "fn_log.h"

static inline void OutputLog(const zprof::Report& rp)
{
    LOG_STREAM_DEFAULT_LOGGER(0, FNLog::PRIORITY_DEBUG, 0, 0, FNLog::LOG_PREFIX_NULL).write_buffer(rp.buff(), (int)rp.offset());
}

static inline void OutputPrint(const zprof::Report& rp)
{
    printf("%s\n", rp.buff());
}


int main(int argc, char* argv[])
{
    FNLog::FastStartDebugLogger();

    //≥ı ºªØ   
    PROF_INIT("fast_using_output_test");


    PROF_DEFINE_COUNTER(cost);
    PROF_START_COUNTER(cost);
    for (size_t i = 0; i < 1000; i++)
    {
        volatile size_t inc = 0;
        inc++;
    }
    PROF_STOP_AND_SAVE_COUNTER(cost);

    PROF_OUTPUT_MULTI_COUNT_CPU("default output", 1000, cost.cost());



    PROF_SET_OUTPUT(&OutputPrint);
    PROF_OUTPUT_MULTI_COUNT_CPU("print output", 1000, cost.cost());

    PROF_SET_OUTPUT(&OutputLog);
    PROF_OUTPUT_MULTI_COUNT_CPU("log output", 1000, cost.cost());

    return 0;
}


