

/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once 
#ifndef ZLIST_H
#define ZLIST_H

#include <stdint.h>
#include <iterator>
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
//#define ZLIST_USED_FENCE

template<class list_type>
struct const_zlist_iterator;

template<class list_type>
struct zlist_iterator : public std::iterator<std::bidirectional_iterator_tag, typename list_type::value_type>
{
    using node_type = typename list_type::node_type;
    using difference_type = typename list_type::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename list_type::value_type;
    using pointer = typename list_type::pointer;
    using reference = typename list_type::reference;

    zlist_iterator(const node_type* const head, u32 id) { head_ = const_cast<node_type*>(head); id_ = id; }
    zlist_iterator(const zlist_iterator<list_type>& other) { head_ = const_cast<node_type*>(other.head_); id_ = other.id_; }
    zlist_iterator(const const_zlist_iterator<list_type>& other) { head_ = const_cast<node_type*>(other.head_); id_ = other.id_; }
    zlist_iterator() :zlist_iterator(NULL, 0) {}
    zlist_iterator& operator ++()
    {
        id_ = (head_ + id_)->next;
        return *this;
    }
    zlist_iterator& operator --()
    {
        id_ = (head_ + id_)->front;
        return *this;
    }
    zlist_iterator operator ++(int)
    {
        zlist_iterator old = *this;
        id_ = (head_ + id_)->next;
        return old;
    }
    zlist_iterator operator --(int)
    {
        zlist_iterator old = *this;
        id_ = (head_ + id_)->front;
        return old;
    }

    pointer operator ->() const
    {
        return list_type::node_cast(*(head_ + id_));
    }
    reference operator *() const
    {
        return *list_type::node_cast(*(head_ + id_));
    }

public:
    node_type* head_;
    u32 id_;
};
template<class list_type>
bool operator == (const zlist_iterator<list_type>& n1, const zlist_iterator<list_type>& n2)
{
    return n1.head_ == n2.head_ && n1.id_ == n2.id_;
}
template<class list_type>
bool operator != (const zlist_iterator<list_type>& n1, const zlist_iterator<list_type>& n2)
{
    return !(n1 == n2);
}


/* type_traits:
*
* is_trivially_copyable: safely
    * memset: safely
    * memcpy: safely
* shm resume : safely
    * has vptr:     no
    * static var:   no
    * has heap ptr: no
    * has code ptr: no
* thread safe: read safe
*
*/

template<s32 _ID = 0>
class zlist_debug_error_helper
{
public:
    // record error when open fence; don't worry about thead safety. 
    static s32 debug_error_;
};
template<s32 _ID>
s32 zlist_debug_error_helper<_ID>::debug_error_ = 0;


//È«¾²Ì¬Ë«ÏòÁ´±í  
template<class _Ty, size_t _Size>
class zlist
{
public:
    struct node_type;
    using value_type = _Ty;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = _Ty*;
    using const_pointer = const _Ty*;
    using reference = _Ty&;
    using const_reference = const _Ty&;

    static const u32 FENCE_VAL = 0xdeadbeaf;
    static const u64 MAX_SIZE = _Size;
    static const u64 LIST_SIZE = _Size + 1;
    static const u64 END_ID = _Size;
    using iterator = zlist_iterator<zlist<_Ty, _Size>>;
    using const_iterator = iterator;

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using inner_space_type = typename std::aligned_storage<sizeof(_Ty), alignof(_Ty)>::type;
    using space_type = typename std::conditional<std::is_trivial<_Ty>::value, _Ty, inner_space_type>::type;
public:
    struct node_type
    {
#ifdef ZLIST_USED_FENCE
        u32 fence;
#endif
        u32 front;
        u32 next;
        space_type space;
    };
    static _Ty* node_cast(node_type& node) { return reinterpret_cast<_Ty*>(&node.space); }
public:

    zlist()
    {
        init();
    }
    ~zlist()
    {
        if (!std::is_trivial<_Ty>::value)
        {
            clear();
        }
    }

    zlist(std::initializer_list<_Ty> init_list)
    {
        init();
        assign(init_list.begin(), init_list.end());
    }

    zlist(const zlist<_Ty, _Size>& other)
    {
        if (data() == other.data())
        {
            return;
        }
        if (std::is_trivial<_Ty>::value && other.size() * 4 >= other.exploit_offset_) 
        {
            plat_init(other);
        }
        else
        {
            init();
            assign(other.begin(), other.end());
        }
    }

    zlist<_Ty, _Size>& operator = (const zlist<_Ty, _Size>& other)
    {
        if (data() == other.data())
        {
            return *this;
        }
        if (std::is_trivial<_Ty>::value && other.size() * 4 >= other.exploit_offset_)
        {
            plat_init(other);
        }
        else
        {
            clear();
            assign(other.begin(), other.end());
        }
        return *this;
    }
    
    


    //std::array api
    iterator begin() noexcept { return iterator(&data_[0], used_head_id_); }
    const_iterator begin() const noexcept { return const_iterator(&data_[0], used_head_id_); }
    const_iterator cbegin() const noexcept { return const_iterator(&data_[0], used_head_id_); }

    iterator end() noexcept { return iterator(&data_[0], END_ID); }
    const_iterator end() const noexcept { return const_iterator(&data_[0], END_ID); }
    const_iterator cend() const noexcept { return const_iterator(&data_[0], END_ID); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }


    reference front() { return *node_cast(data_[used_head_id_]); }
    const_reference front() const { return *node_cast(data_[used_head_id_]); }
    reference back() { return *node_cast(data_[data_[END_ID].front]); }
    const_reference back() const { *node_cast(data_[data_[END_ID].front]); }

    static constexpr u32 static_buf_size(u32 obj_count) { return sizeof(zlist<_Ty, 1>) + sizeof(node_type) *( obj_count -1); }


    const size_type size() const noexcept { return used_count_; }
    const size_type max_size()  const noexcept { return END_ID; }
    const bool empty() const noexcept { return !used_count_; }
    const bool full() const noexcept { return free_id_ == END_ID && exploit_offset_ >= END_ID; }
    size_type capacity() const { return max_size(); }
    const node_type* data() const noexcept { return data_; }
    const bool is_valid_node(void* addr) const noexcept
    {
        u64 uaddr = (u64)addr;
        u64 udata = (u64)data_;
        if (uaddr < udata || uaddr >= udata + sizeof(node_type) * max_size())
        {
            return false;
        }
        return true;
    }

    void clear()
    {
        u32 node_id = used_head_id_;
        if (!std::is_trivial<_Ty>::value)
        {
            while (node_id != END_ID)
            {
                node_cast(data_[node_id])->~_Ty();
                node_id = data_[node_id].next;
            }
        }
        init();
    }

    void fill(const _Ty& value)
    {
        clear();
        insert(end(), max_size(), value);
    }


private:
    void init()
    {
        used_count_ = 0;
        free_id_ = END_ID;
        exploit_offset_ = 0;
        data_[END_ID].next = END_ID;
        data_[END_ID].front = END_ID;
#ifdef ZLIST_USED_FENCE
        data_[END_ID].fence = FENCE_VAL;
#endif
        used_head_id_ = END_ID;
    }

    void plat_init(const zlist<_Ty, _Size>& other)
    {
        used_count_ = other.used_count_;
        free_id_ = other.free_id_;
        exploit_offset_ = other.exploit_offset_;
        used_head_id_ = other.used_head_id_;
        data_[END_ID] = other.data_[END_ID];
        memcpy(data_, other.data_, other.exploit_offset_ * sizeof(node_type));
    }




    bool push_free_node(u32 id)
    {
        node_type& node = data_[id];
#ifdef ZDEBUG_DEATH_MEMORY
        memset(&node.space, 0xdf, sizeof(space_type));
#endif // ZDEBUG_DEATH_MEMORY
        node.next = free_id_;
        node.front = END_ID;
        free_id_ = id;
        return true;
    }
    u32 pop_free_node()
    {
        if (free_id_ != END_ID)
        {
            u32 new_id = free_id_;
            free_id_ = data_[new_id].next;
            return new_id;
        }
        if (exploit_offset_ < END_ID)
        {
            u32 new_id = exploit_offset_++;
#ifdef ZLIST_USED_FENCE
            data_[new_id].fence = FENCE_VAL;
            data_[new_id + 1].fence = FENCE_VAL;
#endif
            return new_id;
        }
        return END_ID;
    }

    bool pick_used_node(u32 id)
    {
        if (id >= END_ID)
        {
            zlist_debug_error_helper<>::debug_error_ ++;
            return false;
        }
#ifdef ZLIST_USED_FENCE
        if (data_[id + 1].fence != FENCE_VAL)
        {
            zlist_debug_error_helper<>::debug_error_++;
            return false;
        }
#endif
        node_type& node = data_[id];
#ifdef ZLIST_USED_FENCE
        if (node.fence != FENCE_VAL)
        {
            zlist_debug_error_helper<>::debug_error_++;
            return false;
        }
#endif
        if (used_head_id_ >= END_ID)
        {
            zlist_debug_error_helper<>::debug_error_++;
            return false; //empty
        }
        if (!std::is_trivial<_Ty>::value)
        {
            node_cast(node)->~_Ty();
        }

        if (used_head_id_ == id)
        {
            used_head_id_ = node.next;
            data_[used_head_id_].front = END_ID;
        }
        else
        {
            data_[node.front].next = node.next;
            data_[node.next].front = node.front;
        }
        used_count_--;
        return true;
    }
    bool pick_and_release_used_node(u32 id)
    {
        if (pick_used_node(id))
        {
            return push_free_node(id);
        }
//            LogError() << "release used node error";
        return false;
    }
    void inject_node(u32 pos_id, u32 new_id)
    {
        data_[new_id].next = pos_id;
        data_[new_id].front = data_[pos_id].front;
        if (pos_id == used_head_id_)
        {
            used_head_id_ = new_id;
        }
        else
        {
            data_[data_[pos_id].front].next = new_id;
        }

        data_[pos_id].front = new_id;
        used_count_++;
#ifdef ZDEBUG_UNINIT_MEMORY
        memset(&data_[new_id].space, 0xfd, sizeof(space_type));
#endif // ZDEBUG_UNINIT_MEMORY
    }

    template<class T = _Ty>
    u32 inject(u32 id, const _Ty & value, typename std::enable_if<std::is_trivial<T>::value>::type* = 0)
    {
        u32 new_id = pop_free_node();
        if (new_id == END_ID)
        {
            return END_ID;
        }
        inject_node(id, new_id);
        *node_cast(data_[new_id]) = value;
        return new_id;
    }

    template<class T = _Ty, typename std::enable_if < !std::is_trivial<T>{}, int > ::type = 0 >
    u32 inject(u32 id, const _Ty& value)
    {
        u32 new_id = pop_free_node();
        if (new_id == END_ID)
        {
            return END_ID;
        }
        inject_node(id, new_id);
        new (&data_[new_id].space) _Ty(value);
        return new_id;
    }


    template< class... Args >
    u32 inject_emplace(u32 id, Args&&... args)
    {
        u32 new_id = pop_free_node();
        if (new_id == END_ID)
        {
            return END_ID;
        }
        inject_node(id, new_id);
        new (&data_[new_id].space) _Ty(args ...);
        return new_id;
    }
public:
    iterator insert(iterator pos, const _Ty& value)
    {
        return iterator(&data_[0], inject(pos.id_, value));
    }

    template< class... Args >
    iterator emplace(iterator pos, Args&&... args)
    {
        return iterator(&data_[0], inject_emplace(pos.id_, args...));
    }

    iterator insert(iterator pos, size_type count, const _Ty& value)
    {
        if (used_count_ + count > max_size())
        {
            return end();
        }
        if (count == 0)
        {
            return end();
        }
        for (size_t i = 0; i < count; i++)
        {
            pos.id_ = inject(pos.id_, value);
        }
        return pos;
    }
    //[first, last)
    template<class other_iterator>
    iterator insert(iterator pos, other_iterator first, other_iterator last)
    {
        if (first == last)
        {
            return end();
        }
        --last;
        while (first != last)
        {
            pos = insert(pos, *(last--));
        }
        pos = insert(pos, *last);
        return pos;
    }

    template<class other_iterator>
    iterator assign(other_iterator first, other_iterator last)
    {
        clear();
        return insert(end(), first, last);
    }


    void push_back(const _Ty& value) { inject(END_ID, value); }
    bool pop_back() { return pick_and_release_used_node(data_[END_ID].front); }
    void push_front(const _Ty& value) { inject(used_head_id_, value); }
    bool pop_front() { return pick_and_release_used_node(used_head_id_); }

    iterator erase(iterator pos)
    {
        u32 pos_id = pos.id_;
        pos++;
        if (!pick_and_release_used_node(pos_id))
        {
            pos = end();
        }
        return pos;
    }

    //[first,last)
    iterator erase(iterator first, iterator last)
    {
        while (first != last)
        {
            erase(first++);
        }
        return last;
    }


    template< class... Args >
    iterator emplace_back(Args&&... args)
    {
        return emplace(end(), args...);
    }
    template< class... Args >
    iterator emplace_front(Args&&... args)
    {
        return emplace(begin(), args...);
    }

private:
    template<class Comp>
    iterator comp_bound(iterator first, iterator last, const _Ty& value, Comp comp)
    {
        while (first != last)
        {
            if (comp(*first, value))
            {
                first++;
                continue;
            }
            break;
        }
        return first;
    }
public:
    template<class Less = std::less<_Ty>>
    iterator lower_bound(iterator first, iterator last, const _Ty& value, Less less = Less())
    {
        return comp_bound(first, last, value, less);
    }
    template<class Greater = std::greater<_Ty>>
    iterator upper_bound(iterator first, iterator last, const _Ty& value, Greater greater = Greater())
    {
        return comp_bound(first, last, value, greater);
    }

private:
    u32 used_count_;
    u32 free_id_;
    u32 exploit_offset_;
    u32 used_head_id_;
    node_type data_[LIST_SIZE];
};

template<class _Ty, size_t _Size>
bool operator==(const zlist<_Ty, _Size>& a1, const zlist<_Ty, _Size>& a2)
{
    return a1.data() == a2.data();
}





#endif