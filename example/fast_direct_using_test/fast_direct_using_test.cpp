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


int main(int argc, char *argv[])
{
    //初始化   
    ProfInst.init("fast_direct_using");
    
    //手动使用计时器     
    if (true)
    {
        ProfCounter<> counter;
        counter.start();
        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }
        counter.stop_and_save();
        printf("inc * 1000 used:%lld ns \n", counter.duration_ns());
        printf("inc avg used:%g ns \n", counter.duration_ns()/1000.0);
    }


    //自动测试并记录到指定条目(使用临时条目0 演示)     
    if (true)
    {
        if (true)
        {
            ProfAutoRecord<> record(ProfInstType::INNER_PROF_NULL);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
        }
        //立刻输出指定条目 
        ProfInst.output_one_record(ProfInstType::INNER_PROF_NULL);
    }

    //自动测试并使用自定义名字实时输出报告 
    if (true)
    {
        if (true)
        {
            ProfAutoRecord<> record(ProfInstType::INNER_PROF_NULL);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
        }
        //立刻输出 
        ProfInst.output_temp_record("inc*1000");
    }


    //定义条目   
    // 建立跳点(可选)   
    // 
    // 记录条目数据 
    //   
    // 合并条目   
    // 
    // 输出报告   
    // 
    // 清零数据  
    // 记录数据  
    // 合并条目  
    // 输出报告
    // 
    
    return 0;
}


