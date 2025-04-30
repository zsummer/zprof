

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once 
#ifndef  ZHASH_MAP_H
#define ZHASH_MAP_H

#include <stdint.h>
#include <type_traits>
#include <cstddef>



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

// init new memory with 0xfd    
//#define ZDEBUG_UNINIT_MEMORY

// backed memory immediately fill 0xdf 
//#define ZDEBUG_DEATH_MEMORY  

// open and check fence 
// not support


template<class node_type, class value_type, u32 INVALID_NODE_ID, u32 HASH_COUNT>
struct zhash_map_iterator
{
    node_type* node_pool_;
    u32 cur_node_id_;
    u32 max_node_id_; //迭代器使用过程中不更新 即迭代器一旦创建 不保证能迭代到新增元素;   

    operator node_type* () const { return node_pool_[cur_node_id_]; }
    operator const node_type* ()const { return node_pool_[cur_node_id_]; }
    zhash_map_iterator()
    {
        node_pool_ = NULL;
        cur_node_id_ = INVALID_NODE_ID;
        max_node_id_ = 0;
    }
    zhash_map_iterator(const node_type* pool,  u32 node_id, u32 max_node_id)
    {
        node_pool_ = const_cast<node_type *>(pool);
        cur_node_id_ = node_id;
        max_node_id_ = max_node_id;
    }
    zhash_map_iterator(const zhash_map_iterator& other)
    {
        node_pool_ = other.node_pool_;
        cur_node_id_ = other.cur_node_id_;
        max_node_id_ = other.max_node_id_;
    }

    void next()
    {
        if (node_pool_ == NULL)
        {
            return;
        }

        for (u32 i = cur_node_id_ + 1; i < max_node_id_; i++)
        {
            if (node_pool_[i].hash_id < HASH_COUNT)
            {
                cur_node_id_ = i;
                return;
            }
        }
        cur_node_id_ = INVALID_NODE_ID;
        return;
    }

    zhash_map_iterator& operator ++()
    {
        next();
        return *this;
    }

    zhash_map_iterator operator ++(int)
    {
        zhash_map_iterator result(*this);
        next();
        return result;
    }

    value_type* operator ->()
    {
        return (value_type*)&node_pool_[cur_node_id_].val_space;
    }
    value_type& operator *()
    {
        return *((value_type*)&node_pool_[cur_node_id_].val_space);
    }
};

template<class node_type, class value_type, u32 INVALID_NODE_ID, u32 HASH_COUNT>
bool operator == (const zhash_map_iterator<node_type, value_type, INVALID_NODE_ID, HASH_COUNT>& n1, const zhash_map_iterator<node_type, value_type, INVALID_NODE_ID, HASH_COUNT>& n2)
{
    return n1.node_pool_ == n2.node_pool_ && n1.cur_node_id_ == n2.cur_node_id_;
}
template<class node_type, class value_type, u32 INVALID_NODE_ID, u32 HASH_COUNT>
bool operator != (const zhash_map_iterator<node_type, value_type, INVALID_NODE_ID, HASH_COUNT>& n1, const zhash_map_iterator<node_type, value_type, INVALID_NODE_ID, HASH_COUNT>& n2)
{
    return !(n1 == n2);
}


template <class Key>
struct zhash
{
    u64 operator()(const Key& key) const
    {
        u64 hash_key = (u64)key;
        static const u64 h = (0x84222325ULL << 32) | 0xcbf29ce4ULL;
        static const u64 kPrime = (0x00000100ULL << 32) | 0x000001b3ULL;
        hash_key ^= h;
        hash_key *= kPrime;
        return hash_key;
    }
};
template <class Key, class Val>
struct zhash_get_pair_key
{
    const Key& operator()(std::pair<const Key&, const Val&> v) const
    {
        return v.first;
    }
};

template <class Key>
struct zhash_get_key
{
    const Key& operator()(const Key& key) const
    {
        return key;
    }
};


/* type_traits:  (when _Ty is is_trivially_copyable)
*
* is_trivially_copyable: safely
    * memset: no(need call reset);
    * memcpy: safely;
* shm resume:  safely
    * has vptr:     no
    * static var:   no
    * has heap ptr: no
    * has code ptr: no
    * has sys ptr:  no
* thread safe: read safely
*
*/


/*
* 支持obj和pod: 针对pod和非pod有静态模版分支 pod更快.
* 固定长度, 其中桶数量为总长2倍. 即hash因子是0.5
* 如果追求更快的性能 _Size数量应该是2的幂次方; 例如 32, 64, 1024 ...
* std自带的hash为取模, 如果key在取模后的数据可能存在大量冲突 推荐用zhash, 以hash的性能消耗换取更小的碰撞冲突来获得整体的性能提升  
*/
template<class Key,
    class Value,
    u32 _Size,
    class GetKey,
    class Hash>
    class zhash_map_impl
{
public:

    using size_type = u32;
    const static size_type FREE_POOL_SIZE = 0;
    const static size_type NODE_COUNT = _Size;
    const static size_type INVALID_NODE_ID = NODE_COUNT + 1;
    const static size_type HASH_COUNT = NODE_COUNT * 2;
    constexpr size_type max_size() const { return NODE_COUNT; }
    //constexpr size_type max_bucket_count() const { return HASH_COUNT; }
    using key_type = Key;
    using value_type = Value;
    using reference = value_type&;
    using const_reference = const value_type&;

    using inner_space_type = typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type;
    using space_type = typename std::conditional<std::is_trivial<value_type>::value, value_type, inner_space_type>::type;

    struct node_type
    {
        size_type next;
        size_type hash_id;
        space_type val_space;
    };
    using iterator = zhash_map_iterator<node_type, value_type, INVALID_NODE_ID, HASH_COUNT>;
    using const_iterator = iterator;
protected:
    size_type buckets_[HASH_COUNT];
    node_type node_pool_[INVALID_NODE_ID+1]; // dereference end() will panic;  it's user error.  
    size_type first_valid_node_id_;
    size_type exploit_offset_; //is the last valid node index(unexploit it the next index) & the value is the buckets used nodes num. 
    size_type count_;
    iterator mi(size_type node_id) const noexcept { return iterator(node_pool_, node_id, exploit_offset_ + 1); }
    static reference rf(node_type& b)  { return *reinterpret_cast<value_type*>(&b.val_space); }

    void reset()
    {
        exploit_offset_ = 0;
        count_ = 0;
        first_valid_node_id_ = 0;
        node_pool_[FREE_POOL_SIZE].next = 0;
        node_pool_[FREE_POOL_SIZE].hash_id = HASH_COUNT;
        memset(buckets_, 0, sizeof(size_type) * HASH_COUNT);
    }

    size_type pop_free()
    {
        if (node_pool_[FREE_POOL_SIZE].next != FREE_POOL_SIZE)
        {
            size_type ret = node_pool_[FREE_POOL_SIZE].next;
            node_pool_[ret].hash_id = HASH_COUNT;
            node_pool_[FREE_POOL_SIZE].next = node_pool_[ret].next;
            count_++;
            if (first_valid_node_id_ == FREE_POOL_SIZE || ret < first_valid_node_id_)
            {
                first_valid_node_id_ = ret;
            }
            return ret;
        }
        if (exploit_offset_ < NODE_COUNT)
        {
            size_type ret = ++exploit_offset_;
            node_pool_[ret].hash_id = HASH_COUNT;
            count_++;
            if (first_valid_node_id_ == FREE_POOL_SIZE || ret < first_valid_node_id_)
            {
                first_valid_node_id_ = ret;
            }
            return ret;
        }
        return FREE_POOL_SIZE;
    }

    void push_free(size_type node_id)
    {
        node_pool_[node_id].hash_id = HASH_COUNT;
        node_pool_[node_id].next = node_pool_[FREE_POOL_SIZE].next;
        node_pool_[FREE_POOL_SIZE].next = node_id;
        count_--;
        if (node_id == first_valid_node_id_)
        {
            first_valid_node_id_ = next_b(first_valid_node_id_+1).cur_node_id_;
        }
#ifdef ZDEBUG_DEATH_MEMORY
        memset(&node_pool_[node_id].val_space, 0xdf, sizeof(space_type));
#endif // ZDEBUG_DEATH_MEMORY
    }

    iterator next_b(size_type node_id) const
    {
        size_type exploit_offset = exploit_offset_ > NODE_COUNT ? NODE_COUNT : exploit_offset_; //clear gcc warning. it's safe. 
        for (size_type i = node_id; i <= exploit_offset; i++)
        {
            if (node_pool_[i].hash_id != HASH_COUNT)
            {
                return mi(i);
            }
        }
        return end();
    }



    std::pair<iterator, bool> insert_v(const value_type& val, bool assign)
    {
        iterator finder = find(GetKey()(val));
        if (finder != end())
        {
            if (assign)
            {
                *finder = val;
            }
            return { finder, false };
        }

        auto ukey = Hash()(GetKey()(val));
        size_type hash_id = (size_type)(ukey % HASH_COUNT);

        size_type new_node_id = pop_free();
        if (new_node_id == FREE_POOL_SIZE)
        {
            return { end(), false };
        }
        node_type& node = node_pool_[new_node_id];
        node.next = FREE_POOL_SIZE;
        node.hash_id = (size_type)hash_id;
        if (!std::is_trivial<value_type>::value)
        {
            new (&node.val_space) value_type(val);
        }
        else
        {
            memcpy(&node.val_space, &val, sizeof(val));
        }

        if (buckets_[hash_id] != FREE_POOL_SIZE)
        {
            node.next = buckets_[hash_id];
        }
        buckets_[hash_id] = new_node_id;
        return { mi(new_node_id), true };
    }

public:
    iterator begin() noexcept { return next_b(first_valid_node_id_); }
    const_iterator begin() const noexcept { return next_b(first_valid_node_id_); }
    const_iterator cbegin() const noexcept { return next_b(first_valid_node_id_); }

    iterator end() noexcept { return mi(INVALID_NODE_ID); }
    const_iterator end() const noexcept { return mi(INVALID_NODE_ID); }
    const_iterator cend() const noexcept { return mi(INVALID_NODE_ID); }

public:
    zhash_map_impl()
    {
        reset();
    }
    zhash_map_impl(std::initializer_list<value_type> init)
    {
        reset();
        for (const auto& v : init)
        {
            insert(v);
        }
    }
    ~zhash_map_impl()
    {
        if (!std::is_trivial<value_type>::value)
        {
            for (reference kv : *this)
            {
                kv.~value_type();
            }
        }
    }
    void clear()
    {
        if (!std::is_trivial<value_type>::value)
        {
            for (reference kv : *this)
            {
                kv.~value_type();
            }
        }
        reset();
    }

    const size_type size() const noexcept { return count_; }
    const bool empty() const noexcept { return !size(); }
    const bool full() const noexcept { return size() == NODE_COUNT; }
    size_type bucket_size(size_type bid)
    {
        return count_;
    }
    float load_factor() const
    {
        return size() / 1.0f / HASH_COUNT;
    }
    std::pair<iterator, bool> insert(const value_type& val)
    {
        return insert_v(val, false);
    }

    iterator find(const key_type& key)
    {
        auto ukey = Hash()(key);
        size_type hash_id = (size_type)(ukey % HASH_COUNT);
        size_type node_id = buckets_[hash_id];
        while (node_id != FREE_POOL_SIZE && GetKey()(rf(node_pool_[node_id])) != key)
        {
            node_id = node_pool_[node_id].next;
        }
        if (node_id != FREE_POOL_SIZE)
        {
            return mi(node_id);
        }
        return end();
    }

    bool contains(const key_type& key)
    {
        return find(key) != end();
    }


    iterator erase(iterator iter)
    {
        size_type node_id = iter.cur_node_id_;
        if (node_id == FREE_POOL_SIZE || node_id > exploit_offset_)
        {
            return end();
        }
        node_type& node = node_pool_[node_id];


        size_type hash_id = node.hash_id;
            
        if (hash_id >= HASH_COUNT)
        {
            return end();
        }
        if (buckets_[hash_id] == FREE_POOL_SIZE)
        {
            return end();
        }
            
        size_type pre_node_id = buckets_[hash_id];
        if (pre_node_id == node_id)
        {
            buckets_[hash_id] = node_pool_[node_id].next;
        }
        else
        {
            size_type cur_node_id = pre_node_id;
            while (node_pool_[cur_node_id].next != FREE_POOL_SIZE && node_pool_[cur_node_id].next != node_id)
            {
                cur_node_id = node_pool_[cur_node_id].next;
            }
            if (node_pool_[cur_node_id].next != node_id)
            {
                return end();
            }
            node_pool_[cur_node_id].next = node_pool_[node_id].next;
        }

        if (!std::is_trivial<value_type>::value)
        {
            rf(node).~value_type();
        }
        push_free(node_id);
        return begin();
    }


    iterator erase(const key_type& key)
    {
        auto ukey = Hash()(key);
        size_type hash_id = (size_type)(ukey % HASH_COUNT);
        size_type pre_node_id = buckets_[hash_id];
        if (pre_node_id == FREE_POOL_SIZE)
        {
            return end();
        }

        size_type node_id = FREE_POOL_SIZE;
        if (GetKey()(rf(node_pool_[pre_node_id])) == key)
        {
            node_id = pre_node_id;
            buckets_[hash_id] = node_pool_[pre_node_id].next;
        }
        else
        {
            size_type cur_node_id = pre_node_id;
            while (node_pool_[cur_node_id].next != FREE_POOL_SIZE)
            {
                if (GetKey()(rf(node_pool_[node_pool_[cur_node_id].next])) != key)
                {
                    cur_node_id = node_pool_[cur_node_id].next;
                    continue;
                }
                node_id = node_pool_[cur_node_id].next;
                node_pool_[cur_node_id].next = node_pool_[node_pool_[cur_node_id].next].next;
                break;
            }
            if (node_id == FREE_POOL_SIZE)
            {
                return end();
            }
        }

        if (!std::is_trivial<value_type>::value)
        {
            rf(node_pool_[node_id]).~value_type();
        }
        push_free(node_id);
        return begin();
    }

};



template<class Key,
    class _Ty,
    u32 _Size,
    class Hash = std::hash<Key>>
    class zhash_map: public zhash_map_impl<Key, std::pair<Key, _Ty>,  _Size, zhash_get_pair_key<Key, _Ty>, Hash>
{
public:
    using supper_map = zhash_map_impl<Key, std::pair<Key, _Ty>, _Size, zhash_get_pair_key<Key, _Ty>, Hash>;
    using value_type = typename supper_map::value_type;
    using key_type = typename supper_map::key_type;
    using iterator = typename supper_map::iterator;
    using const_reference = typename supper_map::const_reference;
    using mapped_type = _Ty;
    zhash_map()
    {
    }
    zhash_map(std::initializer_list<value_type> init):supper_map(init)
    {
    }
    mapped_type& operator[](const key_type& key)
    {
        std::pair<iterator, bool> ret = supper_map::insert_v(std::make_pair(key, mapped_type()), false);
        if (ret.first != supper_map::end())
        {
            return ret.first->second;
        }
        throw std::overflow_error("mapped_type& operator[](const key_type& key)");
    }
};

template<class Key,
    u32 _Size,
    class Hash = std::hash<Key>>
    class zhash_set : public zhash_map_impl<Key, Key, _Size, zhash_get_key<Key>, Hash>
{
public:
    using supper_map = zhash_map_impl<Key, Key, _Size, zhash_get_key<Key>, Hash>;
    using value_type = typename supper_map::value_type;
    using key_type = typename supper_map::key_type;
    using iterator = typename supper_map::iterator;
    using const_reference = typename supper_map::const_reference;
    zhash_set() 
    {
    }
    zhash_set(std::initializer_list<value_type> init) :supper_map(init)
    {
    }
};



#endif