

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/

#pragma once
#ifndef ZMEM_COLOR_H_
#define ZMEM_COLOR_H_

#include <stdint.h>
#include <map>
#include <set>
#include <vector>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <queue>
#include <cstddef>

#include "zallocator.h"
#include "zlist_ext.h"
#include "zvector.h"



enum zmem_color_enum
{
    BASE_MEM_COLOR_NULL = 0,
    BASE_MEM_COLOR_STRING = 1,
    BASE_MEM_COLOR_VECTOR = 2,
    BASE_MEM_COLOR_ZVECTOR = 3,
    BASE_MEM_COLOR_MAP = 4,
    BASE_MEM_COLOR_SET = 5,
    BASE_MEM_COLOR_LIST = 6,
    BASE_MEM_COLOR_SEG_LIST = 7,
    BASE_MEM_COLOR_QUEUE = 8,
    BASE_MEM_COLOR_MAX,
};

static_assert(BASE_MEM_COLOR_MAX < zmalloc::CHUNK_COLOR_MASK / 2, "color max");

using shm_string = std::basic_string<char, std::char_traits<char>, zallocator<char, BASE_MEM_COLOR_STRING> >;

template<class _Ty>
using shm_vector = std::vector<_Ty, zallocator<_Ty, BASE_MEM_COLOR_VECTOR>>;

template<class _Ty, size_t _Size, size_t _FixedSize>
using shm_zvector = zvector<_Ty, _Size, _FixedSize, zallocator<zvector_aligned_space_helper<_Ty>, BASE_MEM_COLOR_ZVECTOR>>;

template<class K, class T, class P = std::less<K> >
using shm_map = std::map<K, T, P, zallocator<std::pair<const K, T>, BASE_MEM_COLOR_MAP> >;
template<class K, class T, class P = std::less<K> >
using shm_multimap = std::multimap<K, T, P, zallocator<std::pair<const K, T>, BASE_MEM_COLOR_MAP> >;
template<class K, class T>
using shm_unordered_map = std::unordered_map< K, T, std::hash<K>, std::equal_to<K>, zallocator<std::pair<const K, T>, BASE_MEM_COLOR_MAP> >;


template<class K, class P = std::less<K>>
using shm_set = std::set<K, P, zallocator<K, BASE_MEM_COLOR_SET> >;
template<class K, class H = std::hash<K>, class E = std::equal_to<K>>
using shm_unordered_set = std::unordered_set< K, H, E, zallocator<const K, BASE_MEM_COLOR_SET> >;


template<class _Ty>
using shm_list = std::list<_Ty, zallocator<_Ty, BASE_MEM_COLOR_LIST>>;

template<class _Ty, size_t _Size, size_t _FixedSize>
using shm_zlist_ext = zlist_ext<_Ty, _Size, _FixedSize, zallocator<zlist_aligned_space_helper<_Ty>, BASE_MEM_COLOR_SEG_LIST>>;

template<class _Ty>
using shm_deque = std::deque<_Ty, zallocator<_Ty, BASE_MEM_COLOR_QUEUE>>;

template<class _Ty>
using shm_queue = std::queue<_Ty, shm_deque<_Ty>>;

//unresumable area.  



#endif



