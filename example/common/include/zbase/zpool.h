
/*
* zpool License
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



#ifndef  ZPOOL_H
#define ZPOOL_H
#include <string.h>
#include <type_traits>
#include <cstddef>


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



class zpool
{
public:
    u32 chunk_size_;
    u32 chunk_count_;
    u32 chunk_exploit_offset_;
    u32 chunk_used_count_;
    u32 chunk_free_id_;
    u32 placeholder_;
    char space_[1];
public:
    //the real size need minus sizeof(block_[1]);  
    constexpr static u32 align_chunk_size(u32 mem_size, u32 align_size) { return ((mem_size == 0 ? 1 : mem_size) + align_size - 1) / align_size * align_size; }
    constexpr static u32 calculate_total_size(u32 mem_size, u32 align_size, u32 mem_count) { return align_chunk_size(mem_size, align_size) * mem_count + sizeof(zpool); }
    inline void init(u32 mem_size, u32 align_size, u32 mem_count)
    {
        //clear head. 
        memset(this, 0, sizeof(zpool));
        chunk_size_ = align_chunk_size(mem_size, align_size);
        chunk_count_ = mem_count;
        chunk_free_id_ = (u32)-1;
    }

    inline char* first_chunk() { return &space_[0]; }
    inline const char* first_chunk() const { return &space_[0]; }
    inline u32   chunk_size() const { return chunk_size_; }
    inline u32   max_size()const { return chunk_count_; }
    inline u32   window_size() const { return chunk_exploit_offset_; } //history's largest end id for used.     
    inline u32   size()const { return chunk_used_count_; }
    inline u32   empty()const { return chunk_used_count_ == 0; }
    inline u32   full()const { return chunk_used_count_ == chunk_count_; }

    inline void* exploit()
    {
        if (chunk_free_id_ != (u32)-1)
        {
            u32* p = (u32*)&space_[chunk_free_id_ * chunk_size_];
            chunk_free_id_ = *p;
            chunk_used_count_++;
#ifdef ZDEBUG_UNINIT_MEMORY
            memset(p, 0xfd, chunk_size_);
#endif // ZDEBUG_UNINIT_MEMORY
            return (void*)p;
        }
        if (chunk_exploit_offset_ < chunk_count_)
        {
            void* p = &space_[chunk_exploit_offset_ * chunk_size_];
            chunk_exploit_offset_++;
            chunk_used_count_++;
#ifdef ZDEBUG_UNINIT_MEMORY
            memset(p, 0xfd, chunk_size_);
#endif // ZDEBUG_UNINIT_MEMORY
            return (void*)p;
        }
        return NULL;
    }

    inline void back(void* addr)
    {
#ifdef ZDEBUG_DEATH_MEMORY
        memset(addr, 0xfd, chunk_size_);
#endif // ZDEBUG_DEATH_MEMORY
        u32 id = (u32)((char*)addr - &space_[0]) / chunk_size_;
        u32* p = (u32*)addr;
        *p = chunk_free_id_;
        chunk_free_id_ = id;
        chunk_used_count_--;
    }

    template<class _Ty>
    inline _Ty* create_without_construct()
    {
        return (_Ty * )exploit();
    }

    template<class _Ty >
    inline typename std::enable_if <std::is_trivial<_Ty>::value, _Ty>::type* create()
    {
        return (_Ty*)exploit();
    }

    template<class _Ty >
    inline typename std::enable_if <!std::is_trivial<_Ty>::value, _Ty>::type* create()
    {
        void* p = exploit();
        if (p == NULL)
        {
            return NULL;
        }
        return new (p) _Ty();
    }


    template<class _Ty, class... Args >
    inline _Ty* create(Args&&... args)
    {
        void* p = exploit();
        if (p == NULL)
        {
            return NULL;
        }
        return new (p) _Ty(std::forward<Args>(args) ...);
    }

    template<class _Ty>
    inline void destroy(const typename std::enable_if <std::is_trivial<_Ty>::value, _Ty>::type * obj)
    {
        back(obj);
    }
    template<class _Ty>
    inline void destroy(const typename std::enable_if <!std::is_trivial<_Ty>::value, _Ty>::type* obj)
    {
        obj->~_Ty();
        back(obj);
    }
};



template<u32 MEM_MIN_SIZE, u32 MEM_COUNT, u32 MEM_ALIGN_SIZE = sizeof(u32)>
class zpool_static
{
public:
    inline void init()
    {
        ref().init(MEM_MIN_SIZE, MEM_ALIGN_SIZE, MEM_COUNT);
    }
    inline void* exploit() { return ref().exploit(); }
    inline void back(void* addr) { return ref().back(addr); }

    inline char* first_chunk() { return ref().first_chunk(); }
    inline const char* first_chunk() const { return ref().first_chunk(); }
    inline u32   chunk_size() const { return ref().chunk_size(); }
    inline u32   max_size()const { return ref().max_size(); }
    inline u32   window_size() const { return ref().window_size(); } //history's largest end id for used.     
    inline u32   size()const { return ref().size(); }
    inline u32   empty()const { return ref().empty(); }
    inline u32   full()const { return ref().full(); }

    template<class _Ty>
    inline _Ty* create() { return ref().template create<_Ty>(); }
    template<class _Ty, class... Args >
    inline _Ty* create(Args&&... args) { return ref().template create<_Ty, Args ...>(std::forward<Args>(args) ... ); }

    template<class _Ty>
    inline void destroy(const _Ty* obj) { ref().destroy(obj); }
private:
    inline zpool& ref() { return  *((zpool*)solo_); }
    inline const zpool& ref() const { return  *((zpool*)solo_); }
    constexpr static u32 SPACE_SIZE = zpool::calculate_total_size(MEM_MIN_SIZE, MEM_ALIGN_SIZE, MEM_COUNT);
    char solo_[SPACE_SIZE];
};

template<class _Ty, u32 ChunkCount>
class zpool_obj_static : public zpool_static<sizeof(_Ty), ChunkCount, (alignof(_Ty) > sizeof(u32) ? alignof(_Ty)  : sizeof(u32)  )>
{
public:
    using zsuper = zpool_static<sizeof(_Ty), ChunkCount, (alignof(_Ty) > sizeof(u32) ? alignof(_Ty) : sizeof(u32) )>;
    zpool_obj_static()
    {
        zsuper::init();
    }

    template<class... Args >
    inline _Ty* create(Args&&... args) { return zsuper::template create<_Ty, Args ...>(std::forward<Args>(args) ...); }

    inline void destroy(const _Ty* obj) { zsuper::destory(obj); }

};




#endif