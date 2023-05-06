/*
* zforeach License
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


#include <memory>
#include <cstddef>

#ifndef ZFOREACH_H
#define ZFOREACH_H

#ifdef WIN32
#pragma warning( push )
#pragma warning(disable : 4200)
#endif // WIN32

using s8 = char;
using u8 = unsigned char;
using s16 = short int;
using u16 = unsigned short int;
using s32 = int;
using u32 = unsigned int;
using s64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;

//分帧分段驱动大间隔轮询 削峰  
//note: 不保证间隔绝对精准, 特别是首次触发,  在频繁发生软窗口变化时, 不保证间隔精准, 但保证每次触发的触发窗口边界内.  
//note: 如有精确要求, 在root tick中手动解决(root tick跳帧) , 或者使用定时器,  这里最好是一些超时检测, 定时检测, 延迟处理等需求.  

namespace zforeach_impl
{
    struct subframe
    {
        using Hook = s32(*)(const subframe&, u32, u32, s64);
        Hook hook_;
        u64 userkey_;
        u64 userdata_;
        u32 segments_;
        u32 hard_begin_;
        u32 hard_end_;
        u32 soft_begin_;
        u32 soft_end_;
        u32 tick_;
        u32 next_id_; //locked cursor when soft window has change   
    };

    static inline s32 init(subframe& sub)
    {
        memset(&sub, 0, sizeof(subframe));
        sub.segments_ = 1;
        return 0;
    }

    static inline bool is_valid(const subframe& sub)
    {
        if (sub.hook_ == NULL)
        {
            return false;
        }
        if (sub.segments_ == 0)
        {
            return false;
        }
        if (sub.soft_begin_ < sub.hard_begin_ || sub.soft_end_ > sub.hard_end_)
        {
            return false;
        }
        return true;
    }

    static inline s32 window_tick(subframe& sub, s64 now_ms)
    {
        if (!is_valid(sub))
        {
            return -1;
        }
        sub.tick_ = (sub.tick_ + 1) % sub.segments_;
        u32 begin_id = sub.next_id_;
        u32 end_id = sub.next_id_ + (sub.soft_end_ - sub.soft_begin_ + sub.segments_ - 1) / sub.segments_ ;
        if (end_id > sub.soft_end_)
        {
            end_id = sub.soft_end_;
        }

        //当前为最后一轮, 设置下一次开始位置 
        if (sub.tick_ == 0)
        {
            sub.next_id_ = sub.soft_begin_;
            end_id = sub.soft_end_;
        }
        else
        {
            sub.next_id_ = end_id;
        }

        sub.hook_(sub, begin_id, end_id, now_ms);
        return 0;
    }

    static inline s32 root_tick(subframe& sub, s64 now_ms)
    {
        sub.soft_begin_ = sub.hard_begin_;
        sub.soft_end_ = sub.hard_end_;
        return window_tick(sub, now_ms);
    }
}

class zforeach
{
public:
    inline s32 init(u64 userkey, u64 userdata, u32 begin_id, u32 end_id, zforeach_impl::subframe::Hook hook, u32 base_tick, u32 foreach_tick)
    {
        zforeach_impl::init(subframe_);
        if (base_tick == 0 || foreach_tick == 0)
        {
            return -1;
        }
        if ((foreach_tick % base_tick) != 0)
        {
            return -2;
        }
        if (foreach_tick < base_tick)
        {
            return -3;
        }
        if (hook == NULL)
        {
            return -4;
        }
        if (end_id < begin_id)
        {
            return -5;
        }
        subframe_.userkey_ = userkey;
        subframe_.userdata_ = userdata;
        subframe_.hard_begin_ = begin_id;
        subframe_.hard_end_ = end_id;
        subframe_.next_id_ = subframe_.hard_begin_;
        subframe_.segments_ = foreach_tick / base_tick;
        subframe_.soft_begin_ = subframe_.hard_begin_;
        subframe_.soft_end_ = subframe_.hard_end_;
        subframe_.tick_ = 0;
        subframe_.hook_ = hook;
        return 0;
    }
    inline s32 window_tick(u32 begin_id, u32 end_id, s64 now_ms)
    { 
        subframe_.soft_begin_ = begin_id;
        subframe_.soft_end_ = end_id;
        return zforeach_impl::window_tick(subframe_, now_ms);
    }
    zforeach_impl::subframe subframe_;
};






//foreach




#ifdef WIN32
#pragma warning( pop  )
#endif // WIN32
#endif