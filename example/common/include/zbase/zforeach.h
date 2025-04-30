
/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once 
#ifndef ZFOREACH_H
#define ZFOREACH_H

#include <stdint.h>
#include <memory>
#include <cstddef>

#ifdef WIN32
#pragma warning( push )
#pragma warning(disable : 4200)
#endif // WIN32



//default use format compatible short type .  
#if !defined(ZBASE_USE_AHEAD_TYPE) && !defined(ZBASE_USE_DEFAULT_TYPE)
#define ZBASE_USE_DEFAULT_TYPE
#endif 

//win & unix format incompatible   
#ifdef ZBASE_USE_AHEAD_TYPE
using s8 = int8_t;
using u8 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using s64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
#endif

#ifdef ZBASE_USE_DEFAULT_TYPE
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
#endif


#if __GNUG__
#define ZBASE_ALIAS __attribute__((__may_alias__))
#else
#define ZBASE_ALIAS
#endif



//分帧轮询 削峰  
//note: 不保证间隔绝对精准, 特别是首次触发,  在频繁发生软窗口变化时, 不保证间隔精准, 但保证每次触发的触发窗口边界内.  
//note: 如有精确要求, 在root tick中手动解决(root tick跳帧) , 或者使用定时器,  这里最好是一些超时检测, 定时检测, 延迟处理等需求.  

/* type_traits:
*
* is_trivially_copyable: safely in-process  
    * memset: safely in-process  
    * memcpy: safely in-process  
* shm resume : safely on rebuild hook adrress
    * has vptr:     no
    * static var:   no
    * has heap ptr: no
    * has code ptr: yes (hook) 
    * has sys ptr: no
* thread safe: no
*
*/


namespace zforeach_impl
{
    struct subframe
    {
        using Hook = s32(*)(const subframe&, u32, u32, s64);
        Hook hook_;
        u64 userkey_;
        u64 userdata_;
        u32 sub_steps_;
        u32 hard_begin_;
        u32 hard_end_;
        u32 soft_begin_;
        u32 soft_end_;
        u32 cur_step_;
        u32 foreach_cursor_; //locked cursor when soft window has change   
    };

    static inline s32 init(subframe& sub, u64 userkey, u64 userdata, u32 begin_id, u32 end_id, subframe::Hook hook, s32 base_frame_len, s32 long_frame_len)
    {
        memset(&sub, 0, sizeof(subframe));
        sub.sub_steps_ = 1;
        if (base_frame_len == 0 || long_frame_len == 0)
        {
            return -1;
        }

        
        if ((long_frame_len % base_frame_len) != 0)
        {
            //auto align the nearst frame time 
            //return -2;
        }

        if (long_frame_len < base_frame_len)
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
        sub.userkey_ = userkey;
        sub.userdata_ = userdata;
        sub.hard_begin_ = begin_id;
        sub.hard_end_ = end_id;
        sub.foreach_cursor_ = sub.hard_begin_;
        sub.sub_steps_ = (long_frame_len + (base_frame_len-1)) / base_frame_len;
        sub.soft_begin_ = sub.hard_begin_;
        sub.soft_end_ = sub.hard_end_;
        sub.cur_step_ = 0;
        sub.hook_ = hook;
        return 0;
    }

    static inline bool is_valid(const subframe& sub)
    {
        if (sub.hook_ == NULL)
        {
            return false;
        }
        if (sub.sub_steps_ == 0)
        {
            return false;
        }
        if (sub.soft_begin_ < sub.hard_begin_ || sub.soft_end_ > sub.hard_end_)
        {
            return false;
        }
        return true;
    }

    static inline s32 window_foreach(subframe& sub, s64 now_ms)
    {
        if (!is_valid(sub))
        {
            return -1;
        }
        sub.cur_step_ = (sub.cur_step_ + 1) % sub.sub_steps_;
        u32 cur_begin_id = sub.foreach_cursor_;
        u32 cur_end_id = sub.foreach_cursor_ + (sub.soft_end_ - sub.soft_begin_ + sub.sub_steps_ - 1) / sub.sub_steps_ ;
        if (cur_end_id > sub.soft_end_)
        {
            cur_end_id = sub.soft_end_;
        }

        //当前为最后一轮, 设置下一次开始位置 
        if (sub.cur_step_ == 0)
        {
            sub.foreach_cursor_ = sub.soft_begin_;
            cur_end_id = sub.soft_end_;
        }
        else
        {
            sub.foreach_cursor_ = cur_end_id;
        }
        //如果发生异常 resume后会开始下一轮   

        sub.hook_(sub, cur_begin_id, cur_end_id, now_ms);
        return 0;
    }

}


template<class _THookObj>
class zforeach
{
public:
    zforeach_impl::subframe subframe_;
    _THookObj foreach_inst_;
    static_assert(!std::is_polymorphic<_THookObj>::value, "zforeach not resume _THookObj vptr. so don't use polymorphic class.");
public:
    inline s32 init(u64 userkey, u32 begin_id, u32 end_id, u32 base_frame_len, u32 long_frame_len)
    {
        s32 ret = zforeach_impl::init(subframe_, userkey, (u64)(void*)this, begin_id, end_id, &zforeach<_THookObj>::global_hook, base_frame_len, long_frame_len);
        if (ret != 0)
        {
            return ret;
        }
        return 0;
    }

    inline s32 resume(const _THookObj& inst)
    {
        foreach_inst_ = inst;
        subframe_.hook_ = &zforeach<_THookObj>::global_hook;
        return 0;
    }
    inline s32 resume()
    {
        subframe_.hook_ = &zforeach<_THookObj>::global_hook;
        return 0;
    }
    inline s32 window_foreach(u32 win_begin, u32 win_end, s64 now_ms)
    { 
        //hard bound auto fixed.   
        subframe_.soft_begin_ = win_begin < subframe_.hard_begin_ ? subframe_.hard_begin_ : win_begin;
        subframe_.soft_end_ = win_end > subframe_.hard_end_? subframe_.hard_end_: win_end;
        return zforeach_impl::window_foreach(subframe_, now_ms);
    }


private:
    static inline s32 global_hook(const zforeach_impl::subframe& sub, u32 begin_id, u32 end_id, s64 now_ms)
    {
        zforeach<_THookObj>* inst = reinterpret_cast<zforeach<_THookObj>*>(sub.userdata_);
        if (inst == NULL)
        {
            return -1;
        }
        s32 ret = inst->foreach_inst_.hook(sub, begin_id, end_id, now_ms);
        return ret;
    }
};









#ifdef WIN32
#pragma warning( pop  )
#endif // WIN32
#endif