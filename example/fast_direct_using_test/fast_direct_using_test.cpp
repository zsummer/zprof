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
    PROF_REG_BEGIN = ProfInstType::declare_begin_id() + 10, //reserve 10;  it's used scene 1~7 case  
    PROF_REG_ALL_MATH,
    PROF_REG_INC,
    PROF_REG_SUB,
    PROF_REG_MUL,
    PROF_REG_DIV,
    PROF_REG_VM_USE,
    PROF_REG_TIMMER,
    PROF_REG_SELF_SIZE,
};

int main(int argc, char *argv[])
{
    //初始化   
    ProfInst.init("fast_direct_using");
    
    //scene 1  
    //手动使用计时器并直接使用标准时间单位的耗时      
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
        printf("scene 1: inc * 1000 used:%lld ns \n", counter.duration_ns());
        printf("scene 1: inc avg used:%g ns \n", counter.duration_ns()/1000.0);
    }





    //scene 2  
    //手动使用计时器 并记录到<注册条目>中 然后立刻输出该<条目>报告     
    static const int scene_2_id = ProfInst.declare_begin_id();
    if (true)
    {
        //注册ID  
        ProfInst.regist(scene_2_id, "scene 2", PROF_COUNTER_RDTSC, false, false);

        //计算耗时 
        ProfCounter<> cost;
        cost.start();

        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }

        cost.stop_and_save();

        //记录到条目
        ProfInst.record_cpu(scene_2_id, 1, cost.cycles());

        //立刻输出条目 
        ProfInst.output_one_record(scene_2_id);
    }



    //scene 3:  scene2的糖  
    static const int scene_3_id = scene_2_id + 1;
    if (true)
    {
        //注册ID 
        ProfInst.regist(scene_3_id, "scene 3", PROF_COUNTER_RDTSC, false, false);

        //计算耗时并记录到scene_3_id 
        if (true)
        {
            ProfAutoRecord<> record(scene_3_id);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
        }

        //立刻输出条目 
        ProfInst.output_one_record(scene_3_id);
    }


    //scene 4: 使用<临时条目>输出性能信息 而不新增注册条目    
    if (true)
    {
        static const int scene_4_tmp_id = ProfInstType::INNER_PROF_NULL;
        if (true)
        {
            ProfAutoRecord<> record(scene_4_tmp_id);
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
        }

        //<临时条目>输出 调用该接口用于输出带<条目信息>的性能信息, 并且随后自动清理性能信息防止污染下次使用    
        ProfInst.output_temp_record("scene_4_tmp_id: inc*1000 cost");
    }

    //scene 5: scene4的简化版    
    //test/demo/benchmark **推荐** 
    if (true)
    {
        ProfAutoAnonRecord<1000> var("scene5 inc*1000");

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

        ProfCounter<> cost;
        cost.start();

        ProfCounter<> inc_cost;
        inc_cost.start();

        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }

        inc_cost.stop_and_save();

        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t sub = 0;
            sub--;
        }
        cost.stop_and_save();
        
        ProfInst.record_cpu(scene_6_tmp_id, 1, cost.cycles());
        ProfInst.output_temp_record("scene_6_tmp_id: total cost");

        ProfInst.record_cpu(scene_6_tmp_id, 1000, inc_cost.cycles());
        ProfInst.output_temp_record("scene_6_tmp_id: per inc");

        ProfInst.record_cpu(scene_6_tmp_id, 1000, cost.cycles() - inc_cost.cycles());
        ProfInst.output_temp_record("scene_6_tmp_id: per sub");
    }


    //scene 7:  以报告形式输出所有当前存在的性能信息  
    printf("%s", "scene 7: output report.\n");
    ProfInst.output_report();

    //scene 8: resident 
    static const int scene_8_resident_id = scene_3_id + 1;
    static const int scene_8_unresident_id = scene_3_id + 2;

    if (true)
    {
        ProfInst.regist(scene_8_resident_id, "scene_8_resident_id", PROF_COUNTER_RDTSC, true, false);
        ProfInst.regist(scene_8_unresident_id, "scene_8_resident_id", PROF_COUNTER_RDTSC, false, false);

        ProfCounter<> cost;
        cost.start();
        for (size_t i = 0; i < 1000; i++)
        {
            volatile size_t inc = 0;
            inc++;
        }
        cost.stop_and_save();
        //写入记录信息  
        ProfInst.record_cpu(scene_8_resident_id, 1000, cost.cycles());
        ProfInst.record_cpu(scene_8_unresident_id, 1000, cost.cycles());
        //输出报告(只输出<注册条目> )   
        printf("%s", "scene 8: output report.\n");
        ProfInst.output_report(PROF_OUTPUT_FLAG_DELCARE);
        //清除unresident记录  
        ProfInst.reset_declare_node(true);
        //输出报告(只输出<注册条目> )   
        printf("%s", "scene 8: output cleaned(unresident)  report.\n");
        ProfInst.output_report(PROF_OUTPUT_FLAG_DELCARE);
    }




    //scene 10: pre 清除所有声明条目 
    ProfInst.reset_declare_node(false);


    //scene 10: 注册条目(适合嵌入到实际项目中而非demo/benchmark/test) 
    if (true)
    {
        //定义条目   
        ProfInst.regist(PROF_REG_ALL_MATH, "all math", PROF_COUNTER_RDTSC, false, false);
        ProfInst.regist(PROF_REG_INC, "inc", PROF_COUNTER_RDTSC, false, false);
        ProfInst.regist(PROF_REG_SUB, "sub", PROF_COUNTER_RDTSC, false, false);
        ProfInst.regist(PROF_REG_MUL, "mul", PROF_COUNTER_RDTSC, false, false);
        ProfInst.regist(PROF_REG_DIV, "div", PROF_COUNTER_RDTSC, false, false);

        ProfInst.regist(PROF_REG_VM_USE, "self vm use:", PROF_COUNTER_RDTSC, false, false);
        ProfInst.regist(PROF_REG_TIMMER, "timer 50ms", PROF_COUNTER_RDTSC, false, false);
        ProfInst.regist(PROF_REG_SELF_SIZE, "self memory size", PROF_COUNTER_RDTSC, false, false);



        //(可选) 绑定展示层级 all math为父级, inc/sub为子级;  输出报告时自动归并在一起并以缩进方式展示层级关系.  
        ProfInst.bind_childs(PROF_REG_ALL_MATH, PROF_REG_INC);
        ProfInst.bind_childs(PROF_REG_ALL_MATH, PROF_REG_SUB);
        ProfInst.bind_childs(PROF_REG_ALL_MATH, PROF_REG_MUL);
        ProfInst.bind_childs(PROF_REG_ALL_MATH, PROF_REG_DIV);

        //(可选) 绑定性能信息的合并层级  
        ProfInst.bind_merge(PROF_REG_ALL_MATH, PROF_REG_INC);
        ProfInst.bind_merge(PROF_REG_ALL_MATH, PROF_REG_SUB);
        ProfInst.bind_merge(PROF_REG_ALL_MATH, PROF_REG_MUL);
        ProfInst.bind_merge(PROF_REG_ALL_MATH, PROF_REG_DIV);

        // (可选)构建跳点优化     
        ProfInst.build_jump_path();

        // 记录条目性能数据 
    
        ProfCounter<> cost;
        //性能统计  
        if (true)
        {
            cost.start();
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t inc = 0;
                inc++;
            }
            ProfInst.record_cpu(PROF_REG_INC, 1000, cost.stop_and_save().cycles());

            cost.start();
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t sub = 0;
                sub--;
            }
            ProfInst.record_cpu(PROF_REG_SUB, 1000, cost.stop_and_save().cycles());

            cost.start();
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t mul = i;
                mul*=1000;
            }
            ProfInst.record_cpu(PROF_REG_MUL, 1000, cost.stop_and_save().cycles());

            cost.start();
            for (size_t i = 0; i < 1000; i++)
            {
                volatile size_t div = i;
                div /= 1000;
            }
            ProfInst.record_cpu(PROF_REG_DIV, 1000, cost.stop_and_save().cycles());
        }

        //vm统计  
        if (true)
        {
            cost.start();
            //记录当前进程的vm使用情况  
            ProfInst.record_vm(PROF_REG_VM_USE, prof_get_mem_use());

            //同时可以记录record_vm这行的消耗到同一条目下的cpu消耗信息中 
            ProfInst.record_cpu(PROF_REG_VM_USE, cost.stop_and_save().cycles());
        }

        //记录字节数量  
        if (true)
        {
            ProfInst.record_mem(PROF_REG_SELF_SIZE, 1, sizeof(ProfInstType));
        }

        //定时器稳定性
        for (size_t i = 0; i < 5; i++)
        {
            cost.start();
            ProfInst.record_timer(PROF_REG_TIMMER, cost.start_val());
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }





        // 合并条目数据     
        ProfInst.do_merge();
        // 
        // 输出报告(只输出<声明条目>)  
        printf("%s", "scene 10 report\n");
        ProfInst.output_report(PROF_OUTPUT_FLAG_DELCARE);
    }

    return 0;
}


