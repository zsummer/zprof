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


enum ProfEnumType
{
    PROF_REG_BEGIN = ProfInstType::declare_begin_id() + 10, //reserve 10;  it's used scene 1~8 case  
    PROF_REG_ALL_MATH,
    PROF_REG_INC,
    PROF_REG_SUB,
    PROF_REG_MUL,
    PROF_REG_DIV,
    PROF_REG_VM_USE,
    PROF_REG_TIMMER,
    PROF_REG_SELF_SIZE,
};

int main(int argc, char* argv[])
{
    //初始化   
    PROF_INIT("fast_direct_using");


    //scene 1  
    //手动使用计时器并直接使用标准时间单位的耗时      
    if (true)
    {
        PROF_DEFINE_COUNTER(counter);
        PROF_START_COUNTER(counter);

        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }
        PROF_STOP_AND_SAVE_COUNTER(counter);

        printf("scene 1: inc * 1000 used:%lld ns \n", counter.duration_ns());
        printf("scene 1: inc avg used:%g ns \n", counter.duration_ns() / 1000.0);
    }





    //scene 2  
    //手动使用计时器 并记录到<注册条目>中 然后立刻输出该<条目>报告     
    static const int scene_2_id = ProfInst.declare_begin_id();
    if (true)
    {
        //注册ID  
        PROF_REGIST_NODE(scene_2_id, "scene 2", CLOCK_RDTSC, false, false);

        //计算耗时 
        PROF_DEFINE_COUNTER(cost);
        PROF_START_COUNTER(cost);
        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }
        PROF_STOP_AND_SAVE_COUNTER(cost);

        //记录到条目
        PROF_RECORD_CPU(scene_2_id, cost.duration_ticks());

        //立刻输出条目 
        PROF_OUTPUT_RECORD(scene_2_id);
    }



    //scene 3:  scene2的糖  
    static const int scene_3_id = scene_2_id + 1;
    if (true)
    {
        //注册ID 
        PROF_FAST_REGIST_NODE_ALIAS(scene_3_id, "scene 3");

        //计算耗时并记录到scene_3_id 
        if (true)
        {
            PROF_DEFINE_AUTO_RECORD(cost, scene_3_id);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
        }

        //立刻输出条目 
        PROF_OUTPUT_RECORD(scene_3_id);
    }


    //scene 4: 使用<临时条目>输出性能信息 而不新增注册条目    
    if (true)
    {
        static const int scene_4_tmp_id = ProfInstType::INNER_PROF_NULL;
        if (true)
        {
            PROF_DEFINE_AUTO_RECORD(cost, scene_4_tmp_id);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
        }

        //<临时条目>输出 调用该接口用于输出带<条目信息>的性能信息, 并且随后自动清理性能信息防止污染下次使用    
        PROF_OUTPUT_TEMP_RECORD("scene_4_tmp_id: inc*1000 cost");
    }

    //scene 5: scene4的简化版    
    //test/demo/benchmark **推荐** 
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(cost, 1000, "scene5 inc*1000");
        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }
    }

    //scene 6:  存在多个有交集的性能统计中, 可以先记录性能信息, 然后顺序执行写入<临时条目>性能信息并使用标准<临时条目>输出     
    if (true)
    {
        static const int scene_6_tmp_id = ProfInstType::INNER_PROF_NULL;

        PROF_DEFINE_COUNTER(cost);
        PROF_START_COUNTER(cost);

        PROF_DEFINE_COUNTER(inc_cost);
        PROF_START_COUNTER(inc_cost);

        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }
        PROF_STOP_AND_SAVE_COUNTER(inc_cost);

        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t sub = 0;
            sub--;
        }
        PROF_STOP_AND_SAVE_COUNTER(cost);

        PROF_RECORD_CPU(scene_6_tmp_id, cost.duration_ticks());
        PROF_OUTPUT_TEMP_RECORD("scene_6_tmp_id: total cost");

        //带count写入 
        PROF_RECORD_CPU_WRAP(scene_6_tmp_id, 1000, cost.duration_ticks(), RECORD_LEVEL_NORMAL);
        PROF_OUTPUT_TEMP_RECORD("scene_6_tmp_id: per inc");

        //同上 更方便  
        PROF_OUTPUT_MULTI_COUNT_CPU("scene_6_tmp_id: per sub", 1000, cost.duration_ticks() - inc_cost.duration_ticks());

    }


    //scene 7:  以报告形式输出所有当前存在的性能信息  
    printf("%s", "scene 7: output report.\n");
    ProfInst.output_report();

    //scene 8: resident 
    static const int scene_8_resident_id = scene_3_id + 1;
    static const int scene_8_unresident_id = scene_3_id + 2;

    if (true)
    {
        PROF_REGIST_NODE(scene_8_resident_id, "scene_8_resident_id", CLOCK_RDTSC, true, false);
        PROF_REGIST_NODE(scene_8_unresident_id, "scene_8_resident_id", CLOCK_RDTSC, false, false);



        PROF_DEFINE_COUNTER(cost);
        PROF_START_COUNTER(cost);
        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }
        PROF_STOP_AND_SAVE_COUNTER(cost);

        //写入记录信息  
        PROF_RECORD_CPU_WRAP(scene_8_resident_id, 1000, cost.duration_ticks(), RECORD_LEVEL_NORMAL);
        PROF_RECORD_CPU_WRAP(scene_8_unresident_id, 1000, cost.duration_ticks(), RECORD_LEVEL_NORMAL);

        //输出报告(只输出<注册条目> )   
        printf("%s", "scene 8: output report.\n");  
        PROF_OUTPUT_REPORT(OUT_FLAG_DELCARE);

        //清除unresident记录  
        PROF_RESET_DECLARE();

        //输出报告(只输出<注册条目> )   
        printf("%s", "scene 8: output cleaned(unresident)  report.\n");
        PROF_OUTPUT_REPORT(OUT_FLAG_DELCARE);
    }




    //scene 10: pre 清除所有声明条目 
    PROF_RESET_DECLARE(false);

    //scene 10: 注册条目(适合嵌入到实际项目中而非demo/benchmark/test) 
    if (true)
    {
        //定义条目   
        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_ALL_MATH, "all math");
        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_INC, "inc");
        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_SUB, "sub");
        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_MUL, "mul");
        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_DIV, "div");

        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_VM_USE, "self vm use:");
        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_TIMMER, "timer 50ms");
        PROF_FAST_REGIST_NODE_ALIAS(PROF_REG_SELF_SIZE, "self memory size");




        //(可选) 绑定展示层级 all math为父级, inc/sub为子级;  输出报告时自动归并在一起并以缩进方式展示层级关系.  
        PROF_BIND_CHILD(PROF_REG_ALL_MATH, PROF_REG_INC);

        //(可选) 绑定性能信息的合并层级  
        PROF_BIND_MERGE(PROF_REG_ALL_MATH, PROF_REG_INC);

        //(可选) 同时绑定两个层级    
        PROF_BIND_CHILD_AND_MERGE(PROF_REG_ALL_MATH, PROF_REG_SUB);
        PROF_BIND_CHILD_AND_MERGE(PROF_REG_ALL_MATH, PROF_REG_MUL);
        PROF_BIND_CHILD_AND_MERGE(PROF_REG_ALL_MATH, PROF_REG_DIV);


        // (可选)构建跳点优化     
        PROF_BUILD_JUMP_PATH();


        // 记录条目性能数据 
        PROF_DEFINE_COUNTER(cost);

        //性能统计  
        if (true)
        {
            PROF_START_COUNTER(cost);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
            PROF_RECORD_CPU_WRAP(PROF_REG_INC, 1000, cost.stop_and_save().duration_ticks(), RECORD_LEVEL_NORMAL);

            PROF_RESTART_COUNTER(cost);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t sub = 0;
                sub--;
            }
            PROF_RECORD_CPU_WRAP(PROF_REG_SUB, 1000, cost.stop_and_save().duration_ticks(), RECORD_LEVEL_NORMAL);

            PROF_RESTART_COUNTER(cost);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t mul = i;
                mul *= 1000;
            }
            PROF_RECORD_CPU_WRAP(PROF_REG_MUL, 1000, cost.stop_and_save().duration_ticks(), RECORD_LEVEL_NORMAL);

            PROF_RESTART_COUNTER(cost);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t div = i;
                div /= 1000;
            }
            PROF_RECORD_CPU_WRAP(PROF_REG_DIV, 1000, cost.stop_and_save().duration_ticks(), RECORD_LEVEL_NORMAL);
        }

        //vm统计  
        if (true)
        {
            PROF_RESTART_COUNTER(cost);
            //记录当前进程的vm使用情况  
            PROF_RECORD_VM(PROF_REG_VM_USE, get_self_mem());

            //同时可以记录record_vm这行的消耗到同一条目下的cpu消耗信息中 
            PROF_RECORD_CPU_WRAP(PROF_REG_VM_USE, 1, cost.stop_and_save().duration_ticks(), RECORD_LEVEL_NORMAL);
        }

        //记录字节数量  
        if (true)
        {
            PROF_RECORD_MEM(PROF_REG_SELF_SIZE, 1, sizeof(ProfInstType));
        }

        //定时器稳定性
        for (size_t i = 0; i < 5; i++)
        {
            PROF_START_COUNTER(cost);
            PROF_RECORD_TIMER(PROF_REG_TIMMER, cost.start_val());
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }





        // 合并条目   
        PROF_DO_MERGE();

        // 输出报告(只输出<声明条目>)  
        printf("%s", "scene 10 report\n");
        PROF_OUTPUT_REPORT(OUT_FLAG_DELCARE);
    }

    return 0;
}


