
/*
* zallocator License
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


#include "zmalloc.h"
#include <memory>
#include <limits>
#include <cstddef>
#ifndef  ZALLOCATOR_H
#define ZALLOCATOR_H


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


template <class _Ty, unsigned short _Color = 0>
class zallocator
{
public:
    using value_type = _Ty;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind { using other = zallocator<U, _Color>; };

    inline explicit zallocator() {}
    inline ~zallocator() {}
    inline  zallocator(const zallocator&) {}
    template<typename U>
    inline zallocator(const zallocator<U, _Color>&) {}

    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) const { return &r; }
    inline size_type max_size() const { return (std::numeric_limits<size_type>::max)() / sizeof(_Ty); }

    inline pointer allocate(size_type cnt, typename std::allocator<void>::const_pointer = 0)
    {
        return reinterpret_cast<pointer>(zmalloc::instance().alloc_memory<_Color>((u64)cnt * (u64)sizeof(_Ty)));
    }
    inline void deallocate(pointer p, size_type c)
    {
        u64 free_size = zmalloc::instance().free_memory(p);
        (void)free_size;
        (void)c;
        //assert free_size > c * sizeof(Ty); 
    }

    template <class... Args>
    inline void construct(pointer p, Args&&... args) { new (p) _Ty(std::forward<Args>(args)...); }
    inline void construct(pointer p) { new (p) _Ty(); }
    inline void construct(pointer p, const_reference v) { new ((void*)p) _Ty(v); }

    inline void destroy(pointer p) { ((_Ty*)p)->~_Ty(); }

    inline bool operator==(const zallocator&)const { return true; }
    inline bool operator !=(const zallocator& a)const { return !operator==(a); }
};


#endif