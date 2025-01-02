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

typedef char s8;
typedef unsigned char u8;
typedef short int s16;
typedef unsigned short int u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;
typedef unsigned long pointer;
typedef float f32;




int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");

    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;


    int matrix_x = 16 * 1024;
    int matrix_y = 16 * 1024;
    int matrix_size = matrix_x * matrix_y;

    char* matrix = new char[matrix_size]; //256M
    for (int i = 0; i < matrix_size; i++)
    {
        matrix[i] = (char)(i % 256);
    }

    long long ordered_sum = 0;
    long long ordered_cicles = 0;

    long long jump_sum = 0;
    long long jump_cicles = 0;

    long long empty_sum = 0;
    long long empty_cicles = 0;

    PROF_DEFINE_COUNTER(cost);
    cost.Start();
    for (long long x = 0; x < matrix_x; x++)
    {
        for (long long y = 0; y < matrix_y; y++)
        {
            ordered_sum += *(matrix + x * matrix_y + y);
        }
    }
    ordered_cicles = cost.StopAndSave().cost();


    cost.Start();
    for (long long x = 0; x < matrix_x; x++)
    {
        for (long long y = 0; y < matrix_y; y++)
        {
            jump_sum += *(matrix + y * matrix_y + x);
        }
    }
    jump_cicles = cost.StopAndSave().cost();

    if (jump_sum != ordered_sum)
    {
        LogError() << "test has error";
        return -1;
    }
    LogInfo() << "cpu cache: order sum matrix use cicles:" << ordered_cicles << ", read use cycles:" << ordered_cicles * 1.0 / matrix_size << "per item";
    LogInfo() << "cpu cache out order sum matrix use cicles:" << jump_cicles    << ", read use cycles:" << jump_cicles * 1.0 / matrix_size << "per item";

    ordered_sum = 0;
    ordered_cicles = 0;
    jump_sum = 0;
    jump_cicles = 0;
    

#define CELL_SIZE 64
    cost.Start();
    for (long long x = 0; x < matrix_x; x++)
    {
        for (long long y = 0; y < matrix_y; y++)
        {
            long long new_block_x = x / CELL_SIZE;
            long long new_block_y = y / CELL_SIZE;
            long long new_cell_x = x % CELL_SIZE;
            long long new_cell_y = y % CELL_SIZE;
            ordered_sum += *(matrix + (new_block_x * matrix_y / CELL_SIZE + new_block_y) * CELL_SIZE + new_cell_x * CELL_SIZE + new_cell_y);
        }
    }
    ordered_cicles = cost.StopAndSave().cost();

    cost.Start();
    for (long long y = 0; y < matrix_y; y++)
    {
        for (long long x = 0; x < matrix_x; x++)
        {
            long long new_block_x = x / CELL_SIZE;
            long long new_block_y = y / CELL_SIZE;
            long long new_cell_x = x % CELL_SIZE;
            long long new_cell_y = y % CELL_SIZE;
            jump_sum += *(matrix + (new_block_x * matrix_y / CELL_SIZE + new_block_y) * CELL_SIZE + new_cell_x * CELL_SIZE + new_cell_y);
        }
    }
    jump_cicles = cost.StopAndSave().cost();

    if (jump_sum != ordered_sum)
    {
        LogError() << "test has error";
        return -1;
    }
    LogInfo() << "cpu cache block : order sum matrix  use cicles:" << ordered_cicles << ", read use cycles:" << ordered_cicles * 1.0 / matrix_size << "per item";
    LogInfo() << "cpu cache block: out order sum matrix cicles:" << jump_cicles << ", read use cycles:" << jump_cicles * 1.0 / matrix_size << "per item";






    long long x = 0;
    long long sign_x = 1;
    long long y = 0;
    long long sign_y = 1;
    long long step_x = 1;
    long long step_y = 2;

    ordered_sum = 0;
    ordered_cicles = 0;
    jump_sum = 0;
    jump_cicles = 0;

    cost.Start();
    for (long long i = 0; i < matrix_y; i++)
    {
        x += step_x * sign_x;
        y += step_y * sign_y;
        if (x >= matrix_x && sign_x > 0)
        {
            sign_x = -1;
            x = matrix_x - 1;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (x < 0 && sign_x < 0)
        {
            sign_x = 1;
            x = 0;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (y >= matrix_y && sign_y > 0)
        {
            sign_y = -1;
            y = matrix_y - 1;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (y < 0 && sign_y < 0)
        {
            sign_y = 1;
            y = 0;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }


        long long new_block_x = x / CELL_SIZE;
        long long new_block_y = y / CELL_SIZE;
        long long new_cell_x = x % CELL_SIZE;
        long long new_cell_y = y % CELL_SIZE;
        ordered_sum += *(matrix + (new_block_x * matrix_y / CELL_SIZE + new_block_y) * CELL_SIZE + new_cell_x * CELL_SIZE + new_cell_y);
    }

    ordered_cicles = cost.StopAndSave().cost();

    cost.Start();
    for (long long i = 0; i < matrix_y; i++)
    {
        x += step_x * sign_x;
        y += step_y * sign_y;
        if (x >= matrix_x && sign_x > 0)
        {
            sign_x = -1;
            x = matrix_x - 1;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (x < 0 && sign_x < 0)
        {
            sign_x = 1;
            x = 0;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (y >= matrix_y && sign_y > 0)
        {
            sign_y = -1;
            y = matrix_y - 1;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (y < 0 && sign_y < 0)
        {
            sign_y = 1;
            y = 0;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        jump_sum += *(matrix + y * matrix_y + x);
    }
    jump_cicles = cost.StopAndSave().cost();



    cost.Start();
    for (long long i = 0; i < matrix_y; i++)
    {
        x += step_x * sign_x;
        y += step_y * sign_y;
        if (x >= matrix_x && sign_x > 0)
        {
            sign_x = -1;
            x = matrix_x - 1;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (x < 0 && sign_x < 0)
        {
            sign_x = 1;
            x = 0;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (y >= matrix_y && sign_y > 0)
        {
            sign_y = -1;
            y = matrix_y - 1;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        if (y < 0 && sign_y < 0)
        {
            sign_y = 1;
            y = 0;
            step_x = rand() % 2 + 1;
            step_y = rand() % 2 + 1;
            continue;
        }
        empty_sum += x + y;
    }
    empty_cicles = cost.StopAndSave().cost();


    LogInfo() << "cpu cache brownian empty  use cicles:" << empty_cicles << ", read use cycles:" << empty_cicles * 1.0 / matrix_y << "per item";
    LogInfo() << "cpu cache brownian block  use cicles:" << ordered_cicles << ", read use cycles:" << ordered_cicles * 1.0 / matrix_y << "per item";
    LogInfo() << "cpu cache brownian normal use cicles:" << jump_cicles << ", read use cycles:" << jump_cicles * 1.0 / matrix_y << "per item";




    
    LogInfo() << "all test finish .salt:" << cycles + jump_sum + ordered_sum + empty_sum + x + y ;
    return 0;
}


