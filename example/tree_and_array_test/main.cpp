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
#include "test.h"
#include <unordered_map>
#include <unordered_set>
#include "zhash_map.h"
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
struct EntityCell
{
    s32 red_cell_;
    s32 orange_cell_;
    s32 green_cell_;
    std::unordered_set<u64> player_refs;
    std::unordered_set<u64> npc_refs;
};

inline s32 AdaptAxisIndex(float axis, int aligning) { return (int)axis / aligning; }
inline u64 AdaptStoreIndex(s32 x, s32 y) { return (((u64)(u32)x) << 32) + (u32)y; }
inline u64 AdaptStoreIndex(float x, float y, int aligning) { return AdaptStoreIndex(AdaptAxisIndex(x, aligning), AdaptAxisIndex(y, aligning)); }

class FastHash
{
public:
    u64 operator()(u64 input)
    {
        return (((input >> 32) * 73856093) ^ ((input & 0xffffffff) * 19349663));
    }
};



int main(int argc, char *argv[])
{
    PROF_INIT("inner prof");
    //ProfInst.init_prof("inner prof");
    regist_prof();
    PROF_DEFINE_AUTO_ANON_RECORD(delta, "self use mem in main func begin and exit");
    PROF_OUTPUT_SELF_MEM("self use mem in main func begin");
    if (true)
    {
        PROF_DEFINE_AUTO_ANON_RECORD(guard, "start fnlog use");
        FNLog::FastStartDebugLogger();
    }

    LogDebug() << " main begin test. ";
    volatile double cycles = 0.0f;

    std::unordered_map<u64, EntityCell> entity_unordered_map;
    zsummer::shm_arena::zhash_map<u64, EntityCell, 1048576, FastHash> * entity_hash_map_ptr = new zsummer::shm_arena::zhash_map<u64, EntityCell, 1048576, FastHash>();
    zsummer::shm_arena::zhash_map<u64, EntityCell, 1048576, FastHash>& entity_hash_map = *entity_hash_map_ptr;
    std::map<u64, EntityCell> entity_map;
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_unordered_map insert cost ");
        for (float i = 0; i < 800; i += 1.0)
        {

            for (float j = 0; j < 800; j += 1.0)
            {
                EntityCell& cell = entity_unordered_map[AdaptStoreIndex(i, j, 1)];
                cell.red_cell_ = (int)i;
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_unordered_map grid view cost ");
        cycles = 0;
        for (float i = 0; i < 800; i += 1.0)
        {
            for (float j = 0; j < 800; j += 1.0)
            {
                for (int k = 0; k < 9; k++)
                {
                    s32 x = AdaptAxisIndex(i, 1);
                    s32 y = AdaptAxisIndex(j, 1);
                    u64 cur_id = AdaptStoreIndex(x + (k / 3) - 1, y + (k % 3) - 1);
                    auto iter = entity_unordered_map.find(cur_id);
                    if (iter != entity_unordered_map.end())
                    {
                        cycles += iter->second.red_cell_;
                    }
                }
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_unordered_map erase cost ");
        for (float i = 0; i < 800; i += 1.0)
        {
            for (float j = 0; j < 800; j += 1.0)
            {
                auto iter = entity_unordered_map.find(AdaptStoreIndex(i, j, 1));
                if (iter == entity_unordered_map.end())
                {
                    LogError() << "not found grid. i:" << i << ", j" << j << ", id:" << AdaptStoreIndex(i, j, 1);
                    continue;
                }
                entity_unordered_map.erase(iter);
            }
        }
        if (!entity_unordered_map.empty())
        {
            LogError() << "map not clean";
        }
    }



    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_hash_map insert cost ");
        for (float i = 0; i < 800; i += 1.0)
        {

            for (float j = 0; j < 800; j += 1.0)
            {
                EntityCell& cell = entity_hash_map[AdaptStoreIndex(i, j, 1)];
                cell.red_cell_ = (int)i;
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_hash_map grid view cost ");
        cycles = 0;
        for (float i = 0; i < 800; i += 1.0)
        {
            for (float j = 0; j < 800; j += 1.0)
            {
                for (int k = 0; k < 9; k++)
                {
                    s32 x = AdaptAxisIndex(i, 1);
                    s32 y = AdaptAxisIndex(j, 1);
                    u64 cur_id = AdaptStoreIndex(x + (k / 3) - 1, y + (k % 3) - 1);
                    auto iter = entity_hash_map.find(cur_id);
                    if (iter != entity_hash_map.end())
                    {
                        cycles += iter->second.red_cell_;
                    }
                }
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_hash_map erase cost ");
        for (float i = 0; i < 800; i += 1.0)
        {
            for (float j = 0; j < 800; j += 1.0)
            {
                auto iter = entity_hash_map.find(AdaptStoreIndex(i, j, 1));
                if (iter == entity_hash_map.end())
                {
                    LogError() << "not found grid. i:" << i << ", j" << j << ", id:" << AdaptStoreIndex(i, j, 1);
                    continue;
                }
                entity_hash_map.erase(iter);
            }
        }
        if (!entity_hash_map.empty())
        {
            LogError() << "map not clean";
        }
    }


    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_map insert cost ");
        for (float i = 0; i < 800; i += 1.0)
        {

            for (float j = 0; j < 800; j += 1.0)
            {
                EntityCell& cell = entity_map[AdaptStoreIndex(i, j, 1)];
                cell.red_cell_ = (int)i;
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_map grid view cost ");
        cycles = 0;
        for (float i = 0; i < 800; i += 1.0)
        {
            for (float j = 0; j < 800; j += 1.0)
            {
                for (int k = 0; k < 9; k++)
                {
                    s32 x = AdaptAxisIndex(i, 1);
                    s32 y = AdaptAxisIndex(j, 1);
                    u64 cur_id = AdaptStoreIndex(x + (k / 3) - 1, y + (k % 3) - 1);
                    auto iter = entity_map.find(cur_id);
                    if (iter != entity_map.end())
                    {
                        cycles += iter->second.red_cell_;
                    }
                }
            }
        }
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 800 * 800, "entity_map erase cost ");
        for (float i = 0; i < 800; i += 1.0)
        {
            for (float j = 0; j < 800; j += 1.0)
            {
                auto iter = entity_map.find(AdaptStoreIndex(i, j, 1));
                if (iter == entity_map.end())
                {
                    LogError() << "not found grid. i:" << i << ", j" << j << ", id:" << AdaptStoreIndex(i, j, 1);
                    continue;
                }
                entity_map.erase(iter);
            }
        }
        if (!entity_map.empty())
        {
            LogError() << "map not clean";
        }
    }


    
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "std::hash<u64>");
        volatile size_t ret = 0;
        std::hash<u64> h;
        volatile int loop_count = 10000;
        for (int i = 0; i < loop_count; i++)
        {
            ret += h(i) % 16384;
        }
        cycles += ret;
    }
    if (true)
    {
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "x,y hash");
        volatile size_t ret = 0;
        volatile int loop_count = 10000;
        for (int i = 0; i < loop_count; i ++)
        {
            ret += ((i * 73856093) ^ (i * 19349663)) & (16384 - 1);
        }
        cycles += ret;
    }
    if (true)
    {
        auto hash = [](int x, int y, u64 bucket_size)
        {
            return (((unsigned int)x * 73856093) ^ ((unsigned int)y * 19349663)) & (bucket_size - 1);
        };
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "detour x y hash");
        volatile size_t ret = 0;
        volatile int loop_count = 10000;
        for (int i = 0; i < loop_count; i++)
        {
            ret += hash(i,i, 16384);
        }
        cycles += ret;
    }
    if (true)
    {
        auto hash = [](u64 input, u64 bucket_size)
        {
            return (((input >> 32) * 73856093) ^ ((input & 0xffffffff) * 19349663)) & (bucket_size - 1);
        };
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "detour u64 hash");
        volatile size_t ret = 0;
        volatile int loop_count = 10000;
        for (int i = 0; i < loop_count; i++)
        {
            ret += hash(i, 16384);
        }
        cycles += ret;
    }

    if (true)
    {
        auto hash = [](u64 input, u64 bucket_size)
        {
            input ^= input >> 23;
            input *= 0x2127599bf4325c37ULL;
            input ^= input >> 47;
            return input & (bucket_size - 1);
        };
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "Xorshifts and one multiplication");
        volatile size_t ret = 0;
        volatile int loop_count = 10000;
        for (int i = 0; i < loop_count; i++)
        {
            ret += hash(i, 16384);
        }
        cycles += ret;
    }
    if (true)
    {
        std::map<std::string, size_t> map_string;
        std::unordered_map<std::string, size_t> hashmap_string;
        std::vector<std::string> datas;
        for (int i = 0; i < 10000; i++)
        {
            char buf[50];
            sprintf(buf, "afd%04d", i);
            datas.push_back(buf);
        }
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "map_string insert 10000");
            for (size_t i = 0; i < 10000; i++)
            {
                map_string[datas[i]] = i;
            }
        }
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "hashmap_string insert 10000");
            for (size_t i = 0; i < 10000; i++)
            {
                hashmap_string[datas[i]] = i;
            }
        }

        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "map_string find 10000");
            for (size_t i = 0; i < 10000; i++)
            {
                cycles += map_string.at(datas[i]);
            }
        }

        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "hashmap_string find 10000");
            for (size_t i = 0; i < 10000; i++)
            {
                cycles += hashmap_string.at(datas[i]);
            }
        }

        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "map_string erase 10000");
            for (size_t i = 0; i < 10000; i++)
            {
                map_string.erase(datas[i]);
            }
        }
        if (true)
        {
            PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "hashmap_string erase 10000");
            for (size_t i = 0; i < 10000; i++)
            {
                hashmap_string.erase(datas[i]);
            }
        }

    }

    if (true)
    {
        auto hash = [](u64 input, u64 bucket_size)
        {
            input *= 0xc6a4a7935bd1e995ULL;
            input ^= input >> 47;
            input *= 0xc6a4a7935bd1e995ULL;
            return input & (bucket_size - 1);
        };
        PROF_DEFINE_AUTO_MULTI_ANON_RECORD(guard, 10000, "Xorshifts and two multiplication");
        volatile size_t ret = 0;
        volatile int loop_count = 10000;
        for (int i = 0; i < loop_count; i++)
        {
            ret += hash(i, 16384);
        }
        cycles += ret;
    }







    PROF_SERIALIZE_FN_LOG();

 
    if (true)
    {
        LogDebug() << "std::hash<unsigned long long>()(1000):" << std::hash<unsigned long long>()(1000) << "std::hash<unsigned long long>()(30):" << std::hash<unsigned long long>()(30);
        std::hash<unsigned long long> h;
        LogDebug() << "std::hash<unsigned long long> h; h(1000):" << h(1000) << "h(30):" << h(30);
    }

    LogInfo() << "all test finish .salt:" << cycles;
    return 0;
}


